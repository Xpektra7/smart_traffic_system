#include "Ultrasonic.h"

void UltrasonicSensor(const char* name, UltrasonicState &state, unsigned long readDuration, int trigPin, int echoPin) {
  const int debounceCount = 2;
  const float riseThreshold = 10;
  const unsigned long pauseDuration = 10000;
  const unsigned long sampleInterval = 100; // ~100ms per ultrasonic ping

  if (!state.initialized) {
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    state.cycleStart = millis();
    state.lastSample = 0;
    state.initialized = true;
  }

  auto getDistance = [&]() {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    long duration = pulseIn(echoPin, HIGH, 30000);
    float d = (duration * 0.0343) / 2;
    return (d > 550 || d <= 0) ? 550 : d;
  };

  auto getAverageDistance = [&]() {
    state.readings[state.readIndex] = getDistance();
    state.readIndex = (state.readIndex + 1) % UltrasonicState::numReadings;
    float sum = 0;
    for (int i = 0; i < UltrasonicState::numReadings; i++) sum += state.readings[i];
    return sum / UltrasonicState::numReadings;
  };

  unsigned long now = millis();

  // ---- only sample every 100ms ----
  if (now - state.lastSample < sampleInterval) return;
  state.lastSample = now;

  if (state.readingPhase) {
    float avg = getAverageDistance();

    if (avg > state.peakDistance) {
      state.peakDistance = avg;
      state.triggerDistance = state.peakDistance - 150;
    }

    Serial.print(name); 
    Serial.print(" => Distance: "); Serial.print(avg);
    Serial.print("  Count: "); Serial.println(state.vehicleCount);

    if (avg < state.triggerDistance && !state.carPresence) {
      state.entryCounter++;
      if (state.entryCounter >= debounceCount) {
        state.carPresence = true;
        state.entryCounter = 0;
        state.entryDistance = avg;
        state.entryTime = now;
        Serial.print(name); Serial.println(" => Car entered");
      }
    } else state.entryCounter = 0;

    if (state.carPresence && avg > state.lastDistance + riseThreshold) {
      state.exitCounter++;
      if (state.exitCounter >= debounceCount) {
        state.carPresence = false;
        float travelDistance = avg - state.entryDistance;
        float travelTime = (now - state.entryTime) / 1000.0;
        if (travelTime > 0) {
          float speed = (travelDistance / 100.0) / travelTime;
          state.totalSpeed += speed;
          state.speedCount++;
        }
        state.vehicleCount++;
        state.exitCounter = 0;
        Serial.print(name); Serial.println(" => Car left");
      }
    } else state.exitCounter = 0;

    state.lastDistance = avg;

    if (now - state.cycleStart >= readDuration) {
      state.vehicleCount = 0;
      state.totalSpeed = 0;
      state.speedCount = 0;
      state.readingPhase = false;
      state.cycleStart = now;
    }
  } else if (now - state.cycleStart >= pauseDuration) {
    state.readingPhase = true;
    state.cycleStart = now;
  }
}
