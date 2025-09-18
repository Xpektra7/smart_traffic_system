#include <Arduino.h>

// --- Lane stats ---
struct LaneStats {
  int count;
  float flow;
  float avgSpeed;
};
LaneStats laneA, laneB, laneC;

// --- Ultrasonic states ---
#include "Ultrasonic.h"   // assume your UltrasonicState + UltrasonicSensor() live here
UltrasonicState sensor1, sensor2, sensor3;

// --- Traffic light pins ---
const int A_R = 12, A_Y = 13, A_G = 14;
const int B_R = 15, B_Y = 21, B_G = 22;
const int C_R = 19, C_Y = 25, C_G = 26;

// --- Traffic state ---
unsigned long prevMillis = 0;
int currentStep = 0;

// --- Timing defaults (s) ---
unsigned long greenA = 20, greenB = 20, greenC = 20;
unsigned long overlap = 5;

// --- Adaptive params ---
const float Kp = 5;        // proportional gain
const float s_target = 0.2;
const float Δmax = 5;      // max change per cycle (s)
const unsigned long minGreen = 10, maxGreen = 60;

// --- Adaptive adjust ---
unsigned long adjustGreen(unsigned long currentGreen, float flow, float speed) {
  float s = flow / (speed > 0 ? speed : 1);
  float delta = Kp * (s - s_target);
  if (delta > Δmax) delta = Δmax;
  if (delta < -Δmax) delta = -Δmax;
  unsigned long newGreen = constrain(currentGreen + (long)delta, minGreen, maxGreen);
  return newGreen;
}

// --- Light state machine ---
void trafficController(unsigned long gA, unsigned long gB, unsigned long gC, unsigned long ov) {
  unsigned long now = millis();
  unsigned long durations[6] = {
    gA * 1000, ov * 1000, gB * 1000,
    ov * 1000, gC * 1000, ov * 1000
  };

  if (now - prevMillis >= durations[currentStep]) {
    prevMillis = now;

    // --- End-of-phase actions ---
    if (currentStep == 0) { // A green ended
      laneA.count = sensor1.vehicleCount;
      laneA.flow = sensor1.vehicleCount / (float)greenA;
      laneA.avgSpeed = (sensor1.speedCount > 0) ? sensor1.totalSpeed / sensor1.speedCount : 0;
      greenA = adjustGreen(greenA, laneA.flow, laneA.avgSpeed);
    }
    if (currentStep == 2) { // B green ended
      laneB.count = sensor2.vehicleCount;
      laneB.flow = sensor2.vehicleCount / (float)greenB;
      laneB.avgSpeed = (sensor2.speedCount > 0) ? sensor2.totalSpeed / sensor2.speedCount : 0;
      greenB = adjustGreen(greenB, laneB.flow, laneB.avgSpeed);
    }
    if (currentStep == 4) { // C green ended
      laneC.count = sensor3.vehicleCount;
      laneC.flow = sensor3.vehicleCount / (float)greenC;
      laneC.avgSpeed = (sensor3.speedCount > 0) ? sensor3.totalSpeed / sensor3.speedCount : 0;
      greenC = adjustGreen(greenC, laneC.flow, laneC.avgSpeed);
    }

    currentStep = (currentStep + 1) % 6;
  }

  // --- Reset all LEDs ---
  digitalWrite(A_R, LOW); digitalWrite(A_Y, LOW); digitalWrite(A_G, LOW);
  digitalWrite(B_R, LOW); digitalWrite(B_Y, LOW); digitalWrite(B_G, LOW);
  digitalWrite(C_R, LOW); digitalWrite(C_Y, LOW); digitalWrite(C_G, LOW);

  // --- Apply current step ---
  switch (currentStep) {
    case 0: digitalWrite(A_G, HIGH); digitalWrite(B_R, HIGH); digitalWrite(C_R, HIGH); break;
    case 1: digitalWrite(A_Y, HIGH); digitalWrite(B_Y, HIGH); digitalWrite(C_R, HIGH); break;
    case 2: digitalWrite(B_G, HIGH); digitalWrite(A_R, HIGH); digitalWrite(C_R, HIGH); break;
    case 3: digitalWrite(B_Y, HIGH); digitalWrite(C_Y, HIGH); digitalWrite(A_R, HIGH); break;
    case 4: digitalWrite(C_G, HIGH); digitalWrite(A_R, HIGH); digitalWrite(B_R, HIGH); break;
    case 5: digitalWrite(C_Y, HIGH); digitalWrite(A_Y, HIGH); digitalWrite(B_R, HIGH); break;
  }
}

// --- Setup ---
void setup() {
  Serial.begin(115200);
  pinMode(A_R, OUTPUT); pinMode(A_Y, OUTPUT); pinMode(A_G, OUTPUT);
  pinMode(B_R, OUTPUT); pinMode(B_Y, OUTPUT); pinMode(B_G, OUTPUT);
  pinMode(C_R, OUTPUT); pinMode(C_Y, OUTPUT); pinMode(C_G, OUTPUT);
  digitalWrite(A_R, HIGH); digitalWrite(B_R, HIGH); digitalWrite(C_R, HIGH);
}

// --- Loop ---
void loop() {
  trafficController(greenA, greenB, greenC, overlap);

  // Run ultrasonic only during green phases
  if (currentStep == 0) UltrasonicSensor("U1", sensor1, greenA*1000, 5, 32);
  if (currentStep == 2) UltrasonicSensor("U2", sensor2, greenB*1000, 23, 18);
  if (currentStep == 4) UltrasonicSensor("U3", sensor3, greenC*1000, 27, 34);
}
