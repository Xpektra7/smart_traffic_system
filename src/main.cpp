#include <Arduino.h>

const int trigPin = 5;
const int echoPin = 32;
bool carPresence = false;
int vehicleCount = 0;

const int numReadings = 3;  // rolling average
float readings[numReadings];
int readIndex = 0;

const int debounceCount = 2;
int entryCounter = 0;
int exitCounter = 0;

float peakDistance = 0;
float triggerDistance = peakDistance - 150;
float lastDistance = 0;
const float riseThreshold = 10;

unsigned long cycleStart = 0;
const unsigned long readDuration = 30000;  // 30s
const unsigned long pauseDuration = 10000; // 10s
bool readingPhase = true;

// --- speed tracking ---
float entryDistance = 0;
unsigned long entryTime = 0;
float totalSpeed = 0; // sum of speeds for average
int speedCount = 0;

void setup() {
  Serial.begin(115200);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  for (int i = 0; i < numReadings; i++) readings[i] = 0;
  cycleStart = millis();
}

float getDistance() {
  long duration;
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  float distance = (duration * 0.0343) / 2;
  if (distance > 550) distance = 550;
  return distance;
}

float getAverageDistance() {
  readings[readIndex] = getDistance();
  readIndex = (readIndex + 1) % numReadings;

  float sum = 0;
  for (int i = 0; i < numReadings; i++) sum += readings[i];
  return sum / numReadings;
}

void loop() {
  unsigned long now = millis();

  if (readingPhase) {
    float avgDistance = getAverageDistance();

    if (avgDistance > peakDistance) {
      peakDistance = avgDistance;
      triggerDistance = peakDistance - 150;
    }

    Serial.print("Distance: "); Serial.print(avgDistance);
    Serial.print("  Count: "); Serial.println(vehicleCount);

    // --- entry detection ---
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

    // --- exit detection ---
    if (carPresence && avgDistance > lastDistance + riseThreshold) {
      exitCounter++;
      if (exitCounter >= debounceCount) {
        carPresence = false;
        float travelDistance = avgDistance - entryDistance; // cm
        float travelTime = (now - entryTime) / 1000.0;      // s
        float speed = (travelDistance / 100.0) / travelTime; // m/s
        totalSpeed += speed;
        speedCount++;

        vehicleCount++;
        exitCounter = 0;
        Serial.println("Car left");
      }
    } else exitCounter = 0;

    lastDistance = avgDistance;

    // --- end of 30s window ---
    if (now - cycleStart >= readDuration) {
      float avgSpeed = (speedCount > 0) ? totalSpeed / speedCount : 0;
      float flow = vehicleCount / (readDuration / 1000.0); // cars per s

      Serial.print("Final count: "); Serial.println(vehicleCount);
      Serial.print("Average speed (m/s): "); Serial.println(avgSpeed);
      Serial.print("Flow (cars/s): "); Serial.println(flow);

      // reset for next cycle
      vehicleCount = 0;
      totalSpeed = 0;
      speedCount = 0;
      readingPhase = false;
      cycleStart = now;
    }

  } else { // pause phase
    if (now - cycleStart >= pauseDuration) {
      readingPhase = true;
      cycleStart = now;
    }
  }

  delay(100);
}
