#pragma once
#include <stdint.h>

namespace frosty {

enum class color : uint32_t {
  pink = 0xFFB2D1,
  white = 0xFFFFFF,
  black = 0x000000,
  cyan = 0x00FFFF,
  gui_bg = 0xF0F0F0,
  white_pure = 0xFFFFFF
};

inline uint32_t make_raw(uint8_t r, uint8_t g, uint8_t b) {
  return (uint32_t)r << 16 | (uint32_t)g << 8 | (uint32_t)b;
}

inline uint32_t hsv_to_rgb(float h, float s, float v) {
  s /= 255.0f;
  v /= 255.0f;
  if (s == 0)
    return make_raw(v * 255, v * 255, v * 255);

  float hh = (h >= 360.0f) ? 0.0f : h / 60.0f;
  int i = (int)hh;
  float ff = hh - i;
  float p = v * (1.0f - s);
  float q = v * (1.0f - (s * ff));
  float t = v * (1.0f - (s * (1.0f - ff)));

  switch (i) {
  case 0:
    return make_raw(v * 255, t * 255, p * 255);
  case 1:
    return make_raw(q * 255, v * 255, p * 255);
  case 2:
    return make_raw(p * 255, v * 255, t * 255);
  case 3:
    return make_raw(p * 255, q * 255, v * 255);
  case 4:
    return make_raw(t * 255, p * 255, v * 255);
  default:
    return make_raw(v * 255, p * 255, q * 255);
  }
}

struct draw_point {
  int x, y;
  uint32_t c;
  uint8_t size;
  draw_point() = default;
  draw_point(int _x, int _y, uint32_t _c, uint8_t _s)
      : x(_x), y(_y), c(_c), size(_s) {}
};
} // namespace frosty
