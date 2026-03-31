#include "limine.h"
#include <stddef.h>
#include <stdint.h>

namespace frosty::math {
static inline int32_t sin(int32_t angle) {
  static constexpr int32_t lut[] = {
      0,   17,  34,  50,   64,  76,  86,  93,  98,  100, 98,  93,
      86,  76,  64,  50,   34,  17,  0,   -17, -34, -50, -64, -76,
      -86, -93, -98, -100, -98, -93, -86, -76, -64, -50, -34, -17};
  return lut[(angle % 360) / 10];
}
} // namespace frosty::math

namespace frosty::hw {
struct port {
  static inline void outb(uint16_t p, uint8_t v) {
    asm volatile("outb %0, %1" : : "a"(v), "Nd"(p));
  }
  static inline uint8_t inb(uint16_t p) {
    uint8_t r;
    asm volatile("inb %1, %0" : "=a"(r) : "Nd"(p));
    return r;
  }
  static void wait() { outb(0x80, 0); }
};

struct speaker {
  static void play(uint32_t freq) {
    if (freq == 0)
      return;
    uint32_t div = 1193180 / freq;
    port::outb(0x43, 0xB6);
    port::wait();
    port::outb(0x42, static_cast<uint8_t>(div & 0xFF));
    port::wait();
    port::outb(0x42, static_cast<uint8_t>((div >> 8) & 0xFF));
    uint8_t tmp = port::inb(0x61);
    if (tmp != (tmp | 3))
      port::outb(0x61, tmp | 3);
  }
  static void mute() { port::outb(0x61, port::inb(0x61) & 0xFC); }
};
} // namespace frosty::hw

namespace frosty::gfx {
struct color {
  uint32_t value;
  static constexpr color pink() { return {0xFFB2D1}; }
  static constexpr color white() { return {0xFFFFFF}; }
  static constexpr color black() { return {0x000000}; }
  static constexpr color hot_pink() { return {0xFF69B4}; }
};

class canvas {
  limine_framebuffer *fb;

public:
  canvas(limine_framebuffer *f) : fb(f) {}

  void rect(int x, int y, int w, int h, color c) {
    auto *ptr = static_cast<uint32_t *>(fb->address);
    int stride = fb->pitch / 4;
    for (int i = y; i < y + h; ++i) {
      if (i < 0 || i >= static_cast<int>(fb->height))
        continue;
      for (int j = x; j < x + w; ++j) {
        if (j >= 0 && j < static_cast<int>(fb->width)) {
          ptr[i * stride + j] = c.value;
        }
      }
    }
  }

  uint64_t width() const { return fb->width; }
  uint64_t height() const { return fb->height; }
};
} // namespace frosty::gfx

static volatile limine_framebuffer_request fb_req = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID, .revision = 0};

extern "C" void kmain() {
  using namespace frosty;

  if (!fb_req.response || fb_req.response->framebuffer_count < 1) {
    while (true)
      asm("hlt");
  }

  gfx::canvas screen(fb_req.response->framebuffers[0]);

  int angle = 0;
  int base_x = 0;
  int sz = 60;

  while (true) {
    screen.rect(0, 0, screen.width(), screen.height(), gfx::color::pink());

    for (int i = 0; i < static_cast<int>(screen.width()); i += 40) {
      int wave_y = (screen.height() / 2) + (math::sin(angle + i) * 100 / 100);
      screen.rect(i, wave_y, 10, 10, gfx::color::hot_pink());
    }

    int kitty_y = (screen.height() / 2 - 30) + (math::sin(angle) * 150 / 100);
    int kitty_x = base_x;

    screen.rect(kitty_x, kitty_y, sz, sz, gfx::color::white());
    screen.rect(kitty_x + 12, kitty_y + 15, 10, 10, gfx::color::black());
    screen.rect(kitty_x + 38, kitty_y + 15, 10, 10, gfx::color::black());
    screen.rect(kitty_x + 25, kitty_y + 35, 10, 6, {0xFF7777});

    base_x = (base_x + 5) % screen.width();
    angle = (angle + 10) % 360;

    if (angle % 90 == 0) {
      hw::speaker::play(440 + angle);
    } else {
      hw::speaker::mute();
    }

    for (volatile uint64_t i = 0; i < 0x1FFFFF; i = i + 1)
      ;
  }
}
