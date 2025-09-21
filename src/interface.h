#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>

// Externs for lane data
extern unsigned long greenA, greenB, greenC;
extern int sensor1_count, sensor2_count, sensor3_count;
extern float sensor1_flow, sensor2_flow, sensor3_flow;
extern float sensor1_speed, sensor2_speed, sensor3_speed;

// Functions
void connectWiFi();
void startWebServer();
void handleWebServer();
void updateTrafficStatus(int step);
void updateSensorData(int count, float speed, float flow);
void pushLog(const String &message);
void updateLaneData(char lane, int count, float flow, float speed);
void allRed();
