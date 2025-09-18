#include <Arduino.h>
#include "Adaptive.h"

unsigned long adjustGreen(unsigned long currentGreen, float flow, float speed, float Kp, float s_target, float deltamax, unsigned long minGreen, unsigned long maxGreen) {
  float s = flow / (speed > 0 ? speed : 1);
  float delta = Kp * (s - s_target);
  if (delta > deltamax) delta = deltamax;
  if (delta < -deltamax) delta = -deltamax;
  return constrain(currentGreen + (long)delta, minGreen, maxGreen);
}
