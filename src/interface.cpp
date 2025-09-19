#include "interface.h"

const char* WIFI_SSID     = "Xpektra";
const char* WIFI_PASSWORD = "se7777en";
const char* API_URL       = "http://your-api-url.com";  // change if you set up backend

void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());
}

void sendTrafficStatus(int currentStep) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(API_URL) + "/traffic/status";
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    String payload = "{\"currentStep\": " + String(currentStep) + "}";
    int httpCode = http.POST(payload);

    Serial.println("Traffic Status Response: " + String(httpCode));
    http.end();
  }
}

void sendSensorData(int vehicleCount, float avgSpeed, float flowRate) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(API_URL) + "/traffic/sensors";
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    String payload = "{";
    payload += "\"vehicleCount\": " + String(vehicleCount) + ",";
    payload += "\"avgSpeed\": " + String(avgSpeed, 2) + ",";
    payload += "\"flowRate\": " + String(flowRate, 2);
    payload += "}";

    int httpCode = http.POST(payload);

    Serial.println("Sensor Data Response: " + String(httpCode));
    http.end();
  }
}

void pushLog(const String &message) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(API_URL) + "/logs";
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    String payload = "{\"timestamp\": " + String(millis()) +
                     ", \"message\": \"" + message + "\"}";

    int httpCode = http.POST(payload);

    Serial.println("Log Response: " + String(httpCode));
    http.end();
  }
}
