#pragma once
#include <stdint.h>

namespace frosty::math {

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

} // namespace frosty::math
