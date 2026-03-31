#include "drivers/input.hpp"
#include "gfx/canvas.hpp"
#include "gui/theme.hpp"

namespace frosty {
constinit static draw_point doodle[20000] = {};
constinit static int strokes[512], p_idx = 0, s_idx = 0;
constinit static bool drawing = false;
} // namespace frosty

static volatile limine_framebuffer_request fb_req = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID, .revision = 0, .response = nullptr};

extern "C" void kmain() {
  using namespace frosty;
  if (!fb_req.response)
    while (true)
      hw::cpu::hlt();

  gfx::canvas screen{fb_req.response->framebuffers[0]};
  hw::input_handler input{};
  input.init(0, 0, screen.width(), screen.height());

  gui::state ui;
  ui.init();

  while (true) {
    screen.clear(color::pink);
    input.update();

    if (input.size_up) {
      if (ui.brush_size < 99)
        ui.brush_size += 2;
      input.size_up = false;
    }
    if (input.size_down) {
      if (ui.brush_size > 2)
        ui.brush_size -= 2;
      input.size_down = false;
    }

    const int box_w = (screen.width() - 120) / 10;
    const int bar_h = 45;
    const int picker_w = 110;
    const int picker_h = 110;

    int cur = input.active_tool - 1;
    if (cur < 0)
      cur = 0;
    if (cur > 9)
      cur = 9;
    ui.active_idx = cur;

    int px = (cur * box_w) + 110;
    if (px + picker_w > screen.width())
      px = screen.width() - picker_w;

    bool on_bar = (input.mouse_y < bar_h);
    bool on_picker =
        (input.mouse_x >= px && input.mouse_x < px + picker_w &&
         input.mouse_y >= bar_h && input.mouse_y < bar_h + picker_h);

    if (input.left_click) {
      if (on_bar && input.mouse_x > 110) {
        input.active_tool = ((input.mouse_x - 110) / box_w) + 1;
      } else if (on_picker) {
        int local_x = input.mouse_x - px;
        int local_y = input.mouse_y - bar_h - 5;

        if (local_x > 85) {
          ui.slots[cur].h = (local_y * 359) / 80;
        } else if (local_x > 5 && local_x < 85 && local_y >= 0 &&
                   local_y < 80) { // Sat/Val square
          ui.slots[cur].s = (local_x * 255) / 80;
          ui.slots[cur].v = 255 - (local_y * 255) / 80;
        }

        if (ui.slots[cur].h < 0)
          ui.slots[cur].h = 0;
        if (ui.slots[cur].h > 359)
          ui.slots[cur].h = 359;
        ui.slots[cur].update();
      } else if (input.mouse_y > bar_h) {
        // Drawing logic
        if (!drawing && s_idx < 511) {
          strokes[s_idx++] = p_idx;
          drawing = true;
        }
        if (p_idx < 19999) {
          doodle[p_idx++] =
              draw_point(input.mouse_x, input.mouse_y,
                         ui.slots[cur].current_color, ui.brush_size);
        }
      }
    } else {
      drawing = false;
    }

    if (input.ctrl && input.z && s_idx > 0) {
      p_idx = strokes[--s_idx];
      input.z = false;
    }

    for (int s = 0; s < s_idx; ++s) {
      int start = strokes[s], end = (s + 1 < s_idx) ? strokes[s + 1] : p_idx;
      for (int i = start + 1; i < end; ++i)
        screen.draw_line(doodle[i - 1].x, doodle[i - 1].y, doodle[i].x,
                         doodle[i].y, doodle[i].size, (color)doodle[i].c);
    }

    screen.draw_rect(0, 0, screen.width(), bar_h, color::gui_bg);
    screen.draw_text(10, 18, "FROSTY", color::black);

    for (int i = 0; i < 10; i++) {
      int bx = 110 + (i * box_w);
      screen.draw_rect(bx + 2, 5, box_w - 4, 30,
                       (color)ui.slots[i].current_color);
      if (ui.active_idx == i)
        screen.draw_rect(bx + 2, 38, box_w - 4, 3, color::white);
    }

    if (on_bar || on_picker) {
      screen.draw_rect(px, bar_h, picker_w, picker_h, color::gui_bg);
      for (int y = 0; y < 80; y += 4) {
        for (int x = 0; x < 80; x += 4) {
          uint32_t c = hsv_to_rgb(ui.slots[cur].h, (x * 255) / 80,
                                  255 - ((y * 255) / 80));
          screen.draw_rect(px + 5 + x, bar_h + 5 + y, 4, 4, (color)c);
        }
      }

      for (int y = 0; y < 80; y++) {
        uint32_t hue_col = hsv_to_rgb((y * 359) / 80, 255, 255);
        screen.draw_line(px + 90, bar_h + 5 + y, px + 105, bar_h + 5 + y, 1,
                         (color)hue_col);
      }
      screen.draw_text(px + 5, bar_h + 90, "PICKER", color::black);
    }

    int sw = screen.width();
    int sh = screen.height();
    screen.draw_rect(sw - 150, sh - 40, 140, 35, color::gui_bg);
    screen.draw_text(sw - 145, sh - 28, "SIZE", color::black);
    screen.draw_rect(sw - 100, sh - 25, (ui.brush_size * 80) / 100, 8,
                     color::white);

    screen.draw_rect(input.mouse_x - 1, input.mouse_y - 1, 3, 3, color::white);

    screen.swap();
    hw::cpu::delay(1000000);
  }
}
