#pragma once
#include "../limine.h"
#include "../math/math.hpp"
#include "font.hpp"
#include "types.hpp"
#include <cstddef>

namespace frosty::gfx {

constinit static uint32_t back_buffer[1920 * 1080];

class canvas {
  limine_framebuffer *fb;

public:
  explicit canvas(limine_framebuffer *f) : fb(f) {}

  void clear(color c) noexcept {
    const auto val = static_cast<uint32_t>(c);
    for (size_t i = 0; i < (fb->width * fb->height); ++i)
      back_buffer[i] = val;
  }

  void put_pixel(int x, int y, color c) noexcept {
    if (x < 0 || x >= (int)fb->width || y < 0 || y >= (int)fb->height)
      return;
    back_buffer[y * fb->width + x] = static_cast<uint32_t>(c);
  }

  void draw_rect(int x, int y, int w, int h, color c) noexcept {
    for (int i = y; i < y + h; ++i)
      for (int j = x; j < x + w; ++j)
        put_pixel(j, i, c);
  }

  void draw_text(int x, int y, const char *str, color c) noexcept {
    while (*str) {
      uint8_t index = static_cast<uint8_t>(*str);
      if (index < 128) {
        const uint8_t *glyph = font8x8[index];
        for (int i = 0; i < 8; i++) {
          for (int j = 0; j < 8; j++) {
            if (glyph[i] & (1 << (7 - j))) {
              put_pixel(x + j, y + i, c);
            }
          }
        }
      }
      x += 9;
      str++;
    }
  }

  void draw_line(int x0, int y0, int x1, int y1, int size, color c) noexcept {
    int dx = math::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -math::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while (true) {
      int half = size / 2;
      for (int i = -half; i <= half; ++i)
        for (int j = -half; j <= half; ++j)
          put_pixel(x0 + j, y0 + i, c);

      if (x0 == x1 && y0 == y1)
        break;
      int e2 = 2 * err;
      if (e2 >= dy) {
        err += dy;
        x0 += sx;
      }
      if (e2 <= dx) {
        err += dx;
        y0 += sy;
      }
    }
  }

  void swap() noexcept {
    auto *const front = static_cast<uint32_t *>(fb->address);
    const uint32_t stride = fb->pitch / 4;
    for (uint32_t y = 0; y < fb->height; ++y)
      for (uint32_t x = 0; x < fb->width; ++x)
        front[y * stride + x] = back_buffer[y * fb->width + x];
  }

  int width() const { return (int)fb->width; }
  int height() const { return (int)fb->height; }
};
} // namespace frosty::gfx
