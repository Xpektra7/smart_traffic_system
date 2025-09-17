#include <Arduino.h>

// ESP32 DevKitC (ESP32-WROOM-32D) — Wokwi mapping from your diagram.json
// Sensors: 3x HC-SR04 (Trig/Echo), 3x PIR (as microwave stand-ins)
// Actuators: 9x relay modules (R/Y/G for 3 approaches)
// Control: simple adaptive timing (base + extensions if queue/presence detected)

struct Approach {
  int redRelay;
  int yelRelay;
  int grnRelay;
  int trigPin;    // HC-SR04 TRIG
  int echoPin;    // HC-SR04 ECHO (input-only OK)
  int pirPin;     // PIR OUT
  const char* name;
};

// ---- Pin map from your connections ----
// Ultrasonic1: TRIG=4,  ECHO=34
// Ultrasonic2: TRIG=14, ECHO=35
// Ultrasonic3: TRIG=27, ECHO=36 (VP)
// PIR1->GPIO39(VN), PIR2->GPIO32, PIR3->GPIO33
// Relays: r/y/g per approach (from your relay IN wiring)
//   Right cluster: relay7 IN=5 (R), relay8 IN=19 (Y), relay9 IN=18 (G)
//   Top cluster:   relay1 IN=23 (R), relay2 IN=22 (Y), relay3 IN=21 (G)
//   Left cluster:  relay4 IN=25 (R), relay5 IN=13 (Y), relay6 IN=15 (G)

// NOTE: If your Wokwi relay is active-LOW, set ACTIVE_HIGH=false.
constexpr bool ACTIVE_HIGH = true;

Approach A = {23, 22, 21,  4, 34, 39, "A"}; // Top cluster + Ultrasonic1 + PIR1
Approach B = {25, 13, 15, 14, 35, 32, "B"}; // Left cluster + Ultrasonic2 + PIR2
Approach C = { 5, 19, 18, 27, 36, 33, "C"}; // Right cluster + Ultrasonic3 + PIR3

Approach approaches[3] = {A, B, C};

// ---- Timing (tune as needed) ----
uint32_t baseGreenMs    = 8000;   // base green time
uint32_t maxExtendMs     = 12000;  // max added time
uint32_t extendStepMs    = 1000;   // per occupancy/presence check
uint32_t checkIntervalMs = 500;    // how often to re-check during green
uint32_t yellowMs        = 2000;   // yellow time
uint32_t allRedMs        = 500;    // safety all-red between phases

// ---- Detection thresholds ----
float queueNearCm        = 120.0;  // vehicle considered "near" if distance < this
int   nearCountThresh    = 2;      // require this many "near" hits per window to extend

// ---- Helpers to drive relays ----
void setRelay(int pin, bool on) {
  digitalWrite(pin, (ACTIVE_HIGH ? (on ? HIGH : LOW) : (on ? LOW : HIGH)));
}
void setLights(const Approach& ap, bool R, bool Y, bool G) {
  setRelay(ap.redRelay, R);
  setRelay(ap.yelRelay, Y);
  setRelay(ap.grnRelay, G);
}
void allRed() {
  for (auto &ap : approaches) setLights(ap, true, false, false);
}

// ---- Ultrasonic distance (cm) with timeout ----
float readDistanceCm(int trig, int echo) {
  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  // 25ms timeout (~4.3m); adjust if needed
  uint32_t dur = pulseIn(echo, HIGH, 25000);
  if (dur == 0) return 9999.0; // timeout => no object
  // sound speed ~343 m/s => 29.1 us per cm round-trip
  return dur / 58.2; // cm
}

// ---- Presence & density heuristics ----
struct Presence {
  bool pir;
  bool nearUltrasonic;
  float distCm;
};

Presence sense(const Approach& ap) {
  Presence p{};
  p.pir = (digitalRead(ap.pirPin) == HIGH);
  p.distCm = readDistanceCm(ap.trigPin, ap.echoPin);
  p.nearUltrasonic = (p.distCm < queueNearCm);
  return p;
}

void phaseGreenAdaptive(Approach& ap) {
  // Yellow off, Red off, Green on for baseGreenMs + extensions if occupancy persists
  setLights(ap, false, false, true);
  uint32_t start = millis();
  uint32_t elapsed = 0;
  uint32_t extended = 0;
  uint32_t lastCheck = 0;
  int nearHits = 0;

  while (elapsed < baseGreenMs + extended) {
    uint32_t now = millis();
    elapsed = now - start;

    if (now - lastCheck >= checkIntervalMs) {
      lastCheck = now;
      Presence p = sense(ap);
      if (p.pir || p.nearUltrasonic) nearHits++;
      // Every ~2s window, decide on extension
      if ((elapsed % 2000) < checkIntervalMs) {
        if (nearHits >= nearCountThresh && extended + extendStepMs <= maxExtendMs) {
          extended += extendStepMs;
        }
        nearHits = 0;
      }
    }
    // small yield
    delay(5);
  }

  // Yellow
  setLights(ap, false, true, false);
  delay(yellowMs);

  // Back to red before next approach
  setLights(ap, true, false, false);
  delay(allRedMs);
}

void setup() {
  Serial.begin(115200);

  // Relays as outputs, default all-RED
  int relayPins[] = {A.redRelay, A.yelRelay, A.grnRelay,
                     B.redRelay, B.yelRelay, B.grnRelay,
                     C.redRelay, C.yelRelay, C.grnRelay};
  for (int pin : relayPins) {
    pinMode(pin, OUTPUT);
    setRelay(pin, false); // off
  }
  allRed();

  // PIR inputs
  pinMode(A.pirPin, INPUT);
  pinMode(B.pirPin, INPUT);
  pinMode(C.pirPin, INPUT);

  // Echo pins are input-only; TRIG pins set in readDistanceCm()
  Serial.println("Adaptive traffic controller running...");
}

void loop() {
  // Simple round-robin with adaptive green per approach
  for (auto &ap : approaches) {
    // Ensure cross approaches are red
    for (auto &other : approaches) {
      if (&other != &ap) setLights(other, true, false, false);
    }
    // Run adaptive green on current approach
    phaseGreenAdaptive(ap);
  }
}
