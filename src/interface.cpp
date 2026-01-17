// interface.cpp
#include <Arduino.h>
#include <LittleFS.h>
#include "interface.h"

// Toggle this to 1 for Wokwi/local simulation (no WiFi/Firebase).
// Set to 0 to enable real WiFi + Firebase REST PUTs (HTTPClient).
#define SIMULATE 1

#if SIMULATE == 0
  #include <WiFi.h>
  #include <HTTPClient.h>
#endif

#include <WebServer.h>

// ----------------- CONFIG (edit when SIMULATE==0) -----------------
#if SIMULATE == 0
  static const char* WIFI_SSID = "YOUR_WIFI_SSID";
  static const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";
  // No trailing slash. Example:
  // https://traffic-system-dashboard-default-rtdb.firebaseio.com
  static const char* FIREBASE_BASE = "https://traffic-system-dashboard-default-rtdb.firebaseio.com";
#endif
// -----------------------------------------------------------------

// Externs from your other modules
extern unsigned long greenA;
extern unsigned long greenB;
extern unsigned long greenC;
extern bool allRedLatched;

// Small public copy of lane state for web API / simulation
struct PublicLane {
  int count = 0;
  float flow = 0.0f;
  float avgSpeed = 0.0f;
  unsigned long greenTime = 0;
  String status = "red";
};

static PublicLane laneA_public, laneB_public, laneC_public;

// Web server (works in Wokwi)
static WebServer server(80);

// Simple uptime timestamp (no RTC) for JSON
String getIsoTimestamp() {
  // returns seconds since boot as string (simple, monotonic)
  unsigned long s = millis() / 1000;
  return String(s);
}

// ---------- Helpers (Firebase stub or real PUT) ----------
#if SIMULATE == 0
// Perform HTTP PUT to Firebase REST endpoint (path like "lanes/laneA")
static bool firebasePut(const String &path, const String &jsonPayload) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("firebasePut: WiFi not connected");
    return false;
  }
  HTTPClient http;
  String url = String(FIREBASE_BASE) + "/" + path + ".json";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  int code = http.PUT(jsonPayload);
  String resp = http.getString();
  http.end();
  if (code >= 200 && code < 300) {
    return true;
  } else {
    Serial.printf("firebasePut failed: code=%d resp=%s\n", code, resp.c_str());
    return false;
  }
}
#else
// Simulation: just print, don't network
static bool firebasePut(const String &path, const String &jsonPayload) {
  Serial.println("SIMULATED firebasePut -> " + path);
  Serial.println(jsonPayload);
  return true;
}
#endif

// POST for logs (optional)
#if SIMULATE == 0
static bool firebasePost(const String &path, const String &jsonPayload) {
  if (WiFi.status() != WL_CONNECTED) return false;
  HTTPClient http;
  String url = String(FIREBASE_BASE) + "/" + path + ".json";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(jsonPayload);
  String resp = http.getString();
  http.end();
  return (code >= 200 && code < 300);
}
#else
static bool firebasePost(const String &path, const String &jsonPayload) {
  Serial.println("SIMULATED firebasePost -> " + path);
  Serial.println(jsonPayload);
  return true;
}
#endif

// ---------------- Public API (called by your trafficController / main) ----------------
void connectWiFi() {
#if SIMULATE == 0
  Serial.printf("Connecting to WiFi SSID=%s\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(250);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nWiFi connect failed/time out.");
  }
#else
  Serial.println("SIMULATION MODE - skipping WiFi connect");
#endif
}

