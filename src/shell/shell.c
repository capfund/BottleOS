#include "shell.h"
#include "../clib/clib.h"
#include "../commands/commands.h"
#include "../kernel.h"
#include "../keyboard/keyboard.h"
#include "../vga/vga.h"

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
    } else {
      vga_putstr("Unknown command\n", WHITE_ON_BLACK);
    }
  }
}

static void shell_handle_input(char c) {
  if (c == '\n') {
    input_buffer[input_pos] = '\0';
    vga_putchar('\n', WHITE_ON_BLACK);

    char *argv[MAX_ARGS];
    int argc;
    shell_parse_input(input_buffer, argv, &argc);
    shell_execute_command(argc, argv);

    input_pos = 0;
    vga_putstr("> ", WHITE_ON_BLACK);
  } else if (c == '\b') {
    if (input_pos > 0) {
      input_pos--;
      unsigned int row = vga_get_cursor_row();
      unsigned int col = vga_get_cursor_col();
      if (col > 0) {
        vga_set_cursor(row, col - 1);
      } else if (row > 0) {
        vga_set_cursor(row - 1, VGA_MEM_WIDTH - 1);
      }
      vga_putchar(' ', WHITE_ON_BLACK);
      if (col > 0) {
        vga_set_cursor(row, col - 1);
      } else if (row > 0) {
        vga_set_cursor(row - 1, VGA_MEM_WIDTH - 1);
      }
    }
  } else {
    if (input_pos < INPUT_BUFFER_SIZE - 1) {
      input_buffer[input_pos++] = c;
      vga_putchar(c, WHITE_ON_BLACK);
    }
  }
}

void shell_start(void) {

  vga_putstr("> ", WHITE_ON_BLACK);

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
        vga_putchar('\n', WHITE_ON_BLACK);
        vga_putstr("> ", WHITE_ON_BLACK);
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
