#include "limine.h"
#include <stddef.h>
#include <stdint.h>

namespace frosty {

enum class color : uint32_t {
  pink = 0xFFB2D1,
  white = 0xFFFFFF,
  black = 0x000000,
  hot_pink = 0xFF69B4,
  nose = 0xFF7777,
  ink = 0x4B0082
};

namespace math {
template <typename T> [[nodiscard]] constexpr auto abs(T v) noexcept -> T {
  return v < 0 ? -v : v;
}

struct sin_table {
  int32_t values[36];
  consteval sin_table() : values{} {
    int32_t raw[] = {0,    17,  34,  50,  64,  76,  86,  93,  98,
                     100,  98,  93,  86,  76,  64,  50,  34,  17,
                     0,    -17, -34, -50, -64, -76, -86, -93, -98,
                     -100, -98, -93, -86, -76, -64, -50, -34, -17};
    for (auto i = 0; i < 36; ++i)
      values[i] = raw[i];
  }
};

static constexpr auto lut = sin_table();
[[nodiscard]] constexpr auto sin(int32_t angle) noexcept -> int32_t {
  return lut.values[(angle % 360) / 10];
}
} // namespace math

namespace hw {
struct port {
  static inline void outb(uint16_t p, uint8_t v) noexcept {
    asm volatile("outb %0, %1" : : "a"(v), "Nd"(p) : "memory");
  }
  [[nodiscard]] static inline auto inb(uint16_t p) noexcept -> uint8_t {
    uint8_t r;
    asm volatile("inb %1, %0" : "=a"(r) : "Nd"(p) : "memory");
    return r;
  }
};

struct speaker {
  static void play(uint32_t freq) noexcept {
    if (freq < 20)
      return;
    const auto div = 1193180 / freq;
    port::outb(0x43, 0xB6);
    port::outb(0x42, static_cast<uint8_t>(div & 0xFF));
    port::outb(0x42, static_cast<uint8_t>((div >> 8) & 0xFF));
    const auto tmp = port::inb(0x61);
    if (!(tmp & 3))
      port::outb(0x61, tmp | 3);
  }
  static void mute() noexcept { port::outb(0x61, port::inb(0x61) & 0xFC); }
};

class input_handler {
public:
  int mouse_x{0}, mouse_y{0};
  bool left_click{false}, ctrl{false}, z{false}, z_pressed{false};

  void init(int x, int y) noexcept {
    mouse_x = x;
    mouse_y = y;
    ps2_ready(1);
    port::outb(0x64, 0xA8);
    ps2_ready(1);
    port::outb(0x64, 0x20);
    ps2_ready(0);
    auto status = port::inb(0x60) | 2;
    ps2_ready(1);
    port::outb(0x64, 0x60);
    ps2_ready(1);
    port::outb(0x60, status);
    mouse_cmd(0xF6);
    mouse_read();
    mouse_cmd(0xF4);
    mouse_read();
  }

  void update() noexcept {
    while (port::inb(0x64) & 1) {
      uint8_t status = port::inb(0x64);
      uint8_t data = port::inb(0x60);

      if (status & 0x20) { // Data is from Mouse
        // Self-Sync: First byte of packet must have bit 3 set
        if (m_cycle == 0 && !(data & 0x08))
          continue;

        m_packet[m_cycle++] = data;
        if (m_cycle == 3) {
          m_cycle = 0;
          left_click = m_packet[0] & 0x01;
          int32_t dx = m_packet[1];
          int32_t dy = m_packet[2];
          if (m_packet[0] & 0x10)
            dx -= 256;
          if (m_packet[0] & 0x20)
            dy -= 256;
          mouse_x += dx;
          mouse_y -= dy;
        }
      } else { // Data is from Keyboard
        if (data == 0x1D)
          ctrl = true;
        else if (data == 0x9D)
          ctrl = false;
        else if (data == 0x2C) {
          if (!z_pressed) {
            z = true;
            z_pressed = true;
          }
        } else if (data == 0xAC) {
          z = false;
          z_pressed = false;
        }
      }
    }
  }

private:
  uint8_t m_packet[3]{};
  uint8_t m_cycle{0};

  void ps2_ready(uint8_t type) noexcept {
    for (int i = 0; i < 1000; i++) {
      if (type == 0 && (port::inb(0x64) & 1))
        return;
      if (type == 1 && !(port::inb(0x64) & 2))
        return;
    }
  }
  void mouse_cmd(uint8_t data) noexcept {
    ps2_ready(1);
    port::outb(0x64, 0xD4);
    ps2_ready(1);
    port::outb(0x60, data);
  }
  auto mouse_read() noexcept -> uint8_t {
    ps2_ready(0);
    return port::inb(0x60);
  }
};
} // namespace hw

namespace gfx {
// Fixed size internal buffer for 1-to-1 mapping
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
} // namespace gfx

struct point {
  int x, y;
};
constinit static point doodle_data[20000] = {};
constinit static int strokes[512];
constinit static int p_idx = 0;
constinit static int s_idx = 0;
constinit static bool drawing = false;

} // namespace frosty

static volatile limine_framebuffer_request fb_req = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID, .revision = 0};

extern "C" void kmain() {
  using namespace frosty;
  if (!fb_req.response)
    while (true)
      asm("hlt");

  gfx::canvas screen{fb_req.response->framebuffers[0]};
  hw::input_handler input{};
  input.init(screen.width() / 2, screen.height() / 2);

  const uint32_t scale[] = {261, 329, 392, 440, 523};
  auto frame = 0;

  while (true) {
    screen.clear(color::pink);
    input.update();

    // Screen Boundary Enforcement
    if (input.mouse_x < 0)
      input.mouse_x = 0;
    if (input.mouse_y < 0)
      input.mouse_y = 0;
    if (input.mouse_x >= screen.width())
      input.mouse_x = screen.width() - 1;
    if (input.mouse_y >= screen.height())
      input.mouse_y = screen.height() - 1;

    if (input.left_click) {
      if (!drawing && s_idx < 511) {
        strokes[s_idx++] = p_idx;
        drawing = true;
      }
      if (p_idx < 19999) {
        if (p_idx == 0 || (doodle_data[p_idx - 1].x != input.mouse_x ||
                           doodle_data[p_idx - 1].y != input.mouse_y)) {
          doodle_data[p_idx++] = {input.mouse_x, input.mouse_y};
        }
      }
    } else {
      drawing = false;
    }

    if (input.ctrl && input.z) {
      if (s_idx > 0)
        p_idx = strokes[--s_idx];
      input.z = false;
    }

    for (int s = 0; s < s_idx; ++s) {
      int start = strokes[s];
      int end = (s + 1 < s_idx) ? strokes[s + 1] : p_idx;
      for (int i = start + 1; i < end; ++i) {
        screen.draw_line(doodle_data[i - 1].x, doodle_data[i - 1].y,
                         doodle_data[i].x, doodle_data[i].y, color::ink);
      }
    }

    const auto kitty_y = input.mouse_y + (math::sin(frame * 6) * 15 / 100);
    screen.draw_rect(input.mouse_x, kitty_y, 44, 44, color::white);
    screen.draw_rect(input.mouse_x + 8, kitty_y + 10, 8, 8, color::black);
    screen.draw_rect(input.mouse_x + 28, kitty_y + 10, 8, 8, color::black);
    screen.draw_rect(input.mouse_x + 17, kitty_y + 28, 10, 5, color::nose);

    screen.swap();

    frame++;
    for (volatile int i = 0; i < 2000; i++)
      ;
  }
}
