#pragma once
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

  unsigned long lastSample = 0;
};

void UltrasonicSensor(const char* name, UltrasonicState &state, unsigned long readDuration, int trigPin, int echoPin);
