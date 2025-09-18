#include "TrafficLight.h"

// Pin definitions
const int A_R = 12, A_Y = 13, A_G = 14;
const int B_R = 15, B_Y = 21, B_G = 22;
const int C_R = 19, C_Y = 25, C_G = 26;

// Lane stats and sensors
LaneStats laneA, laneB, laneC;
UltrasonicState sensor1, sensor2, sensor3;

void trafficController(unsigned long gA, unsigned long gB, unsigned long gC, unsigned long ov, int &currentStep, unsigned long &prevMillis,
                       unsigned long &greenA, unsigned long &greenB, unsigned long &greenC,
                       float Kp, float s_target, float deltamax,
                       unsigned long minGreen, unsigned long maxGreen) {
  unsigned long now = millis();
  unsigned long durations[6] = {gA * 1000, ov * 1000, gB * 1000, ov * 1000, gC * 1000, ov * 1000};

  if (now - prevMillis >= durations[currentStep]) {
    prevMillis = now;

    // End-of-phase updates
    if (currentStep == 0) {
      laneA.count = sensor1.vehicleCount;
      laneA.flow = sensor1.vehicleCount / (float)greenA;
      laneA.avgSpeed = (sensor1.speedCount > 0) ? sensor1.totalSpeed / sensor1.speedCount : 0;
      laneA.update(laneA.flow, laneA.avgSpeed);

      unsigned long oldGreen = greenA;
      greenA = adjustGreen(greenA, laneA.flow, laneA.avgSpeed, Kp, s_target, deltamax, minGreen, maxGreen);

      Serial.print("Lane A | Count: "); Serial.print(laneA.count);
      Serial.print(" | Flow: "); Serial.print(laneA.flow, 2);
      Serial.print(" | AvgSpeed: "); Serial.print(laneA.avgSpeed, 2);
      Serial.print(" | Green: "); Serial.print(oldGreen);
      Serial.print("s -> "); Serial.print(greenA); Serial.println("s");
    }
    if (currentStep == 2) {
      laneB.count = sensor2.vehicleCount;
      laneB.flow = sensor2.vehicleCount / (float)greenB;
      laneB.avgSpeed = (sensor2.speedCount > 0) ? sensor2.totalSpeed / sensor2.speedCount : 0;
      laneB.update(laneB.flow, laneB.avgSpeed);

      unsigned long oldGreen = greenB;
      greenB = adjustGreen(greenB, laneB.flow, laneB.avgSpeed, Kp, s_target, deltamax, minGreen, maxGreen);

      Serial.print("Lane B | Count: "); Serial.print(laneB.count);
      Serial.print(" | Flow: "); Serial.print(laneB.flow, 2);
      Serial.print(" | AvgSpeed: "); Serial.print(laneB.avgSpeed, 2);
      Serial.print(" | Green: "); Serial.print(oldGreen);
      Serial.print("s -> "); Serial.print(greenB); Serial.println("s");
    }
    if (currentStep == 4) {
      laneC.count = sensor3.vehicleCount;
      laneC.flow = sensor3.vehicleCount / (float)greenC;
      laneC.avgSpeed = (sensor3.speedCount > 0) ? sensor3.totalSpeed / sensor3.speedCount : 0;
      laneC.update(laneC.flow, laneC.avgSpeed);

      unsigned long oldGreen = greenC;
      greenC = adjustGreen(greenC, laneC.flow, laneC.avgSpeed, Kp, s_target, deltamax, minGreen, maxGreen);

      Serial.print("Lane C | Count: "); Serial.print(laneC.count);
      Serial.print(" | Flow: "); Serial.print(laneC.flow, 2);
      Serial.print(" | AvgSpeed: "); Serial.print(laneC.avgSpeed, 2);
      Serial.print(" | Green: "); Serial.print(oldGreen);
      Serial.print("s -> "); Serial.print(greenC); Serial.println("s");
    }

    currentStep = (currentStep + 1) % 6;
  }

  // Reset all LEDs
  digitalWrite(A_R, LOW); digitalWrite(A_Y, LOW); digitalWrite(A_G, LOW);
  digitalWrite(B_R, LOW); digitalWrite(B_Y, LOW); digitalWrite(B_G, LOW);
  digitalWrite(C_R, LOW); digitalWrite(C_Y, LOW); digitalWrite(C_G, LOW);

  // Apply current step
  switch(currentStep) {
    case 0: digitalWrite(A_G,HIGH); digitalWrite(B_R,HIGH); digitalWrite(C_R,HIGH); break;
    case 1: digitalWrite(A_Y,HIGH); digitalWrite(B_Y,HIGH); digitalWrite(C_R,HIGH); break;
    case 2: digitalWrite(B_G,HIGH); digitalWrite(A_R,HIGH); digitalWrite(C_R,HIGH); break;
    case 3: digitalWrite(B_Y,HIGH); digitalWrite(C_Y,HIGH); digitalWrite(A_R,HIGH); break;
    case 4: digitalWrite(C_G,HIGH); digitalWrite(A_R,HIGH); digitalWrite(B_R,HIGH); break;
    case 5: digitalWrite(C_Y,HIGH); digitalWrite(A_Y,HIGH); digitalWrite(B_R,HIGH); break;
  }
}
