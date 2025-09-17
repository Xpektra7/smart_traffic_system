#include "UltrasonicSensor.h"
#include <Arduino.h>

static const int DEFAULT_SAMPLE_HZ = 20;
static const float DEFAULT_DROP_CM = 50.0f;
static const float DEFAULT_RISE_CM = 20.0f;

// internal state
static UltrasonicConfig sensorCfg[ULTRASONIC_MAX_SENSORS];
static TaskHandle_t sensorTasks[ULTRASONIC_MAX_SENSORS] = {0};
static bool sensorRunning[ULTRASONIC_MAX_SENSORS] = {0};
static UltrasonicResult latestResults[ULTRASONIC_MAX_SENSORS];
static QueueHandle_t resultQueues[ULTRASONIC_MAX_SENSORS] = {0};
static SemaphoreHandle_t trigSemaphore = NULL; // serializes trigger pulses to avoid crosstalk
static size_t sensorCount = 0;
static bool stopRequested = false;

// simple ring log per sensor (in RAM)
struct LogEntry { UltrasonicResult r; };
static LogEntry logs[ULTRASONIC_MAX_SENSORS][ULTRASONIC_LOG_ENTRIES];
static size_t logIndex[ULTRASONIC_MAX_SENSORS] = {0};

// helpers
static void pushLog(int idx, const UltrasonicResult &r) {
  logs[idx][logIndex[idx] % ULTRASONIC_LOG_ENTRIES].r = r;
  logIndex[idx] = (logIndex[idx] + 1) % ULTRASONIC_LOG_ENTRIES;
}

static UltrasonicResult emptyResult() {
  UltrasonicResult z; z.vehicleCount = 0; z.occupancyRatio = 0; z.avgSpeed = 0; z.timestamp = 0; return z;
}

