#pragma once

#include <Arduino.h>

// Call from main setup/loop
void connectWiFi();
void startWebServer();
void handleWebServer();

// Called from main loop periodically / from trafficController
void updateTrafficStatus(int currentStep);
void pushLog(const String &msg);

// Called from trafficController to push per-lane stats
void updateLaneData(char lane, int count, float flow, float avgSpeed);

// Optional helper you can call from Serial or elsewhere
String getIsoTimestamp();
