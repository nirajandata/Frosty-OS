#pragma once
#include <cstddef>
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

struct point {
  int x, y;
};

} // namespace frosty
