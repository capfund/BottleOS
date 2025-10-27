#include "commands.h"
#include "../clib/clib.h"
#include "../fs/fs.h"
#include "../kernel.h"
#include "../vga/vga.h"

void cmd_hello() { vga_putstr("Hello, user!\n", color_green_on_black()); }

void cmd_clear() { vga_clear_screen(); }

void cmd_write(int argc, char **argv) {
  if (argc < 3) {
    vga_putstr("Usage: write <filename> <text>\n", 0x0E);
    return;
  }

  // Concatenate all arguments after filename into content
  char content[512];
  int pos = 0;
  for (int i = 2; i < argc && pos < 511; i++) {
    int j = 0;
    while (argv[i][j] && pos < 511) {
      content[pos++] = argv[i][j++];
    }
    if (i < argc - 1 && pos < 511) {
      content[pos++] = ' ';
    }
  }
  content[pos] = '\0';

  int result = fs_write_file(argv[1], (uint8_t *)content, pos);
  if (result < 0) {
    vga_putstr("write: error writing file\n", 0x0C);
  } else {
    vga_putstr("File written successfully\n", 0x0A);
  }
}

void cmd_echo(int argc, char *argv[]) {
  int newline = 1;
  int start = 1;
  if (argc > 1 && strcmp(argv[1], "-n") == 0) {
    newline = 0;
    start = 2;
  }
  for (int i = start; i < argc; i++) {
    vga_putstr(argv[i], color_white_on_black());
    if (i < argc - 1)
      vga_putchar(' ', color_white_on_black());
  }
  if (newline)
    vga_putchar('\n', color_white_on_black());
}

void cmd_theme(int argc, char *argv[]) {
  if (argc < 2) {
    vga_putstr("Usage: theme -l | -d\n", color_green_on_black());
    return;
  }

  if (strcmp(argv[1], "-l") == 0) {
    light_mode = 1;
    vga_clear_screen();
    vga_putstr("Welcome to BottleOS Shell [light]\n", color_green_on_black());
  } else if (strcmp(argv[1], "-d") == 0) {
    light_mode = 0;
    vga_clear_screen();
    vga_putstr("Welcome to BottleOS Shell [dark]\n", color_green_on_black());
  } else {
    light_mode = !light_mode; // toggle the mode
    if (light_mode) {
      vga_clear_screen();
      vga_putstr("Welcome to BottleOS Shell [light]\n", color_green_on_black());
    } else {
      vga_clear_screen();
      vga_putstr("Welcome to BottleOS Shell [dark] \n", color_green_on_black());
    }
  }
}

void cmd_ls() { fs_list_files(); }

void cmd_touch(int argc, char *argv[]) {
  if (argc < 2) {
    vga_putstr("Usage: touch <filename>\n", 0x0F);
    return;
  }
  fs_create_file(argv[1]);
}

void cmd_cat(int argc, char **argv) {
  if (argc < 2) {
    vga_putstr("Usage: cat <filename>\n", 0x0E);
    return;
  }

  char buffer[FS_BLOCK_SIZE];
  int read_bytes = fs_read_file(argv[1], (uint8_t *)buffer, sizeof(buffer));

  if (read_bytes < 0) {
    vga_putstr("cat: file not found or read error\n", 0x0C);
    return;
  }

  // Print file contents
  for (int i = 0; i < read_bytes; i++) {
    vga_putchar(buffer[i], 0x0F);
  }

  vga_putchar('\n', 0x0F);
}

int cmd_bye(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  vga_putstr("Shutting down...\n", color_green_on_black());
  while (1) {
    __asm__("hlt");
  }
  return 0;
}
