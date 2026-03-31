#include "drivers/input.hpp"
#include "drivers/speaker.hpp"
#include "gfx/canvas.hpp"

namespace frosty {
constinit static point doodle_data[20000] = {};
constinit static int strokes[512];
constinit static int p_idx = 0;
constinit static int s_idx = 0;
constinit static bool drawing = false;
} // namespace frosty

static volatile limine_framebuffer_request fb_req = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID, .revision = 0};

extern "C" void kmain() {
  using namespace frosty;
  if (!fb_req.response)
    while (true)
      hw::cpu::hlt();

  gfx::canvas screen{fb_req.response->framebuffers[0]};
  hw::input_handler input{};
  input.init(screen.width() / 2, screen.height() / 2, screen.width(),
             screen.height());

  const uint32_t scale[] = {261, 329, 392, 440, 523};
  auto frame = 0;

  while (true) {
    screen.clear(color::pink);
    input.update();

    if (input.mouse_x < 0)
      input.mouse_x = 0;
    if (input.mouse_y < 0)
      input.mouse_y = 0;
    if (input.mouse_x >= screen.width())
      input.mouse_x = screen.width() - 1;
    if (input.mouse_y >= screen.height())
      input.mouse_y = screen.height() - 1;

    if (input.left_click) {
      if (!drawing && s_idx < 511) {
        strokes[s_idx++] = p_idx;
        drawing = true;
      }
      if (p_idx < 19999) {
        if (p_idx == 0 || (doodle_data[p_idx - 1].x != input.mouse_x ||
                           doodle_data[p_idx - 1].y != input.mouse_y)) {
          doodle_data[p_idx++] = {input.mouse_x, input.mouse_y};
        }
      }
    } else {
      drawing = false;
    }

    if (input.ctrl && input.z) {
      if (s_idx > 0)
        p_idx = strokes[--s_idx];
      input.z = false;
    }

    for (int s = 0; s < s_idx; ++s) {
      int start = strokes[s];
      int end = (s + 1 < s_idx) ? strokes[s + 1] : p_idx;
      for (int i = start + 1; i < end; ++i) {
        screen.draw_line(doodle_data[i - 1].x, doodle_data[i - 1].y,
                         doodle_data[i].x, doodle_data[i].y, color::ink);
      }
    }

    const auto kitty_y = input.mouse_y + (math::sin(frame * 6) * 15 / 100);
    screen.draw_rect(input.mouse_x, kitty_y, 44, 44, color::white);
    screen.draw_rect(input.mouse_x + 8, kitty_y + 10, 8, 8, color::black);
    screen.draw_rect(input.mouse_x + 28, kitty_y + 10, 8, 8, color::black);
    screen.draw_rect(input.mouse_x + 17, kitty_y + 28, 10, 5, color::nose);

    screen.swap();

    frame++;

    hw::cpu::delay(2'000'000);
  }
}
