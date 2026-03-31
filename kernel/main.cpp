#include "limine.h"
#include <stddef.h>
#include <stdint.h>

static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID, .revision = 0};

static inline void outb(uint16_t port, uint8_t val) {
  asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
  uint8_t ret;
  asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

void serial_print(const char *str) {
  for (size_t i = 0; str[i] != '\0'; i++) {
    outb(0x3f8, str[i]);
  }
}

void draw_rect(struct limine_framebuffer *fb, int x, int y, int w, int h,
               uint32_t color) {
  uint32_t *fb_ptr = (uint32_t *)fb->address;
  for (int i = y; i < y + h; i++) {
    for (int j = x; j < x + w; j++) {
      if (j >= 0 && j < (int)fb->width && i >= 0 && i < (int)fb->height) {
        fb_ptr[i * (fb->pitch / 4) + j] = color;
      }
    }
  }
}

extern "C" void kmain(void) {
  serial_print("\n--- Booting Animation ---\n");

  if (framebuffer_request.response == NULL ||
      framebuffer_request.response->framebuffer_count < 1) {
    for (;;)
      asm("hlt");
  }

  struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
  uint32_t pink = 0xAAADD1;
  uint32_t white = 0xFFFFFF;

  int box_x = 100;
  int box_y = 100;
  int dx = 5;
  int dy = 5;
  int size = 60;

  for (;;) {
    draw_rect(fb, 0, 0, fb->width, fb->height, pink);

    draw_rect(fb, box_x, box_y, size, size, white);
    draw_rect(fb, box_x + 15, box_y + 15, 10, 10, 0x000000);
    draw_rect(fb, box_x + 35, box_y + 15, 10, 10, 0x000000);
    draw_rect(fb, box_x + 25, box_y + 35, 10, 5, 0xEE2777);

    box_x = box_x + dx;
    box_y = box_y + dy;

    if (box_x <= 0 || box_x + size >= (int)fb->width) {
      dx = -dx;
    }
    if (box_y <= 0 || box_y + size >= (int)fb->height) {
      dy = -dy;
    }
  }
}
