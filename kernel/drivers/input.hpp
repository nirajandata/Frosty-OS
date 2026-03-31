#pragma once
#include "../arch/io.hpp"

namespace frosty::hw {
class input_handler {
public:
  int32_t mouse_x{0}, mouse_y{0};
  bool left_click{false}, ctrl{false}, z{false}, z_pressed{false};
  bool size_up{false}, size_down{false};
  uint8_t active_tool{1};

  void init(int x, int y, int sw, int sh) noexcept {
    mouse_x = x;
    mouse_y = y;
    lw = sw;
    lh = sh;
    flush();
    ps2(1);
    port::outb(0x64, 0xA8);
    ps2(1);
    port::outb(0x64, 0x20);
    ps2(0);
    uint8_t s = port::inb(0x60) | 2;
    ps2(1);
    port::outb(0x64, 0x60);
    ps2(1);
    port::outb(0x60, s);
    cmd(0xF6);
    cmd(0xF3);
    cmd(200);
    cmd(0xE8);
    cmd(3);
    cmd(0xF4);
    flush();
  }

  void update() noexcept {
    while (port::inb(0x64) & 0x01) {
      uint8_t s = port::inb(0x64);
      uint8_t d = port::inb(0x60);
      if (s & 0x20)
        proc_m(d);
      else
        proc_k(d);
    }
  }

private:
  uint8_t p[3]{};
  uint8_t c{0};
  int lw{0}, lh{0};

  void flush() noexcept {
    while (port::inb(0x64) & 0x01)
      port::inb(0x60);
  }

  void proc_m(uint8_t d) noexcept {
    if (c == 0 && !(d & 0x08))
      return;
    p[c++] = d;
    if (c == 3) {
      c = 0;
      left_click = p[0] & 1;
      int32_t dx = static_cast<int32_t>(static_cast<int8_t>(p[1]));
      int32_t dy = static_cast<int32_t>(static_cast<int8_t>(p[2]));
      mouse_x += dx;
      mouse_y -= dy;
      if (mouse_x < 0)
        mouse_x = 0;
      if (mouse_y < 0)
        mouse_y = 0;
      if (mouse_x >= lw)
        mouse_x = lw - 1;
      if (mouse_y >= lh)
        mouse_y = lh - 1;
    }
  }

  void proc_k(uint8_t d) noexcept {
    if (d == 0x1D)
      ctrl = true;
    else if (d == 0x9D)
      ctrl = false;
    else if (d == 0x2C) {
      if (!z_pressed) {
        z = true;
        z_pressed = true;
      }
    } else if (d == 0xAC) {
      z = false;
      z_pressed = false;
    } else if (d >= 0x02 && d <= 0x0B)
      active_tool = d - 0x01;
    else if (d == 0x0C)
      size_down = true;
    else if (d == 0x0D)
      size_up = true;
  }

  void ps2(uint8_t t) noexcept {
    for (int i = 0; i < 1000; i++) {
      if (t == 0 && (port::inb(0x64) & 1))
        return;
      if (t == 1 && !(port::inb(0x64) & 2))
        return;
    }
  }

  void cmd(uint8_t d) noexcept {
    ps2(1);
    port::outb(0x64, 0xD4);
    ps2(1);
    port::outb(0x60, d);
    ps2(0);
    port::inb(0x60);
  }
};
} // namespace frosty::hw
