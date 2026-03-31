#pragma once
#include <stdint.h>

namespace frosty {

enum class color : uint32_t {
  pink = 0xFFB2D1,
  white = 0xFFFFFF,
  black = 0x000000,
  hot_pink = 0xFF69B4,
  nose = 0xFF7777,
  ink = 0x4B0082,
  cyan = 0x00FFFF,
  mint = 0x98FF98,
  red = 0xFF4444,
  green = 0x44FF44,
  blue = 0x4444FF,
  yellow = 0xFFFF44,
  gui_bg = 0xF0F0F0,
  gui_border = 0xCCCCCC
};

inline uint32_t make_raw(uint8_t r, uint8_t g, uint8_t b) {
  return (uint32_t)r << 16 | (uint32_t)g << 8 | (uint32_t)b;
}

inline uint32_t hsv_to_rgb(int h, int s, int v) {
  if (s == 0)
    return make_raw(v, v, v);
  int region = h / 60;
  int rem = (h % 60) * 255 / 60;
  int p = (v * (255 - s)) / 255;
  int q = (v * (255 - ((s * rem) / 255))) / 255;
  int t = (v * (255 - ((s * (255 - rem)) / 255))) / 255;
  switch (region) {
  case 0:
    return make_raw(v, t, p);
  case 1:
    return make_raw(q, v, p);
  case 2:
    return make_raw(p, v, t);
  case 3:
    return make_raw(p, q, v);
  case 4:
    return make_raw(t, p, v);
  default:
    return make_raw(v, p, q);
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
