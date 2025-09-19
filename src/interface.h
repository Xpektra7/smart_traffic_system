#ifndef INTERFACE_H
#define INTERFACE_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

// WiFi credentials
extern const char* WIFI_SSID;
extern const char* WIFI_PASSWORD;

// Backend API URL (replace with real URL if you have one)
extern const char* API_URL;

// Functions
void connectWiFi();
void sendTrafficStatus(int currentStep);
void sendSensorData(int vehicleCount, float avgSpeed, float flowRate);
void pushLog(const String &message);

#endif
