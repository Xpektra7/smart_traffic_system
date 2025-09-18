#pragma once
#include <Arduino.h>
#include "Ultrasonic.h"
#include "Adaptive.h"

// Pins
extern const int A_R, A_Y, A_G;
extern const int B_R, B_Y, B_G;
extern const int C_R, C_Y, C_G;

// Lane stats
struct LaneStats {
  int count;
  float flow;
  float avgSpeed;
  float flowEMA = 0;
  float speedEMA = 0;
  const float alpha = 0.3;

  void update(float newFlow, float newSpeed) {
    flowEMA = alpha * newFlow + (1 - alpha) * flowEMA;
    speedEMA = alpha * newSpeed + (1 - alpha) * speedEMA;
  }
};

extern LaneStats laneA, laneB, laneC;
extern UltrasonicState sensor1, sensor2, sensor3;

// Traffic functions
void trafficController(unsigned long gA, unsigned long gB, unsigned long gC, unsigned long ov, int &currentStep, unsigned long &prevMillis,
                       unsigned long &greenA, unsigned long &greenB, unsigned long &greenC,
                       float Kp, float s_target, float deltamax,
                       unsigned long minGreen, unsigned long maxGreen);
