#include <Arduino.h>
#include "TrafficLight.h"
#include "Ultrasonic.h"

unsigned long prevMillis = 0;
int currentStep = 0;

unsigned long greenA = 20, greenB = 20, greenC = 20;
unsigned long overlap = 5;

// Adaptive parameters
const float Kp = 20; 
const float s_target = 0.05; //0.2+ for real-life
const float deltamax = 5;
const unsigned long minGreen = 5, maxGreen = 60; // 20 , 60 for real life

void setup() {
  Serial.begin(115200);
  pinMode(A_R, OUTPUT); pinMode(A_Y, OUTPUT); pinMode(A_G, OUTPUT);
  pinMode(B_R, OUTPUT); pinMode(B_Y, OUTPUT); pinMode(B_G, OUTPUT);
  pinMode(C_R, OUTPUT); pinMode(C_Y, OUTPUT); pinMode(C_G, OUTPUT);
  digitalWrite(A_R, HIGH); digitalWrite(B_R, HIGH); digitalWrite(C_R, HIGH);
}

void loop() {
  trafficController(greenA, greenB, greenC, overlap, currentStep, prevMillis, greenA, greenB, greenC, Kp, s_target, deltamax, minGreen, maxGreen);

  if (currentStep == 0) UltrasonicSensor("U1", sensor1, greenA*1000, 5, 32);
  if (currentStep == 2) UltrasonicSensor("U2", sensor2, greenB*1000, 23, 18);
  if (currentStep == 4) UltrasonicSensor("U3", sensor3, greenC*1000, 27, 34);
}
