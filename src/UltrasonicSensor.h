#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

#define ULTRASONIC_MAX_SENSORS 4
#define ULTRASONIC_LOG_ENTRIES 128

struct UltrasonicResult {
    int vehicleCount;
    float occupancyRatio;
    float avgSpeed;    // cm/s (relative)
    unsigned long timestamp; // millis() at end of window
};

struct UltrasonicConfig {
    int trigPin;
    int echoPin;
    int windowSeconds;       // measurement window for this sensor
    const char *name;        // e.g. "NS_1"
    uint16_t sampleHz;       // desired sample rate (e.g. 20 = 20Hz)
    float dropThresholdCm;   // detection threshold (default ~50cm)
    float riseThresholdCm;   // return threshold (default ~20cm)
};

#ifdef __cplusplus
extern "C" {
#endif

// Lifecycle
bool startUltrasonicSystem(const UltrasonicConfig *configs, size_t count); // returns true if tasks started
void stopUltrasonicSystem(); // stops tasks (graceful)

// Data access
bool tryPopResult(int sensorIndex, UltrasonicResult &out); // non-blocking pop from results queue for a sensor
UltrasonicResult peekLatest(int sensorIndex); // copy of latest result (0..count-1), timestamp==0 means empty

// Utilities
void forceCalibrateBaseline(int sensorIndex); // force baseline recalculation on next window

#ifdef __cplusplus
}
#endif