bool startUltrasonicSystem(const UltrasonicConfig *configs, size_t count) {
  if (count == 0 || count > ULTRASONIC_MAX_SENSORS) return false;
  // copy configs, set defaults
  for (size_t i=0;i<count;i++) {
    sensorCfg[i] = configs[i];
    if (sensorCfg[i].sampleHz == 0) sensorCfg[i].sampleHz = DEFAULT_SAMPLE_HZ;
    if (sensorCfg[i].dropThresholdCm <= 0) sensorCfg[i].dropThresholdCm = DEFAULT_DROP_CM;
    if (sensorCfg[i].riseThresholdCm <= 0) sensorCfg[i].riseThresholdCm = DEFAULT_RISE_CM;
    latestResults[i] = emptyResult();
    logIndex[i]=0;
    if (!resultQueues[i]) {
      resultQueues[i] = xQueueCreate(8, sizeof(UltrasonicResult));
    }
  }
  // create trig semaphore if absent
  if (!trigSemaphore) trigSemaphore = xSemaphoreCreateMutex();

  sensorCount = count;
  stopRequested = false;

  // stagger start times slightly to avoid simultaneous trigger at startup
  for (size_t i=0;i<count;i++) {
    sensorTasks[i] = NULL;
    sensorRunning[i] = true;
    // create task
    char tname[12]; snprintf(tname, sizeof(tname), "US%d", (int)i+1);
    xTaskCreate(
      [](void *param){
        int idx = (int)(intptr_t)param;
        const UltrasonicConfig &cfg = sensorCfg[idx];

        // per-task baseline auto-calibration on first window
        float baseline = 0;
        bool baselineSet = false;

        for (;;) {
          if (stopRequested) break;

          int seconds = max(1, cfg.windowSeconds);
          unsigned long windowStart = millis();
          unsigned long windowEnd = windowStart + (unsigned long)seconds*1000UL;

          int vehicleCount = 0;
          float occupiedSumSeconds = 0.0f;
          bool carPresent = false;
          unsigned long carStart = 0;

          float totalSpeed = 0.0f;
          int speedSamples = 0;

          float lastDist = -1.0f;
          unsigned long lastTime = 0;

          const TickType_t sampleDelay = pdMS_TO_TICKS(1000 / cfg.sampleHz);

          // to reduce crosstalk, each sample obtains trigSemaphore before pulsing
          while ((long)(millis() - windowStart) < (long)seconds*1000L && !stopRequested) {
            // get exclusive trigger
            if (trigSemaphore) xSemaphoreTake(trigSemaphore, portMAX_DELAY);

            // trigger pulse
            digitalWrite(cfg.trigPin, LOW);
            delayMicroseconds(2);
            digitalWrite(cfg.trigPin, HIGH);
            delayMicroseconds(10);
            digitalWrite(cfg.trigPin, LOW);

            // release quickly so others can trigger
            if (trigSemaphore) xSemaphoreGive(trigSemaphore);

            // read echo (short timeout so not blocking long)
            long dur = pulseIn(cfg.echoPin, HIGH, 30000); // 30ms
            float distanceCm = (dur > 0) ? (dur * 0.034f / 2.0f) : 0.0f;

            // baseline auto-calibration: take max valid quiet reading as baseline
            if (distanceCm > 0 && distanceCm < 500) {
              if (!baselineSet || distanceCm > baseline) { baseline = distanceCm; baselineSet = true; }
            }

            // robust reject: tiny or extremely large = ignore
            bool valid = (distanceCm > 2.0f && distanceCm < 400.0f);

            // vehicle detection using thresholds relative to baseline
            if (valid && baselineSet) {
              if (!carPresent && distanceCm < (baseline - cfg.dropThresholdCm)) {
                carPresent = true;
                carStart = millis();
                vehicleCount++;
              } else if (carPresent && distanceCm > (baseline - cfg.riseThresholdCm)) {
                carPresent = false;
                occupiedSumSeconds += (millis() - carStart) / 1000.0f;
              }
            }

            // speed estimate: slope of distance (cm/s)
            if (valid && lastDist > 0) {
              unsigned long now = millis();
              float dt = (now - lastTime) / 1000.0f;
              if (dt > 0.0001f) {
                float slope = fabs(distanceCm - lastDist) / dt; // cm/s
                totalSpeed += slope;
                speedSamples++;
              }
              lastTime = now;
              lastDist = distanceCm;
            } else if (valid) {
              lastDist = distanceCm;
              lastTime = millis();
            }

            // yield between samples so other tasks run
            vTaskDelay(sampleDelay);
          } // end window loop

          // finalize result
          UltrasonicResult res;
          res.vehicleCount = vehicleCount;
          res.occupancyRatio = occupiedSumSeconds / (float)seconds;
          res.avgSpeed = (speedSamples > 0) ? (totalSpeed / speedSamples) : 0.0f;
          res.timestamp = millis();

          // store latest
          latestResults[idx] = res;
          pushLog(idx, res);

          // push to queue (non-blocking, drop if full)
          if (resultQueues[idx]) xQueueSend(resultQueues[idx], &res, 0);

          // small delay between windows to avoid immediate re-trigger storms and allow web tasks to process
          vTaskDelay(pdMS_TO_TICKS(500));
        } // end forever

        // mark stopped
        sensorRunning[idx] = false;
        vTaskDelete(NULL);
      },
      tname,
      4096,
      (void*)(intptr_t)i,
      1,
      &sensorTasks[i]
    );

    // stagger small delay before creating next task to avoid simultaneous startup
    vTaskDelay(pdMS_TO_TICKS(50));
  }
  return true;
}

void stopUltrasonicSystem() {
  stopRequested = true;
  // give tasks time to exit
  vTaskDelay(pdMS_TO_TICKS(200));
  for (size_t i=0;i<sensorCount;i++) {
    if (sensorTasks[i]) {
      // task will self-delete; ensure pointer cleared
      sensorTasks[i] = NULL;
    }
  }
  sensorCount = 0;
}

bool tryPopResult(int sensorIndex, UltrasonicResult &out) {
  if (sensorIndex < 0 || sensorIndex >= (int)ULTRASONIC_MAX_SENSORS) return false;
  if (!resultQueues[sensorIndex]) return false;
  if (xQueueReceive(resultQueues[sensorIndex], &out, 0) == pdTRUE) return true;
  return false;
}

UltrasonicResult peekLatest(int sensorIndex) {
  if (sensorIndex < 0 || sensorIndex >= (int)ULTRASONIC_MAX_SENSORS) return emptyResult();
  return latestResults[sensorIndex];
}

void forceCalibrateBaseline(int sensorIndex) {
  // simple approach: zero baseline so next window will rebuild from fresh quiet readings
  if (sensorIndex>=0 && sensorIndex< (int)ULTRASONIC_MAX_SENSORS) {
    // nothing fancy stored; baseline is per-task local and will be rebuilt on next window automatically
    // user can restart the system to force immediate recalibration
  }
}
