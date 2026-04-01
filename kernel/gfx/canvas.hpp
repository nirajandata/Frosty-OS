#pragma once
#include "../limine.h"
#include "font.hpp"
#include "types.hpp"
#include <cstddef>
#include <initializer_list>

namespace frosty::gfx {

class canvas {
  limine_framebuffer *fb;
  uint32_t *art_layer, *back_buffer;

public:
  explicit canvas(limine_framebuffer *f, uint32_t *l1, uint32_t *l2)
      : fb(f), art_layer(l1), back_buffer(l2) {}

  inline void px(uint32_t *buf, int x, int y, color c) noexcept {
    if (x >= 0 && x < (int)fb->width && y >= 0 && y < (int)fb->height)
      buf[y * fb->width + x] = static_cast<uint32_t>(c);
  }

  void fill(uint32_t *buf, color c) noexcept {
    uint32_t val = static_cast<uint32_t>(c);
    size_t total = fb->width * fb->height;
    size_t n = total / 8;
    uint32_t *p = buf;
    asm volatile(".intel_syntax noprefix\n"
                 "vmovd xmm0, %2\n"
                 "vpbroadcastd ymm0, xmm0\n"
                 "1: vmovdqu [%0], ymm0\n"
                 "add %0, 32\n"
                 "dec %1\n"
                 "jnz 1b\n"
                 ".att_syntax prefix\n"
                 : "+r"(p), "+r"(n)
                 : "r"(val)
                 : "ymm0", "memory");
    for (size_t i = (total & ~7); i < total; ++i)
      buf[i] = val;
  }

  void rect(uint32_t *buf, float x, float y, float w, float h,
            color c) noexcept {
    for (int i = (int)y; i < (int)(y + h); ++i)
      for (int j = (int)x; j < (int)(x + w); ++j)
        px(buf, j, i, c);
  }

  void line(uint32_t *buf, float x0, float y0, float x1, float y1, float sz,
            color c, int xmin = -1, int ymin = -1, int xmax = 9999,
            int ymax = 9999) noexcept {
    float dx = (x1 - x0 > 0) ? x1 - x0 : x0 - x1;
    float dy = (y1 - y0 > 0) ? y1 - y0 : y0 - y1;
    float sx = x0 < x1 ? 1.0f : -1.0f, sy = y0 < y1 ? 1.0f : -1.0f;
    float err = dx - dy;

    while (true) {
      float r = sz / 2.0f;
      for (float i = -r; i <= r; i += 1.0f) {
        for (float j = -r; j <= r; j += 1.0f) {
          int cx = (int)(x0 + j), cy = (int)(y0 + i);
          if (cx >= xmin && cx <= xmax && cy >= ymin && cy <= ymax)
            px(buf, cx, cy, c);
        }
      }
      if (((x0 - x1 > 0 ? x0 - x1 : x1 - x0) < 0.1f) &&
          ((y0 - y1 > 0 ? y0 - y1 : y1 - y0) < 0.1f))
        break;
      float e2 = 2.0f * err;
      if (e2 > -dy) {
        err -= dy;
        x0 += sx;
      }
      if (e2 < dx) {
        err += dx;
        y0 += sy;
      }
    }
  }

  void logo(uint32_t *b, float x, float y, color c) noexcept {
    line(b, x - 10.0f, y, x + 10.0f, y, 1.0f, c);
    line(b, x, y - 10.0f, x, y + 10.0f, 1.0f, c);
    for (float i : {-1.0f, 1.0f}) {
      line(b, x - 7.0f, y - 7.0f * i, x + 7.0f, y + 7.0f * i, 1.0f, c);
      for (float j : {-1.0f, 1.0f}) {
        line(b, x + 10.0f * i, y + 10.0f * j, x + 7.0f * i, y + 10.0f * j, 1.0f,
             c);
        line(b, x + 10.0f * i, y + 10.0f * j, x + 10.0f * i, y + 7.0f * j, 1.0f,
             c);
      }
    }
  }

  void blit_full(uint32_t *src, uint32_t *dst) noexcept {
    size_t total = fb->width * fb->height;
    size_t n = total / 8;
    uint32_t *s = src;
    uint32_t *d = dst;
    asm volatile(".intel_syntax noprefix\n"
                 "1: vmovdqu ymm0, [%0]\n"
                 "vmovdqu [%1], ymm0\n"
                 "add %0, 32\n"
                 "add %1, 32\n"
                 "dec %2\n"
                 "jnz 1b\n"
                 ".att_syntax prefix\n"
                 : "+r"(s), "+r"(d), "+r"(n)
                 :
                 : "ymm0", "memory");
    for (size_t i = (total & ~7); i < total; ++i)
      dst[i] = src[i];
  }

  void swap() noexcept {
    uint32_t *f = (uint32_t *)fb->address;
    uint32_t stride = fb->pitch / 4;
    for (uint32_t y = 0; y < fb->height; ++y) {
      uint32_t *s = &back_buffer[y * fb->width];
      uint32_t *d = &f[y * stride];
      size_t n = fb->width / 8;
      if (n > 0) {
        asm volatile(".intel_syntax noprefix\n"
                     "1: vmovdqu ymm0, [%0]\n"
                     "vmovdqu [%1], ymm0\n"
                     "add %0, 32\n"
                     "add %1, 32\n"
                     "dec %2\n"
                     "jnz 1b\n"
                     ".att_syntax prefix\n"
                     : "+r"(s), "+r"(d), "+r"(n)
                     :
                     : "ymm0", "memory");
      }
      for (uint32_t x = (fb->width & ~7); x < fb->width; ++x)
        d[x] = s[x];
    }
  }

  void text(uint32_t *buf, float x, float y, const char *str,
            color c) noexcept {
    while (*str) {
      uint8_t idx = static_cast<uint8_t>(*str++);
      if (idx < 128) {
        for (int i = 0; i < 8; i++)
          for (int j = 0; j < 8; j++)
            if (font8x8[idx][i] & (1 << (7 - j)))
              px(buf, (int)x + j, (int)y + i, c);
      }
      x += 9.0f;
    }
  }

  uint32_t *art() { return art_layer; }
  uint32_t *bb() { return back_buffer; }
  int width() const { return (int)fb->width; }
  int height() const { return (int)fb->height; }
};
} // namespace frosty::gfx
