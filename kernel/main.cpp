#include "limine.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace frosty {

enum class color : std::uint32_t {
  pink = 0xFFB2D1,
  white = 0xFFFFFF,
  black = 0x000000,
  hot_pink = 0xFF69B4,
  nose = 0xFF7777
};

namespace math {
[[nodiscard]] consteval auto get_sin_lut() -> std::array<std::int32_t, 36> {
  return {0,   17,  34,  50,   64,  76,  86,  93,  98,  100, 98,  93,
          86,  76,  64,  50,   34,  17,  0,   -17, -34, -50, -64, -76,
          -86, -93, -98, -100, -98, -93, -86, -76, -64, -50, -34, -17};
}

[[nodiscard]] constexpr std::int32_t sin(std::int32_t angle) noexcept {
  constexpr auto lut = get_sin_lut();
  const auto safe_angle = ((angle % 360) + 360) % 360;
  return lut[static_cast<std::size_t>(safe_angle / 10)];
}

class random {
  std::uint64_t state;

public:
  constexpr explicit random(std::uint64_t seed = 0) noexcept {
    if !consteval {
      if (seed == 0) {
        std::uint32_t low, high;
        asm volatile("rdtsc" : "=a"(low), "=d"(high));
        state = (static_cast<std::uint64_t>(high) << 32) | low;
      } else {
        state = seed;
      }
    } else {
      state = seed != 0 ? seed : 0xDEADBEEF;
    }
  }

  [[nodiscard]] constexpr std::uint64_t next(this random &self) noexcept {
    self.state ^= self.state << 13;
    self.state ^= self.state >> 7;
    self.state ^= self.state << 17;
    return self.state;
  }

  [[nodiscard]] constexpr std::uint64_t
  range(this random &self, std::uint64_t min, std::uint64_t max) noexcept {
    return min + (self.next() % (max - min + 1));
  }
};
} // namespace math

namespace hw {
struct port {
  static inline void outb(std::uint16_t p, std::uint8_t v) noexcept {
    if !consteval {
      asm volatile("outb %0, %1" : : "a"(v), "Nd"(p));
    }
  }
  [[nodiscard]] static inline std::uint8_t inb(std::uint16_t p) noexcept {
    if consteval {
      return 0;
    }
    std::uint8_t r;
    asm volatile("inb %1, %0" : "=a"(r) : "Nd"(p));
    return r;
  }
  static void io_wait() noexcept { outb(0x80, 0); }
};

struct timer {
  static void sleep(uint64_t iterations) {
    for (volatile uint64_t i = 0; i < iterations; i = i + 1) {
      asm volatile("pause");
    }
  }
};

struct mouse {
  int x, y;
  uint8_t cycle = 0;
  uint8_t packet[3];

  void wait(uint8_t type) {
    uint32_t timeout = 100000;
    if (type == 0) {
      while (timeout--)
        if ((port::inb(0x64) & 1) == 1)
          return;
    } else {
      while (timeout--)
        if ((port::inb(0x64) & 2) == 0)
          return;
    }
  }

  void write(uint8_t data) {
    wait(1);
    port::outb(0x64, 0xD4);
    wait(1);
    port::outb(0x60, data);
  }

  uint8_t read() {
    wait(0);
    return port::inb(0x60);
  }

  void init(int start_x, int start_y) {
    x = start_x;
    y = start_y;

    wait(1);
    port::outb(0x64, 0xA8);
    wait(1);
    port::outb(0x64, 0x20);
    wait(0);
    uint8_t status = (port::inb(0x60) | 2);
    wait(1);
    port::outb(0x64, 0x60);
    wait(1);
    port::outb(0x60, status);

    write(0xF6);
    read();
    write(0xF4);
    read();
  }

  void poll() {
    if ((port::inb(0x64) & 1) == 0)
      return;

    uint8_t data = port::inb(0x60);
    packet[cycle] = data;
    cycle = (cycle + 1) % 3;

    if (cycle == 0) {
      int dx = (int8_t)packet[1];
      int dy = (int8_t)packet[2];
      x += dx;
      y -= dy;
    }
  }
};

struct speaker {
  static void play(std::uint32_t freq) noexcept {
    if (freq == 0)
      return;
    const std::uint32_t div = 1193180 / freq;
    port::outb(0x43, 0xB6);
    port::io_wait();
    port::outb(0x42, static_cast<std::uint8_t>(div & 0xFF));
    port::io_wait();
    port::outb(0x42, static_cast<std::uint8_t>((div >> 8) & 0xFF));

    const std::uint8_t tmp = port::inb(0x61);
    if (tmp != (tmp | 3)) {
      port::outb(0x61, tmp | 3);
    }
  }
  static void mute() noexcept { port::outb(0x61, port::inb(0x61) & 0xFC); }
};
} // namespace hw

namespace gfx {
class canvas {
  limine_framebuffer *fb;

public:
  constexpr explicit canvas(limine_framebuffer *f) noexcept : fb(f) {}

  constexpr void rect(this const canvas &self, int x, int y, int w, int h,
                      color c) noexcept {
    if !consteval {
      auto *ptr = static_cast<std::uint32_t *>(self.fb->address);
      const int stride = self.fb->pitch / 4;

      for (int i = y; i < y + h; ++i) {
        if (i < 0 || i >= static_cast<int>(self.fb->height))
          continue;
        for (int j = x; j < x + w; ++j) {
          if (j >= 0 && j < static_cast<int>(self.fb->width)) {
            ptr[i * stride + j] = std::to_underlying(c);
          }
        }
      }
    }
  }

  [[nodiscard]] constexpr std::uint64_t
  width(this const canvas &self) noexcept {
    return self.fb->width;
  }
  [[nodiscard]] constexpr std::uint64_t
  height(this const canvas &self) noexcept {
    return self.fb->height;
  }
};
} // namespace gfx

} // namespace frosty

constinit volatile limine_framebuffer_request fb_req = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID, .revision = 0, .response = nullptr};

extern "C" void kmain() noexcept {
  using namespace frosty;

  hw::mouse m;

  if (!fb_req.response || fb_req.response->framebuffer_count < 1) {
    for (;;)
      asm volatile("hlt");
  }

  frosty::math::random rng;
  gfx::canvas screen(fb_req.response->framebuffers[0]);

  int sz = 60;
  uint32_t notes[] = {220, 246, 277, 329, 369};

  int angle = 0;
  int base_x = 0;
  uint64_t tick{};
  m.init(screen.width() / 2, screen.height() / 2);

  for (;;) {
    screen.rect(0, 0, screen.width(), screen.height(), color::pink);

    m.poll();
    if (m.x < 0)
      m.x = 0;
    if (m.y < 0)
      m.y = 0;
    if (m.x > (int)screen.width() - 10)
      m.x = screen.width() - 10;
    if (m.y > (int)screen.height() - 10)
      m.y = screen.height() - 10;

    screen.rect(m.x, m.y, sz, sz, color::white);
    screen.rect(m.x + 12, m.y + 15, 8, 8, color::black);
    screen.rect(m.x + 40, m.y + 15, 8, 8, color::black);
    screen.rect(m.x + 25, m.y + 35, 10, 5, color::nose);

    tick++;

    // commenting out sound since it can be quite annoying
    // if (tick % 200 == 0) {
    //   hw::speaker::play(notes[tick % 5]);
    // } else if (tick % 200 == 50) {
    //   hw::speaker::mute();
    // }

    hw::timer::sleep(0x39992);
  }
}
