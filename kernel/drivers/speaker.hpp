#pragma once
#include "../arch/io.hpp"

namespace frosty::hw {

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

} // namespace frosty::hw
