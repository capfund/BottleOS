#include "vga.h"
#include "../kernel.h"

static unsigned int cursor_row = 0;
static unsigned int cursor_col = 0;

void vga_clear_screen() {
  char *video_memory = VIDEO_MEMORY;
  for (unsigned int i = 0; i < VGA_MEM_WIDTH * VGA_MEM_HEIGHT; i++) {
    video_memory[i * 2] = ' ';
    video_memory[i * 2 + 1] = WHITE_ON_BLACK;
  }
  cursor_row = 0;
  cursor_col = 0;
}

static void vga_scroll() {
  char *video_memory = VIDEO_MEMORY;

  for (unsigned int row = 1; row < VGA_MEM_HEIGHT; row++) {
    for (unsigned int col = 0; col < VGA_MEM_WIDTH; col++) {
      video_memory[((row - 1) * VGA_MEM_WIDTH + col) * 2] =
          video_memory[(row * VGA_MEM_WIDTH + col) * 2];
      video_memory[((row - 1) * VGA_MEM_WIDTH + col) * 2 + 1] =
          video_memory[(row * VGA_MEM_WIDTH + col) * 2 + 1];
    }
  }

  for (unsigned int col = 0; col < VGA_MEM_WIDTH; col++) {
    video_memory[((VGA_MEM_HEIGHT - 1) * VGA_MEM_WIDTH + col) * 2] = ' ';
    video_memory[((VGA_MEM_HEIGHT - 1) * VGA_MEM_WIDTH + col) * 2 + 1] =
        WHITE_ON_BLACK;
  }

  if (cursor_row > 0)
    cursor_row--;
}

void vga_putchar(char c, unsigned char color) {
  char *video_memory = VIDEO_MEMORY;

  if (c == '\n') {
    cursor_col = 0;
    cursor_row++;
    if (cursor_row >= VGA_MEM_HEIGHT)
      vga_scroll();
    return;
  }

  video_memory[(cursor_row * VGA_MEM_WIDTH + cursor_col) * 2] = c;
  video_memory[(cursor_row * VGA_MEM_WIDTH + cursor_col) * 2 + 1] = color;

  cursor_col++;
  if (cursor_col >= VGA_MEM_WIDTH) {
    cursor_col = 0;
    cursor_row++;
    if (cursor_row >= VGA_MEM_HEIGHT)
      vga_scroll();
  }
}

void vga_putstr(const char *str, unsigned char color) {
  for (unsigned int i = 0; str[i] != '\0'; i++) {
    vga_putchar(str[i], color);
  }
}

unsigned int vga_get_cursor_row(void) { return cursor_row; }
unsigned int vga_get_cursor_col(void) { return cursor_col; }
void vga_set_cursor(unsigned int row, unsigned int col) {
  cursor_row = row;
  cursor_col = col;
}
