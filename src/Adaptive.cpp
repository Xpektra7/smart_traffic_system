#include <Arduino.h>
#include "Adaptive.h"

// Keep smoothed values static so they persist across calls
static float smoothedFlow = 0;
static float smoothedSpeed = 0;
static bool initialized = false;

unsigned long adjustGreen(unsigned long currentGreen,
                          float flow,
                          float speed,
                          float Kp,
                          float s_target,
                          float deltamax,
                          unsigned long minGreen,
                          unsigned long maxGreen) {
  const float alpha = 0.3; // smoothing factor (0.1=very smooth, 0.5=fast response)

  // Initialize EMA on first call
  if (!initialized) {
    smoothedFlow = flow;
    smoothedSpeed = speed;
    initialized = true;
  } else {
    smoothedFlow = alpha * flow + (1 - alpha) * smoothedFlow;
    smoothedSpeed = alpha * speed + (1 - alpha) * smoothedSpeed;
  }

  // Use smoothed values
  float s = smoothedFlow / (smoothedSpeed > 0 ? smoothedSpeed : 1);
  float delta = Kp * (s - s_target);

  if (delta > deltamax) delta = deltamax;
  if (delta < -deltamax) delta = -deltamax;

  return constrain(currentGreen + (long)delta, minGreen, maxGreen);
}
