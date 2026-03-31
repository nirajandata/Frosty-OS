#pragma once
#include "../gfx/font.hpp"
#include "../gfx/types.hpp"
#include "widget.hpp"

namespace frosty::gui {
class window : public widget {
public:
  const char *title;
  bool dragging{false};
  int ox, oy;

  window(int x, int y, int w, int h, const char *t) : title(t) {
    r = {x, y, w, h};
  }

  void draw(gfx::canvas &s) override {
    if (!visible)
      return;
    s.draw_rect(r.x, r.y, r.w, r.h, frosty::color::gui_border);
    s.draw_rect(r.x, r.y, r.w, 20, frosty::color::mint);
  }

  bool on_mouse(int mx, int my, bool down) override {
    if (!visible)
      return false;

    if (down && mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + 20) {
      dragging = true;
      ox = mx - r.x;
      oy = my - r.y;
      return true;
    }

    if (!down)
      dragging = false;

    if (dragging) {
      r.x = mx - ox;
      r.y = my - oy;
      return true;
    }
    return is_inside(mx, my);
  }
};
} // namespace frosty::gui
