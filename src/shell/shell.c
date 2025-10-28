#include "shell.h"
#include "../clib/clib.h"
#include "../commands/commands.h"
#include "../fs/fs.h"
#include "../kernel.h"
#include "../keyboard/keyboard.h"
#include "../display/display.h"
#include "../vesa/vesa.h"

static char input_buffer[INPUT_BUFFER_SIZE];
static unsigned int input_pos = 0;

static void shell_parse_input(char *input, char *argv[], int *argc) {
  *argc = 0;
  char *p = input;
  
  while (*p && *argc < MAX_ARGS) {
    while (*p == ' ')
      p++; // skip spaces
    if (*p == '\0')
      break;
    argv[*argc] = p;
    (*argc)++;
    while (*p && *p != ' ')
      p++;
    if (*p)
      *p++ = '\0'; // terminate argument
  }
}

static void shell_execute_command(int argc, char *argv[]) {
  if (argc > 0) {
    if (strcmp(argv[0], "hello") == 0) {
      cmd_hello();
    } else if (strcmp(argv[0], "clear") == 0) {
      cmd_clear();
    } else if (strcmp(argv[0], "echo") == 0) {
      cmd_echo(argc, argv);
    } else if (strcmp(argv[0], "bye") == 0 || strcmp(argv[0], "exit") == 0 ||
               strcmp(argv[0], "shutdown") == 0) {
      cmd_bye(argc, argv);
    } else if (strcmp(argv[0], "theme") == 0) {
      cmd_theme(argc, argv);
    }
    // fs
    else if (strcmp(argv[0], "ls") == 0) {
      cmd_ls();
    } else if (strcmp(argv[0], "touch") == 0) {
      cmd_touch(argc, argv);
    } else if (strcmp(argv[0], "cat") == 0) {
      cmd_cat(argc, argv);
    } else if (strcmp(argv[0], "write") == 0) {
      cmd_write(argc, argv);
    } else if (strcmp(argv[0], "mkdir") == 0) {
      cmd_mkdir(argc, argv);
    } else if (strcmp(argv[0], "rmdir") == 0) {
      cmd_rmdir(argc, argv);
    } else if (strcmp(argv[0], "rm") == 0) {
      cmd_rm(argc, argv);
    } else if (strcmp(argv[0], "cd") == 0) {
      cmd_cd(argc, argv);
    } else if (strcmp(argv[0], "pwd") == 0) {
      cmd_pwd();
    } else {
      display_putstr("Unknown command\n", vesa_rgb(255, 0, 0));
    }
  }
}

static void shell_handle_input(char c) {
  if (c == '\n') {
    input_buffer[input_pos] = '\0';
    display_putchar('\n', vesa_rgb(255, 255, 255));

    char *argv[MAX_ARGS];
    int argc;
    shell_parse_input(input_buffer, argv, &argc);
    shell_execute_command(argc, argv);

    input_pos = 0;
    display_putstr(fs_get_current_dir(), vesa_rgb(0, 255, 0));
    display_putstr(" > ", vesa_rgb(255, 255, 255));
  } else if (c == '\b') {
    if (input_pos > 0) {
      input_pos--;
      
      // Simple backspace: print backspace, space, then backspace again
      display_putchar('\b', vesa_rgb(255, 255, 255)); // Move cursor back
      display_putchar(' ', vesa_rgb(255, 255, 255));  // Erase character
      display_putchar('\b', vesa_rgb(255, 255, 255)); // Move cursor back again
    }
  } else {
    if (input_pos < INPUT_BUFFER_SIZE - 1) {
      input_buffer[input_pos++] = c;
      display_putchar(c, vesa_rgb(255, 255, 255));
    }
  }
}

void shell_start(void) {
  display_putstr("> ", vesa_rgb(255, 255, 255));

  while (1) {
    unsigned char scancode = keyboard_get_scancode();
    keyboard_handle_modifier(scancode);

    if (scancode & 0x80)
      continue; // ignore key releases

    char c = keyboard_scancode_to_ascii(scancode);
    if (!c)
      continue;

    // Handle Ctrl shortcuts
    if (keyboard_is_ctrl_pressed()) {
      switch (c) {
      case 'c': // Ctrl+C clears input
        input_pos = 0;
        display_putstr("\n> ", vesa_rgb(255, 255, 255));
        continue;
      case 'a': // Ctrl+A moves cursor to start
        input_pos = 0;
        continue;
      case 'x': // Ctrl+X
      case 'v': // Ctrl+V
        // Clipboard not implemented
        continue;
      }
    }

    shell_handle_input(c);
  }
}