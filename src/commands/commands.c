#include "commands.h"
#include "../clib/clib.h"
#include "../kernel.h"
#include "../vga/vga.h"

void cmd_hello() { vga_putstr("Hello, user!\n", GREEN_ON_BLACK); }

void cmd_clear() { vga_clear_screen(); }

void cmd_echo(int argc, char *argv[]) {
  int newline = 1;
  int start = 1;
  if (argc > 1 && strcmp(argv[1], "-n") == 0) {
    newline = 0;
    start = 2;
  }
  for (int i = start; i < argc; i++) {
    vga_putstr(argv[i], WHITE_ON_BLACK);
    if (i < argc - 1)
      vga_putchar(' ', WHITE_ON_BLACK);
  }
  if (newline)
    vga_putchar('\n', WHITE_ON_BLACK);
}

int cmd_bye(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  vga_putstr("Shutting down...\n", GREEN_ON_BLACK);
  while (1) {
    __asm__("hlt");
  }
  return 0;
}
