#pragma once
#include "../arch/io.hpp"

namespace frosty::hw {

class input_handler {
public:
  int mouse_x{0}, mouse_y{0};
  bool left_click{false}, ctrl{false}, z{false}, z_pressed{false};

  void init(int x, int y, int screen_w, int screen_h) noexcept {
    mouse_x = x;
    mouse_y = y;
    m_limit_w = screen_w;
    m_limit_h = screen_h;

    // Enable auxiliary mouse device
    ps2_ready(1);
    port::outb(0x64, 0xA8);

    // Enable interrupts
    ps2_ready(1);
    port::outb(0x64, 0x20);
    ps2_ready(0);
    uint8_t status = port::inb(0x60) | 2;
    ps2_ready(1);
    port::outb(0x64, 0x60);
    ps2_ready(1);
    port::outb(0x60, status);

    // Set defaults
    mouse_cmd(0xF6);
    mouse_read();
    // Enable data reporting
    mouse_cmd(0xF4);
    mouse_read();
  }

  void update() noexcept {
    // While data is available in the output buffer
    while (port::inb(0x64) & 0x01) {
      uint8_t status = port::inb(0x64);
      uint8_t data = port::inb(0x60);

      // Bit 5 of status port (0x64) tells us if data is from mouse
      if (status & 0x20) {
        process_mouse(data);
      } else {
        process_keyboard(data);
      }
    }
  }

private:
  uint8_t m_packet[3]{};
  uint8_t m_cycle{0};
  int m_limit_w{0}, m_limit_h{0};

  void process_mouse(uint8_t data) noexcept {
    // Sync check: Bit 3 of the first byte of a mouse packet is ALWAYS 1.
    // If it's 0, we are out of sync and must discard until we find a header.
    if (m_cycle == 0 && !(data & 0x08)) {
      return;
    }

    m_packet[m_cycle++] = data;

    if (m_cycle == 3) {
      m_cycle = 0;

      // Packet 0 bits: [Y overflow, X overflow, Y sign, X sign, 1, Middle,
      // Right, Left]
      left_click = m_packet[0] & 0x01;

      // Delta X and Y are 9-bit signed integers.
      // The 9th bit (sign bit) is in m_packet[0].
      int32_t dx = m_packet[1];
      int32_t dy = m_packet[2];

      if (m_packet[0] & 0x10)
        dx -= 256; // X sign bit
      if (m_packet[0] & 0x20)
        dy -= 256; // Y sign bit

      mouse_x += dx;
      mouse_y -= dy; // PS/2 Y is bottom-to-top, screen is top-to-bottom

      // Rigid boundary enforcement
      if (mouse_x < 0)
        mouse_x = 0;
      if (mouse_y < 0)
        mouse_y = 0;
      if (mouse_x >= m_limit_w)
        mouse_x = m_limit_w - 1;
      if (mouse_y >= m_limit_h)
        mouse_y = m_limit_h - 1;
    }
  }

  void process_keyboard(uint8_t data) noexcept {
    if (data == 0x1D)
      ctrl = true;
    else if (data == 0x9D)
      ctrl = false;
    else if (data == 0x2C) { // Z pressed
      if (!z_pressed) {
        z = true;
        z_pressed = true;
      }
    } else if (data == 0xAC) { // Z released
      z = false;
      z_pressed = false;
    }
  }

  void ps2_ready(uint8_t type) noexcept {
    for (int i = 0; i < 1000; i++) {
      if (type == 0 && (port::inb(0x64) & 0x01))
        return;
      if (type == 1 && !(port::inb(0x64) & 0x02))
        return;
    }
  }

  void mouse_cmd(uint8_t d) noexcept {
    ps2_ready(1);
    port::outb(0x64, 0xD4);
    ps2_ready(1);
    port::outb(0x60, d);
  }

  auto mouse_read() noexcept -> uint8_t {
    ps2_ready(0);
    return port::inb(0x60);
  }
};

} // namespace frosty::hw
