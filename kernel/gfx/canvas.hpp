#pragma once
#include "../limine.h"
#include "../math/math.hpp"
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

  void draw_line(int x0, int y0, int x1, int y1, color c) noexcept {
    int dx = math::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -math::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while (true) {
      for (int i = -1; i <= 1; ++i)
        for (int j = -1; j <= 1; ++j)
          put_pixel(x0 + i, y0 + j, c);
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

  void draw_rect(int x, int y, int w, int h, color c) noexcept {
    for (int i = y; i < y + h; ++i)
      for (int j = x; j < x + w; ++j)
        put_pixel(j, i, c);
  }

  void swap() noexcept {
    auto *const front = static_cast<uint32_t *>(fb->address);
    const uint32_t stride = fb->pitch / 4;
    for (uint32_t y = 0; y < fb->height; ++y) {
      for (uint32_t x = 0; x < fb->width; ++x) {
        front[y * stride + x] = back_buffer[y * fb->width + x];
      }
    }
  }

  [[nodiscard]] constexpr auto width() const noexcept -> int {
    return (int)fb->width;
  }
  [[nodiscard]] constexpr auto height() const noexcept -> int {
    return (int)fb->height;
  }
};

} // namespace frosty::gfx
