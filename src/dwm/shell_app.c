#include "shell_app.h"
#include "../graphics/graphics.h"
#include "../keyboard/keyboard.h"
#include "../clib/clib.h"

#define INPUT_BUFFER_SIZE 128
#define MAX_HISTORY_LINES 32
#define LINE_HEIGHT 10

static char input_buffer[INPUT_BUFFER_SIZE];
static int input_pos = 0;
static char history[MAX_HISTORY_LINES][128];
static int history_count = 0;

static void history_push(const char *s) {
    if (history_count < MAX_HISTORY_LINES) {
        strncpy(history[history_count++], s, sizeof(history[0]) - 1);
        history[history_count-1][sizeof(history[0]) - 1] = '\0';
    } else {
        // roll
        for (int i = 1; i < MAX_HISTORY_LINES; ++i) strncpy(history[i-1], history[i], sizeof(history[0]));
        strncpy(history[MAX_HISTORY_LINES-1], s, sizeof(history[0]) - 1);
        history[MAX_HISTORY_LINES-1][sizeof(history[0]) - 1] = '\0';
    }
}

static void process_command(const char *cmd) {
    if (strcmp(cmd, "") == 0) return;
    if (strcmp(cmd, "hello") == 0) {
        history_push("Hello from VESA shell!");
    } else if (strcmp(cmd, "clear") == 0) {
        history_count = 0;
    } else if (strncmp(cmd, "echo ", 5) == 0) {
        history_push(cmd + 5);
    } else {
        history_push("Unknown command");
    }
}

void shell_app_draw(void) {
    // This function is called with the graphics target set to the window buffer.
    graphics_clear_screen(RGB(30,30,30));

    // Poll keyboard and update input buffer (non-blocking)
    unsigned char scancode = keyboard_get_scancode();
    if (scancode) {
        keyboard_handle_modifier(scancode);
        if (!(scancode & 0x80)) {
            char c = keyboard_scancode_to_ascii(scancode);
            if (c) {
                if (c == '\n') {
                    input_buffer[input_pos] = '\0';
                    history_push(input_buffer);
                    process_command(input_buffer);
                    input_pos = 0;
                    input_buffer[0] = '\0';
                } else if (c == '\b') {
                    if (input_pos > 0) input_pos--;
                    input_buffer[input_pos] = '\0';
                } else {
                    if (input_pos < (INPUT_BUFFER_SIZE - 1)) {
                        input_buffer[input_pos++] = c;
                        input_buffer[input_pos] = '\0';
                    }
                }
            }
        }
    }

    // Render history
    int y = 6;
    int start = history_count > 16 ? history_count - 16 : 0;
    for (int i = start; i < history_count; ++i) {
        graphics_draw_string(6, y, history[i], RGB(220,220,220), 1);
        y += LINE_HEIGHT;
    }

    // Render prompt and current input
    char prompt[INPUT_BUFFER_SIZE + 4];
    strncpy(prompt, "> ", 3);
    strncat(prompt, input_buffer, sizeof(prompt) - 3);
    graphics_draw_string(6, 200 - 18, prompt, RGB(200,255,200), 1);
}
