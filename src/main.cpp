#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include "TrafficLight.h"
#include "Ultrasonic.h"
#include "interface.h"

unsigned long prevMillis = 0;
unsigned long lastUpdate = 0;   // for web refresh
int currentStep = 0;

unsigned long greenA = 20, greenB = 20, greenC = 20;
unsigned long overlap = 5;

// Adaptive parameters
const float Kp = 20; 
const float s_target = 0.05;
const float deltamax = 5;
const unsigned long minGreen = 5, maxGreen = 60;

void setup() {
  Serial.begin(115200);

  // Mount LittleFS
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed!");
    return;
  }
  Serial.println("LittleFS mounted.");

  // Connect WiFi + start server
  connectWiFi();
  startWebServer();

  // Traffic light pins
  pinMode(A_R, OUTPUT); pinMode(A_Y, OUTPUT); pinMode(A_G, OUTPUT);
  pinMode(B_R, OUTPUT); pinMode(B_Y, OUTPUT); pinMode(B_G, OUTPUT);
  pinMode(C_R, OUTPUT); pinMode(C_Y, OUTPUT); pinMode(C_G, OUTPUT);
  digitalWrite(A_R, HIGH); digitalWrite(B_R, HIGH); digitalWrite(C_R, HIGH);
}

void loop() {
  if (allRedLatched) {
    return;  // skip normal light sequencing
  }
  // Handle light control
  trafficController(greenA, greenB, greenC, overlap,
                    currentStep, prevMillis,
                    greenA, greenB, greenC,
                    Kp, s_target, deltamax, minGreen, maxGreen);

  // Sensor logic per active step
  if (currentStep == 0) UltrasonicSensor("U1", sensor1, greenA*1000, 5, 32);
  if (currentStep == 2) UltrasonicSensor("U2", sensor2, greenB*1000, 23, 18);
  if (currentStep == 4) UltrasonicSensor("U3", sensor3, greenC*1000, 27, 34);

  // Web server always listening
  handleWebServer();

  // Update status every 100 ms without blocking
  unsigned long now = millis();
  if (now - lastUpdate >= 100) {
    lastUpdate = now;
    updateTrafficStatus(currentStep);
    pushLog("Step: " + String(currentStep));
    // If you want global stats:
  }


}
