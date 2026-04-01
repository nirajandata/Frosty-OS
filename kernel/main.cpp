#include "arch/simd.hpp"
#include "drivers/input.hpp"
#include "gfx/canvas.hpp"
#include "gui/theme.hpp"

namespace frosty {
struct stroke_meta {
  int start;
  float x0, y0, x1, y1;
};
constinit static draw_point doodle[20000];
constinit static stroke_meta strokes[512];
constinit static int p_idx = 0, s_idx = 0;
constinit static bool drawing = false;
} // namespace frosty

static volatile limine_framebuffer_request fb_req = {
    LIMINE_FRAMEBUFFER_REQUEST_ID, 0};
static volatile limine_memmap_request mm_req = {LIMINE_MEMMAP_REQUEST_ID, 0};
static volatile limine_hhdm_request hh_req = {LIMINE_HHDM_REQUEST_ID, 0};

extern "C" void kmain() {
  using namespace frosty;
  hw::simd::enable();

  if (!fb_req.response || !mm_req.response || !hh_req.response)
    while (true)
      hw::cpu::hlt();

  auto *fb = fb_req.response->framebuffers[0];
  size_t buffer_sz = fb->width * fb->height * 4;
  uint32_t *l1 = nullptr, *l2 = nullptr;

  for (size_t i = 0; i < mm_req.response->entry_count; i++) {
    auto *e = mm_req.response->entries[i];
    if (e->type == LIMINE_MEMMAP_USABLE && e->length >= buffer_sz * 2) {
      l1 = (uint32_t *)(e->base + hh_req.response->offset);
      l2 = (uint32_t *)(e->base + hh_req.response->offset + buffer_sz);
      break;
    }
  }

  if (!l1)
    while (true)
      hw::cpu::hlt();

  gfx::canvas screen{fb, l1, l2};
  hw::input_handler input{};
  input.init(0, 0, screen.width(), screen.height());
  gui::state ui;
  ui.init();
  screen.fill(screen.art(), color::pink);

  while (true) {
    input.update();
    if (input.size_up && ui.brush_size < 99) {
      ui.brush_size += 2;
      input.size_up = false;
    }
    if (input.size_down && ui.brush_size > 2) {
      ui.brush_size -= 2;
      input.size_down = false;
    }

    float mx = (float)input.mouse_x, my = (float)input.mouse_y;
    float box_w = (screen.width() - 120.0f) / 10.0f, bar_h = 45.0f,
          p_w = 110.0f, p_h = 110.0f;

    int hover_idx = (int)((mx - 110.0f) / box_w);
    if (hover_idx < 0)
      hover_idx = 0;
    if (hover_idx > 9)
      hover_idx = 9;

    int active_cur =
        (input.active_tool - 1 < 0)
            ? 0
            : (input.active_tool - 1 > 9 ? 9 : input.active_tool - 1);
    ui.active_idx = active_cur;

    bool on_bar = (my < bar_h && mx > 110.0f);
    float px = (on_bar ? (float)hover_idx : (float)active_cur) * box_w + 110.0f;
    if (px + p_w > screen.width())
      px = screen.width() - p_w;

    bool on_p = (mx >= px && mx < px + p_w && my >= bar_h && my < bar_h + p_h);

    if (input.left_click) {
      if (on_bar) {
        input.active_tool = hover_idx + 1;
      } else if (on_p) {
        float lx = mx - px, ly = my - bar_h - 5.0f;
        int target = on_bar ? hover_idx : active_cur;
        if (lx > 85.0f)
          ui.slots[target].h = (int)((ly * 359.0f) / 80.0f);
        else if (lx > 5.0f && lx < 85.0f && ly >= 0.0f && ly < 80.0f) {
          ui.slots[target].s = (int)((lx * 255.0f) / 80.0f);
          ui.slots[target].v = 255 - (int)((ly * 255.0f) / 80.0f);
        }
        ui.slots[target].update();
      } else if (my > bar_h) {
        if (!drawing && s_idx < 511) {
          strokes[s_idx++] = {p_idx, mx, my, mx, my};
          drawing = true;
        }
        if (p_idx < 19999) {
          doodle[p_idx++] = {(int)mx, (int)my,
                             ui.slots[active_cur].current_color, ui.brush_size};
          auto &m = strokes[s_idx - 1];
          float r = (float)ui.brush_size;
          if (mx - r < m.x0)
            m.x0 = mx - r;
          if (my - r < m.y0)
            m.y0 = my - r;
          if (mx + r > m.x1)
            m.x1 = mx + r;
          if (my + r > m.y1)
            m.y1 = my + r;
          if (p_idx > m.start + 1) {
            screen.line(screen.art(), (float)doodle[p_idx - 2].x,
                        (float)doodle[p_idx - 2].y, (float)doodle[p_idx - 1].x,
                        (float)doodle[p_idx - 1].y, (float)ui.brush_size,
                        (color)ui.slots[active_cur].current_color);
          }
        }
      }
    } else
      drawing = false;

    if (input.ctrl && input.z && s_idx > 0) {
      auto &d = strokes[--s_idx];
      p_idx = d.start;
      input.z = false;
      screen.rect(screen.art(), d.x0, d.y0, d.x1 - d.x0 + 1.0f,
                  d.y1 - d.y0 + 1.0f, color::pink);
      for (int s = 0; s < s_idx; ++s) {
        if (strokes[s].x1 < d.x0 || strokes[s].x0 > d.x1 ||
            strokes[s].y1 < d.y0 || strokes[s].y0 > d.y1)
          continue;
        int start = strokes[s].start,
            end = (s + 1 < s_idx) ? strokes[s + 1].start : p_idx;
        for (int i = start + 1; i < end; ++i)
          screen.line(
              screen.art(), (float)doodle[i - 1].x, (float)doodle[i - 1].y,
              (float)doodle[i].x, (float)doodle[i].y, (float)doodle[i].size,
              (color)doodle[i].c, (int)d.x0, (int)d.y0, (int)d.x1, (int)d.y1);
      }
    }

    screen.blit_full(screen.art(), screen.bb());
    screen.rect(screen.bb(), 0.0f, 0.0f, (float)screen.width(), bar_h,
                color::gui_bg);
    screen.text(screen.bb(), 10.0f, 10.0f, "FROSTY", color::black);
    screen.logo(screen.bb(), 38.0f, 26.0f, color::cyan);

    for (int i = 0; i < 10; i++) {
      float bx = 110.0f + ((float)i * box_w);
      screen.rect(screen.bb(), bx + 2.0f, 5.0f, box_w - 4.0f, 30.0f,
                  (color)ui.slots[i].current_color);
      if (ui.active_idx == i)
        screen.rect(screen.bb(), bx + 2.0f, 38.0f, box_w - 4.0f, 3.0f,
                    color::white);
    }

    if (on_bar || on_p) {
      int t = on_bar ? hover_idx : active_cur;
      screen.rect(screen.bb(), px, bar_h, p_w, p_h, color::gui_bg);
      for (float y = 0.0f; y < 80.0f; y += 4.0f)
        for (float x = 0.0f; x < 80.0f; x += 4.0f)
          screen.rect(screen.bb(), px + 5.0f + x, bar_h + 5.0f + y, 4.0f, 4.0f,
                      (color)hsv_to_rgb((float)ui.slots[t].h,
                                        (x * 255.0f) / 80.0f,
                                        255.0f - ((y * 255.0f) / 80.0f)));
      for (int y = 0; y < 80; y++) {
        uint32_t hc =
            hsv_to_rgb((float)ui.slots[t].h, 255.0f, 255.0f); // Hue bar visual
        for (int x = 0; x < 15; ++x)
          screen.px(screen.bb(), (int)px + 90 + x, (int)bar_h + 5 + y,
                    (color)hsv_to_rgb((y * 359.0f) / 80.0f, 255.0f, 255.0f));
      }
    }

    float sw = (float)screen.width(), sh = (float)screen.height();
    screen.rect(screen.bb(), sw - 150.0f, sh - 40.0f, 140.0f, 35.0f,
                color::gui_bg);
    screen.text(screen.bb(), sw - 145.0f, sh - 28.0f, "SIZE", color::black);
    screen.rect(screen.bb(), sw - 100.0f, sh - 25.0f,
                ((float)ui.brush_size * 80.0f) / 100.0f, 8.0f, color::black);
    screen.rect(screen.bb(), mx - 1.0f, my - 1.0f, 3.0f, 3.0f, color::white);

    screen.swap();
    hw::cpu::delay(5000);
  }
}
