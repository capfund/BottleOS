#include "commands.h"
#include "../clib/clib.h"
#include "../fs/fs.h"
#include "../kernel.h"
// #include "../vga/vga.h"
#include "../display/display.h"
#include "../vesa/vesa.h"

// void cmd_hello() { display_putstr("Hello, user!\n", color_green_on_black()); }
void cmd_hello() { display_putstr("Hello, user!\n", vesa_rgb(0, 255, 0)); }


void cmd_clear() { display_clear(); }

void cmd_write(int argc, char **argv) {
  if (argc < 3) {
    display_putstr("Usage: write <filename> <text>\n", vesa_rgb(255, 0, 0));
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
    display_putstr("write: error writing file\n", vesa_rgb(255, 0, 0));
  } else {
    display_putstr("File written successfully\n", vesa_rgb(0,255,0));
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
    display_putstr(argv[i], (uint32_t ) color_white_on_black());
    if (i < argc - 1)
      display_putchar(' ', (uint32_t) color_white_on_black());
  }
  if (newline)
    display_putchar('\n', (uint32_t) color_white_on_black());
}

void cmd_theme(int argc, char *argv[]) {
  if (argc < 2) {
    display_putstr("Usage: theme -l | -d\n", (uint32_t) color_green_on_black());
    return;
  }

  if (strcmp(argv[1], "-l") == 0) {
    light_mode = 1;
    display_clear();
    display_putstr("Welcome to BottleOS Shell [light]\n", (uint32_t) color_green_on_black());
  } else if (strcmp(argv[1], "-d") == 0) {
    light_mode = 0;
    display_clear();
    display_putstr("Welcome to BottleOS Shell [dark]\n", (uint32_t) color_green_on_black());
  } else {
    light_mode = !light_mode; // toggle the mode
    if (light_mode) {
      display_clear();
      display_putstr("Welcome to BottleOS Shell [light]\n", (uint32_t) color_green_on_black());
    } else {
      display_clear();
      display_putstr("Welcome to BottleOS Shell [dark] \n", (uint32_t) color_green_on_black());
    }
  }
}

void cmd_ls() { fs_list_files(); }

void cmd_touch(int argc, char *argv[]) {
  if (argc < 2) {
    display_putstr("Usage: touch <filename>\n", vesa_rgb(0,0,255));
    return;
  }
  fs_create_file(argv[1]);
}

void cmd_cat(int argc, char **argv) {
  if (argc < 2) {
    display_putstr("Usage: cat <filename>\n", vesa_rgb(0,0,255));
    return;
  }

  char buffer[FS_BLOCK_SIZE];
  int read_bytes = fs_read_file(argv[1], (uint8_t *)buffer, sizeof(buffer));

  if (read_bytes < 0) {
    display_putstr("cat: file not found or read error\n", vesa_rgb(255,0,0));
    return;
  }

  // Print file contents
  for (int i = 0; i < read_bytes; i++) {
    display_putchar(buffer[i], vesa_rgb(128, 128, 0));
  }

  display_putchar('\n', vesa_rgb(128,128,0) );
}

int cmd_bye(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  display_putstr("Shutting down...\n", (uint32_t) color_green_on_black());
  while (1) {
    __asm__("hlt");
  }
  return 0;
}

void cmd_mkdir(int argc, char *argv[]) {
  if (argc < 2) {
    display_putstr("Usage: mkdir <dirname>\n", vesa_rgb(0,0,255));
    return;
  }

  int result = fs_create_directory(argv[1]);
  if (result == -2) {
    display_putstr("mkdir: directory already exists\n", vesa_rgb(255, 255, 0));
  } else if (result < 0) {
    display_putstr("mkdir: error creating directory\n", vesa_rgb(255,0,0));
  } else {
    display_putstr("Directory created\n", vesa_rgb(0,255,0));
  }
}

void cmd_rmdir(int argc, char *argv[]) {
  if (argc < 2) {
    display_putstr("Usage: rmdir <dirname>\n", vesa_rgb(0,0,255));
    return;
  }

  int result = fs_delete_directory(argv[1]);
  if (result == -1) {
    display_putstr("rmdir: directory not found\n", vesa_rgb(255,0,0));
  } else if (result == -2) {
    display_putstr("rmdir: not a directory\n", vesa_rgb(255,0,0));
  } else if (result < 0) {
    display_putstr("rmdir: error deleting directory\n", vesa_rgb(255,0,0));
  } else {
    display_putstr("Directory deleted\n", vesa_rgb(0,255,0));
  }
}

void cmd_rm(int argc, char *argv[]) {
  if (argc < 2) {
    display_putstr("Usage: rm <filename>\n", vesa_rgb(0,0,255));
    return;
  }

  // Check if it's a directory
  if (fs_is_directory(argv[1]) == 1) {
    display_putstr("rm: cannot remove directory, use rmdir\n", vesa_rgb(255,0,0));
    return;
  }

  int result = fs_delete_file(argv[1]);
  if (result < 0) {
    display_putstr("rm: file not found\n", vesa_rgb(255,0,0));
  } else {
    display_putstr("File deleted\n", vesa_rgb(0,255,0));
  }
}

void cmd_cd(int argc, char *argv[]) {
  if (argc < 2) {
    // No argument - go to root
    fs_change_directory("/");
    return;
  }

  int result = fs_change_directory(argv[1]);
  if (result == -1) {
    display_putstr("cd: directory not found\n", vesa_rgb(255,0,0));
  } else if (result == -2) {
    display_putstr("cd: not a directory\n", vesa_rgb(255,0,0));
  }
  // Success is silent
}

void cmd_pwd(void) {
  display_putstr(fs_get_current_dir(), vesa_rgb(0,0,255));
  display_putchar('\n', vesa_rgb(0,0,255));
}
