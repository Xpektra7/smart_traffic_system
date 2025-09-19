#include "TrafficLight.h"

// Pin definitions
const int A_R = 12, A_Y = 13, A_G = 14;
const int B_R = 15, B_Y = 21, B_G = 22;
const int C_R = 19, C_Y = 25, C_G = 26;

// Lane stats and sensors
LaneStats laneA, laneB, laneC;
UltrasonicState sensor1, sensor2, sensor3;

void trafficController(unsigned long gA, unsigned long gB, unsigned long gC, unsigned long ov, 
                       int &currentStep, unsigned long &prevMillis,
                       unsigned long &greenA, unsigned long &greenB, unsigned long &greenC,
                       float Kp, float s_target, float deltamax,
                       unsigned long minGreen, unsigned long maxGreen) {
  unsigned long now = millis();
unsigned long stepDuration = 0;
  switch (currentStep) {
    case 0: stepDuration = greenA * 1000; break; // Lane A green
    case 1: stepDuration = ov * 1000;     break; // A->B overlap
    case 2: stepDuration = greenB * 1000; break; // Lane B green
    case 3: stepDuration = ov * 1000;     break; // B->C overlap
    case 4: stepDuration = greenC * 1000; break; // Lane C green
    case 5: stepDuration = ov * 1000;     break; // C->A overlap
  }

  // --- EMA memory (persist between calls) ---
  static bool emaInit = false;
  static float emaA, emaB, emaC;

  if (now - prevMillis >= stepDuration) {
    prevMillis = now;

    // Collect stats at end of each green
    if (currentStep == 0) {
      laneA.count = sensor1.vehicleCount;
      laneA.flow = sensor1.vehicleCount / (float)greenA;
      laneA.avgSpeed = (sensor1.speedCount > 0) ? sensor1.totalSpeed / sensor1.speedCount : 0;
      laneA.update(laneA.flow, laneA.avgSpeed);
    }
    if (currentStep == 2) {
      laneB.count = sensor2.vehicleCount;
      laneB.flow = sensor2.vehicleCount / (float)greenB;
      laneB.avgSpeed = (sensor2.speedCount > 0) ? sensor2.totalSpeed / sensor2.speedCount : 0;
      laneB.update(laneB.flow, laneB.avgSpeed);
    }
    if (currentStep == 4) {
      laneC.count = sensor3.vehicleCount;
      laneC.flow = sensor3.vehicleCount / (float)greenC;
      laneC.avgSpeed = (sensor3.speedCount > 0) ? sensor3.totalSpeed / sensor3.speedCount : 0;
      laneC.update(laneC.flow, laneC.avgSpeed);
    }

    // End of cycle: after step 5
    if (currentStep == 5) {
      unsigned long oldA = greenA, oldB = greenB, oldC = greenC;

      // --- Step 1: compute demand scores ---
      float demandA = laneA.flow;
      float demandB = laneB.flow;
      float demandC = laneC.flow;

      float totalDemand = demandA + demandB + demandC;
      if (totalDemand < 0.001) totalDemand = 0.001;

      // --- Step 2: compute available green budget ---
      unsigned long overlapTotal = ov * 3;
      unsigned long greenBudget = (90 - overlapTotal);

      // --- Step 3: raw proportional allocation ---
      float sA = (float)greenBudget * (demandA / totalDemand);
      float sB = (float)greenBudget * (demandB / totalDemand);
      float sC = (float)greenBudget * (demandC / totalDemand);

      // --- Step 3.5: EMA smoothing ---
      const float alpha = 0.3;
      if (!emaInit) {
        emaA = sA; emaB = sB; emaC = sC;
        emaInit = true;
      } else {
        emaA = alpha * sA + (1 - alpha) * emaA;
        emaB = alpha * sB + (1 - alpha) * emaB;
        emaC = alpha * sC + (1 - alpha) * emaC;
      }

      greenA = (unsigned long)(emaA + 0.5f);
      greenB = (unsigned long)(emaB + 0.5f);
      greenC = (unsigned long)(emaC + 0.5f);

      // --- Step 4: bounding + redistribution ---
      bool changed;
      do {
        changed = false;
        unsigned long totalAssigned = 0;
        int freeLanes = 0;

        if (greenA < minGreen) { greenA = minGreen; changed = true; }
        else if (greenA > maxGreen) { greenA = maxGreen; changed = true; }
        if (greenB < minGreen) { greenB = minGreen; changed = true; }
        else if (greenB > maxGreen) { greenB = maxGreen; changed = true; }
        if (greenC < minGreen) { greenC = minGreen; changed = true; }
        else if (greenC > maxGreen) { greenC = maxGreen; changed = true; }

        totalAssigned = greenA + greenB + greenC;
        if (greenA > minGreen && greenA < maxGreen) freeLanes++;
        if (greenB > minGreen && greenB < maxGreen) freeLanes++;
        if (greenC > minGreen && greenC < maxGreen) freeLanes++;

        long diff = (long)greenBudget - (long)totalAssigned;
        if (diff != 0 && freeLanes > 0) {
          long share = diff / freeLanes;
          if (greenA > minGreen && greenA < maxGreen) greenA += share;
          if (greenB > minGreen && greenB < maxGreen) greenB += share;
          if (greenC > minGreen && greenC < maxGreen) greenC += share;
          changed = true;
        }
      } while (changed);

      greenA = constrain(greenA, minGreen, maxGreen);
      greenB = constrain(greenB, minGreen, maxGreen);
      greenC = constrain(greenC, minGreen, maxGreen);

      // --- Step 5: logging ---
      Serial.println("=== End of Cycle (Fixed 90s + EMA) Redistribution ===");
      Serial.print("Lane A | Avg Speed "); Serial.print(laneA.avgSpeed);
      Serial.print(" | Flow "); Serial.print(demandA, 2);
      Serial.print(" | Green: "); Serial.print(oldA); Serial.print("s -> "); Serial.print(greenA); Serial.println("s");

      Serial.print("Lane B | Avg Speed "); Serial.print(laneB.avgSpeed);
      Serial.print(" | Flow "); Serial.print(demandB, 2);
      Serial.print(" | Green: "); Serial.print(oldB); Serial.print("s -> "); Serial.print(greenB); Serial.println("s");

      Serial.print("Lane C | Avg Speed "); Serial.print(laneC.avgSpeed);
      Serial.print(" | Flow "); Serial.print(demandC, 2);
      Serial.print(" | Green: "); Serial.print(oldC); Serial.print("s -> "); Serial.print(greenC); Serial.println("s");

      Serial.println("===============================================");
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
