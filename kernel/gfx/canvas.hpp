#pragma once
#include "../limine.h"
#include "../math/math.hpp"
#include "font.hpp"
#include "types.hpp"
#include <cstddef>

namespace frosty::gfx {

alignas(32) constinit static uint32_t drawing_layer[1920 * 1080];
alignas(32) constinit static uint32_t back_buffer[1920 * 1080];

class canvas {
  limine_framebuffer *fb;

public:
  explicit canvas(limine_framebuffer *f) : fb(f) {}

  void clear_persistent(color c) noexcept {
    const auto val = static_cast<uint32_t>(c);
    for (size_t i = 0; i < (fb->width * fb->height); ++i)
      drawing_layer[i] = val;
  }

  void put_pixel(int x, int y, color c) noexcept {
    if (x < 0 || x >= (int)fb->width || y < 0 || y >= (int)fb->height)
      return;
    back_buffer[y * fb->width + x] = static_cast<uint32_t>(c);
  }

  void put_pixel_persistent(int x, int y, color c) noexcept {
    if (x < 0 || x >= (int)fb->width || y < 0 || y >= (int)fb->height)
      return;
    drawing_layer[y * fb->width + x] = static_cast<uint32_t>(c);
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

  void draw_line_persistent(int x0, int y0, int x1, int y1, int size,
                            color c) noexcept {
    int dx = math::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -math::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while (true) {
      int half = size / 2;
      for (int i = -half; i <= half; ++i)
        for (int j = -half; j <= half; ++j)
          put_pixel_persistent(x0 + j, y0 + i, c);
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

  void compose_layers() noexcept {
    uint32_t *src = drawing_layer;
    uint32_t *dst = back_buffer;
    size_t count = (fb->width * fb->height) / 8;
    asm volatile(".intel_syntax noprefix\n"
                 "1:\n"
                 "vmovdqu ymm0, [rax]\n"
                 "vmovdqu [rdx], ymm0\n"
                 "add rax, 32\n"
                 "add rdx, 32\n"
                 "dec rcx\n"
                 "jnz 1b\n"
                 ".att_syntax prefix\n"
                 : "+a"(src), "+d"(dst), "+c"(count)
                 :
                 : "ymm0", "memory");
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
