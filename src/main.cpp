#include <Arduino.h>

void UltrasonicSensor(unsigned long readDuration, int trigPin, int echoPin) {
  // --- persistent state (static ensures memory survives across calls) ---
  static bool initialized = false;
  static bool carPresence = false;
  static int vehicleCount = 0;

  const int numReadings = 3;
  static float readings[numReadings];
  static int readIndex = 0;

  const int debounceCount = 2;
  static int entryCounter = 0;
  static int exitCounter = 0;

  static float peakDistance = 0;
  static float triggerDistance = -150;
  static float lastDistance = 0;
  const float riseThreshold = 10;

  static unsigned long cycleStart = 0;
  const unsigned long pauseDuration = 10000; // 10s
  static bool readingPhase = true;

  // speed tracking
  static float entryDistance = 0;
  static unsigned long entryTime = 0;
  static float totalSpeed = 0;
  static int speedCount = 0;

  // one-time init
  if (!initialized) {
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    for (int i = 0; i < numReadings; i++) readings[i] = 0;
    cycleStart = millis();
    initialized = true;
  }

  // --- helper lambdas ---
  auto getDistance = [&]() {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    long duration = pulseIn(echoPin, HIGH, 30000); // 30ms timeout
    float distance = (duration * 0.0343) / 2;
    if (distance > 550 || distance <= 0) distance = 550;
    return distance;
  };

  auto getAverageDistance = [&]() {
    readings[readIndex] = getDistance();
    readIndex = (readIndex + 1) % numReadings;

    float sum = 0;
    for (int i = 0; i < numReadings; i++) sum += readings[i];
    return sum / numReadings;
  };

  // --- main logic ---
  unsigned long now = millis();

  if (readingPhase) {
    float avgDistance = getAverageDistance();

    if (avgDistance > peakDistance) {
      peakDistance = avgDistance;
      triggerDistance = peakDistance - 150;
    }

    Serial.print("Distance: "); Serial.print(avgDistance);
    Serial.print("  Count: "); Serial.println(vehicleCount);

    // entry
    if (avgDistance < triggerDistance && !carPresence) {
      entryCounter++;
      if (entryCounter >= debounceCount) {
        carPresence = true;
        entryCounter = 0;
        entryDistance = avgDistance;
        entryTime = now;
        Serial.println("Car entered");
      }
    } else entryCounter = 0;

    // exit
    if (carPresence && avgDistance > lastDistance + riseThreshold) {
      exitCounter++;
      if (exitCounter >= debounceCount) {
        carPresence = false;
        float travelDistance = avgDistance - entryDistance; // cm
        float travelTime = (now - entryTime) / 1000.0;      // s
        if (travelTime > 0) {
          float speed = (travelDistance / 100.0) / travelTime; // m/s
          totalSpeed += speed;
          speedCount++;
        }
        vehicleCount++;
        exitCounter = 0;
        Serial.println("Car left");
      }
    } else exitCounter = 0;

    lastDistance = avgDistance;

    // end of window
    if (now - cycleStart >= readDuration) {
      float avgSpeed = (speedCount > 0) ? totalSpeed / speedCount : 0;
      float flow = vehicleCount / (readDuration / 1000.0);

      Serial.print("Final count: "); Serial.println(vehicleCount);
      Serial.print("Average speed (m/s): "); Serial.println(avgSpeed);
      Serial.print("Flow (cars/s): "); Serial.println(flow);

      // reset
      vehicleCount = 0;
      totalSpeed = 0;
      speedCount = 0;
      readingPhase = false;
      cycleStart = now;
    }

  } else { // pause
    if (now - cycleStart >= pauseDuration) {
      readingPhase = true;
      cycleStart = now;
    }
  }

  delay(100); // maintain sample rate
}

// --- usage ---
void setup() {
  Serial.begin(115200);
}

void loop() {
  UltrasonicSensor(30000, 5, 32); // run 30s windows
  // you can add delay here if you want slower polling
}
