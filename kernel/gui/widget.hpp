#pragma once
#include "../gfx/canvas.hpp"

namespace frosty::gui {
struct point {
  int x, y;
};
struct rect {
  int x, y, w, h;
};

class widget {
public:
  rect r;
  bool visible{true};
  virtual void draw(gfx::canvas &s) = 0;
  virtual bool on_mouse(int mx, int my, bool down) = 0;
  bool is_inside(int mx, int my) {
    return (mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h);
  }
};
} // namespace frosty::gui
