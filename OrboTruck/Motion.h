#pragma once
#include <math.h>
#include <stdint.h>

namespace Motion {
const float PI_F = 3.14159265358979323846f;
struct Wheel { float angle, speed; };
inline float wrap(float a) {
  while (a > 180) a -= 360;
  while (a <= -180) a += 360;
  return a;
}
// Body frame: x forward, y left, positive omega counterclockwise.
inline bool solve(float vx, float vy, float omega, float x, float y,
                  float low, float high, float previous, Wheel &out) {
  float wx = vx - omega*y, wy = vy + omega*x;
  float speed = sqrtf(wx*wx + wy*wy);
  if (speed < 0.001f) { out = {previous, 0}; return true; }
  float a = wrap(atan2f(wy, wx)*180/PI_F);
  float b = wrap(a+180);
  bool okA = a >= low && a <= high, okB = b >= low && b <= high;
  if (!okA && !okB) return false;
  if (okA && (!okB || fabsf(a-previous) <= fabsf(b-previous))) out = {a, speed};
  else out = {b, -speed};
  return true;
}
inline float approach(float value, float goal, float step) {
  if (value < goal) return fminf(value+step, goal);
  return fmaxf(value-step, goal);
}
// Signed modular difference without signed-overflow arithmetic.
inline int32_t countDelta(uint32_t now, uint32_t before) {
  uint32_t d = now-before;
  return d <= 0x7FFFFFFFUL ? (int32_t)d : -1-(int32_t)(0xFFFFFFFFUL-d);
}
}
