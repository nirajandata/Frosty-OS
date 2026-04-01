#pragma once
#include <stdint.h>

namespace frosty::hw {

// for simd support to prevent os crash T-T
struct simd {
  static inline void enable() noexcept {
    uint64_t cr0, cr4;

    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1ULL << 2); // Clear EM (Emulation)
    cr0 |= (1ULL << 1);  // Set MP (Monitor Coprocessor)
    asm volatile("mov %0, %%cr0" : : "r"(cr0));

    asm volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1ULL << 9);  // OSFXSR
    cr4 |= (1ULL << 10); // OSXMMEXCPT
    cr4 |= (1ULL << 18); // OSXSAVE
    asm volatile("mov %0, %%cr4" : : "r"(cr4));

    uint32_t eax, edx;
    asm volatile("xgetbv" : "=a"(eax), "=d"(edx) : "c"(0));
    eax |= 0x07; // Enable x87, SSE, and AVX state
    asm volatile("xsetbv" : : "a"(eax), "d"(edx), "c"(0));
  }
};

} // namespace frosty::hw