void startWebServer() {
  // mount LittleFS if not mounted
  if (!LittleFS.begin()) {
    Serial.println("LittleFS not mounted in startWebServer!");
    // still continue; some tests may not need FS
  }

  // root serves index.html if exists
  server.on("/", HTTP_GET, []() {
    if (LittleFS.exists("/index.html")) {
      File f = LittleFS.open("/index.html", "r");
      if (f) {
        String content;
        while (f.available()) content += char(f.read());
        f.close();
        server.send(200, "text/html", content);
        return;
      }
    }
    server.send(200, "text/plain", "Traffic controller (simulated)");
  });

  // API for status used by dashboard (same shape as we described earlier)
  server.on("/api/status", HTTP_GET, []() {
    String j = "{";
    j += "\"status\":{";
    j += "\"greenA\":" + String(greenA) + ",";
    j += "\"greenB\":" + String(greenB) + ",";
    j += "\"greenC\":" + String(greenC) + ",";
    j += "\"allRed\":" + String(allRedLatched ? "true" : "false");
    j += "},";
    auto laneJson = [](const PublicLane &L) {
      String s = "{";
      s += "\"count\":" + String(L.count) + ",";
      s += "\"flow\":" + String(L.flow, 4) + ",";
      s += "\"avgSpeed\":" + String(L.avgSpeed, 4) + ",";
      s += "\"greenTime\":" + String(L.greenTime) + ",";
      s += "\"status\":\"" + L.status + "\"";
      s += "}";
      return s;
    };
    j += "\"lanes\":{";
    j += "\"laneA\":" + laneJson(laneA_public) + ",";
    j += "\"laneB\":" + laneJson(laneB_public) + ",";
    j += "\"laneC\":" + laneJson(laneC_public);
    j += "}}";
    server.send(200, "application/json", j);
  });

  // optional toggle (useful during demos)
  server.on("/api/toggleAllRed", HTTP_POST, []() {
    allRedLatched = !allRedLatched;
    server.send(200, "application/json", "{\"allRed\":" + String(allRedLatched ? "true" : "false") + "}");
  });

  server.begin();
  Serial.println("Web server started (port 80)");
}

void handleWebServer() {
  server.handleClient();
}

void pushLog(const String &msg) {
  // push to /logs as simple JSON with timestamp
  String payload = "{";
  payload += "\"msg\":\"" + msg + "\",";
  payload += "\"ts\":\"" + getIsoTimestamp() + "\"";
  payload += "}";
  if (!firebasePost("logs", payload)) {
    Serial.println("pushLog: failed (or simulated)");
  }
}

// Update traffic-wide status node (currentStep + green times)
void updateTrafficStatus(int currentStep) {
  String json = "{";
  json += "\"currentStep\":" + String(currentStep) + ",";
  json += "\"greenA\":" + String(greenA) + ",";
  json += "\"greenB\":" + String(greenB) + ",";
  json += "\"greenC\":" + String(greenC) + ",";
  json += "\"allRed\":" + String(allRedLatched ? "true" : "false") + ",";
  json += "\"ts\":\"" + getIsoTimestamp() + "\"";
  json += "}";
  // Put to /status
  if (!firebasePut("status", json)) {
    // in SIMULATE mode this will succeed as a print
    Serial.println("updateTrafficStatus: firebasePut failed or simulated");
  }

  // also update public lane greenTimes for /api/status
  laneA_public.greenTime = greenA;
  laneB_public.greenTime = greenB;
  laneC_public.greenTime = greenC;
}

// Called by trafficController to push lane stats
void updateLaneData(char lane, int count, float flow, float avgSpeed) {
  // Update local public copies (so web GET /api/status shows latest)
  if (lane == 'A' || lane == 'a') {
    laneA_public.count = count;
    laneA_public.flow = flow;
    laneA_public.avgSpeed = avgSpeed;
    laneA_public.greenTime = greenA;
    // status can be set here if you want depending on currentStep
  } else if (lane == 'B' || lane == 'b') {
    laneB_public.count = count;
    laneB_public.flow = flow;
    laneB_public.avgSpeed = avgSpeed;
    laneB_public.greenTime = greenB;
  } else if (lane == 'C' || lane == 'c') {
    laneC_public.count = count;
    laneC_public.flow = flow;
    laneC_public.avgSpeed = avgSpeed;
    laneC_public.greenTime = greenC;
  } else {
    Serial.println("updateLaneData: invalid lane char");
    return;
  }

  // Build JSON to push to Firebase (or simulate)
  String json = "{";
  json += "\"count\":" + String(count) + ",";
  json += "\"flow\":" + String(flow, 4) + ",";
  json += "\"avgSpeed\":" + String(avgSpeed, 4) + ",";
  json += "\"greenTime\":" + String((lane=='A' || lane=='a') ? greenA : lane=='B' || lane=='b' ? greenB : greenC) + ",";
  // include status if you set it; default to unknown here
  String statusStr = "unknown";
  json += "\"status\":\"" + statusStr + "\",";
  json += "\"updatedAt\":\"" + getIsoTimestamp() + "\"";
  json += "}";

  String path;
  if (lane == 'A' || lane == 'a') path = "lanes/laneA";
  else if (lane == 'B' || lane == 'b') path = "lanes/laneB";
  else path = "lanes/laneC";

  if (!firebasePut(path, json)) {
    Serial.println("updateLaneData: firebasePut failed (or simulated)");
  } else {
    Serial.printf("updateLaneData: pushed %s\n", path.c_str());
  }
}
