#pragma once
#include "../gfx/types.hpp"

namespace frosty::gui {
struct slot {
  int h, s, v;
  uint32_t current_color;
  void update() { current_color = static_cast<uint32_t>(hsv_to_rgb(h, s, v)); }
};

struct state {
  slot slots[10];
  int active_idx = 0;
  uint8_t brush_size = 5;

  void init() {
    for (int i = 0; i < 10; i++) {
      slots[i] = {i * 36, 255, 255, 0};
      slots[i].update();
    }
  }
};
} // namespace frosty::gui
