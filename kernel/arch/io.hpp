#pragma once
#include <stdint.h>

namespace frosty::hw {

struct port {
  static inline void outb(uint16_t p, uint8_t v) noexcept {
    asm volatile("outb %0, %1" : : "a"(v), "Nd"(p) : "memory");
  }
  static inline auto inb(uint16_t p) noexcept -> uint8_t {
    uint8_t r;
    asm volatile("inb %1, %0" : "=a"(r) : "Nd"(p) : "memory");
    return r;
  }
};

struct cpu {
  static inline void pause() noexcept { asm volatile("pause"); }

  static inline void hlt() noexcept { asm volatile("hlt"); }

  [[nodiscard]] static inline auto rdtsc() noexcept -> uint64_t {
    uint32_t low, high;
    asm volatile("rdtsc" : "=a"(low), "=d"(high));
    return (static_cast<uint64_t>(high) << 32) | low;
  }

  static void delay(uint64_t cycles) noexcept {
    const auto start = rdtsc();
    while (rdtsc() - start < cycles) {
      pause();
    }
  }
};

} // namespace frosty::hw
