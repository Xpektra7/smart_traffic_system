#include <Arduino.h>

struct UltrasonicState {
  bool initialized = false;
  bool carPresence = false;
  int vehicleCount = 0;

  static const int numReadings = 3;
  float readings[numReadings] = {0};
  int readIndex = 0;

  int entryCounter = 0;
  int exitCounter = 0;

  float peakDistance = 0;
  float triggerDistance = -150;
  float lastDistance = 0;

  unsigned long cycleStart = 0;
  bool readingPhase = true;

  float entryDistance = 0;
  unsigned long entryTime = 0;
  float totalSpeed = 0;
  int speedCount = 0;
};

void UltrasonicSensor(UltrasonicState &state, unsigned long readDuration, int trigPin, int echoPin) {
  const int debounceCount = 2;
  const float riseThreshold = 10;
  const unsigned long pauseDuration = 10000;

  if (!state.initialized) {
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    state.cycleStart = millis();
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

  if (state.readingPhase) {
    float avg = getAverageDistance();

    if (avg > state.peakDistance) {
      state.peakDistance = avg;
      state.triggerDistance = state.peakDistance - 150;
    }

    Serial.print("Distance: "); Serial.print(avg);
    Serial.print("  Count: "); Serial.println(state.vehicleCount);

    if (avg < state.triggerDistance && !state.carPresence) {
      state.entryCounter++;
      if (state.entryCounter >= debounceCount) {
        state.carPresence = true;
        state.entryCounter = 0;
        state.entryDistance = avg;
        state.entryTime = now;
        Serial.println("Car entered");
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
        Serial.println("Car left");
      }
    } else state.exitCounter = 0;

    state.lastDistance = avg;

    if (now - state.cycleStart >= readDuration) {
      float avgSpeed = (state.speedCount > 0) ? state.totalSpeed / state.speedCount : 0;
      float flow = state.vehicleCount / (readDuration / 1000.0);
      Serial.print("Final count: "); Serial.println(state.vehicleCount);
      Serial.print("Average speed (m/s): "); Serial.println(avgSpeed);
      Serial.print("Flow (cars/s): "); Serial.println(flow);

      state.vehicleCount = 0;
      state.totalSpeed = 0;
      state.speedCount = 0;
      state.readingPhase = false;
      state.cycleStart = now;
    }

  } else {
    if (now - state.cycleStart >= pauseDuration) {
      state.readingPhase = true;
      state.cycleStart = now;
    }
  }

  delay(100);
}

// usage
UltrasonicState sensor1, sensor2;

void setup() {
  Serial.begin(115200);
}

void loop() {
  UltrasonicSensor(sensor1, 30000, 5, 32);
  UltrasonicSensor(sensor2, 30000, 23, 18);
}
