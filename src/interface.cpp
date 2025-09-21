#include "interface.h"

const char* WIFI_SSID     = "Xpektra";
const char* WIFI_PASSWORD = "se8888en";

WebServer server(80);

// 🔹 Shared state definitions
int g_currentStep = 0;
int g_vehicleCount = 0;
float g_avgSpeed = 0.0;
float g_flowRate = 0.0;
String g_log = "";

int laneA_count = 0, laneB_count = 0, laneC_count = 0;
float laneA_flow = 0, laneB_flow = 0, laneC_flow = 0;
float laneA_speed = 0, laneB_speed = 0, laneC_speed = 0;

void updateLaneData(char lane, int count, float flow, float speed) {
  switch (lane) {
    case 'A': laneA_count = count; laneA_flow = flow; laneA_speed = speed; break;
    case 'B': laneB_count = count; laneB_flow = flow; laneB_speed = speed; break;
    case 'C': laneC_count = count; laneC_flow = flow; laneC_speed = speed; break;
  }
}

void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");

  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    Serial.print(".");
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.println("IP address: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nWiFi failed, continuing without WiFi...");
  }
}

void startWebServer() {
  // Serve HTML
  server.on("/", []() {
    File file = LittleFS.open("/index.html", "r");
    if (!file) {
      server.send(404, "text/plain", "index.html not found");
      return;
    }
    server.streamFile(file, "text/html");
    file.close();
  });

  // Serve CSS
  server.on("/style.css", []() {
    File file = LittleFS.open("/style.css", "r");
    if (!file) {
      server.send(404, "text/plain", "style.css not found");
      return;
    }
    server.streamFile(file, "text/css");
    file.close();
  });

  // Serve JS
  server.on("/allred", HTTP_GET, []() {
    allRed();
    server.send(200, "text/plain", "All signals set to RED");
  });


  // Serve JSON status
  server.on("/status", []() {
    String json = "{";
    json += "\"currentStep\":" + String(g_currentStep) + ",";
    json += "\"lanes\":[";
    
    json += "{ \"id\":\"A\", \"green\":" + String(greenA) +
            ", \"count\":" + String(laneA_count) +
            ", \"flow\":" + String(laneA_flow, 2) +
            ", \"speed\":" + String(laneA_speed, 2) + "},";

    json += "{ \"id\":\"B\", \"green\":" + String(greenB) +
            ", \"count\":" + String(laneB_count) +
            ", \"flow\":" + String(laneB_flow, 2) +
            ", \"speed\":" + String(laneB_speed, 2) + "},";

    json += "{ \"id\":\"C\", \"green\":" + String(greenC) +
            ", \"count\":" + String(laneC_count) +
            ", \"flow\":" + String(laneC_flow, 2) +
            ", \"speed\":" + String(laneC_speed, 2) + "}";

    json += "]}";
    server.send(200, "application/json", json);
  });

  // Start server
  server.begin();
  Serial.println("Web server started!");
}

// 🔹 Must be called inside loop()
void handleWebServer() {
  server.handleClient();
}

// Update helpers
void updateTrafficStatus(int step) {
  g_currentStep = step;
}

void updateSensorData(int count, float speed, float flow) {
  g_vehicleCount = count;
  g_avgSpeed = speed;
  g_flowRate = flow;
}

void pushLog(const String &message) {
  g_log += message + "\n";
  if (g_log.length() > 2000) {
    g_log.remove(0, 500);  // trim old logs
  }
}
