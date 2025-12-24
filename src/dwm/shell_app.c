#include "shell_app.h"
#include "dwm.h"
#include "../graphics/graphics.h"
#include "../keyboard/keyboard.h"
#include "../clib/clib.h"
#include "event.h"
#include "../fs/fs.h"
#include "../rtc/rtc.h"

#define INPUT_BUFFER_SIZE 128
#define MAX_HISTORY_LINES 32
#define LINE_HEIGHT 10

#define SHELL_MAX_INSTANCES 16

typedef struct {
    int used;
    int id; // window id
    char input_buffer[INPUT_BUFFER_SIZE];
    int input_pos;
    char history[MAX_HISTORY_LINES][128];
    int history_count;
} ShellInstance;

static ShellInstance instances[SHELL_MAX_INSTANCES];

static void history_push(ShellInstance *inst, const char *s) {
    if (!inst) return;
    if (inst->history_count < MAX_HISTORY_LINES) {
        strncpy(inst->history[inst->history_count++], s, sizeof(inst->history[0]) - 1);
        inst->history[inst->history_count-1][sizeof(inst->history[0]) - 1] = '\0';
    } else {
        // roll
        for (int i = 1; i < MAX_HISTORY_LINES; ++i) strncpy(inst->history[i-1], inst->history[i], sizeof(inst->history[0]));
        strncpy(inst->history[MAX_HISTORY_LINES-1], s, sizeof(inst->history[0]) - 1);
        inst->history[MAX_HISTORY_LINES-1][sizeof(inst->history[0]) - 1] = '\0';
    }
}

static void write_2d(char *buf, unsigned int v) {
    buf[0] = '0' + (v / 10);
    buf[1] = '0' + (v % 10);
}

static void shell_print_time(ShellInstance *inst) {
    struct rtc_time t;
    char buf[64];
    char yearbuf[8];

    rtc_read(&t);

    // Format: HH:MM:SS  DD/MM/YYYY
    write_2d(&buf[0],  t.hour);
    buf[2] = ':';
    write_2d(&buf[3],  t.minute);
    buf[5] = ':';
    write_2d(&buf[6],  t.second);
    buf[8] = ' ';
    buf[9] = ' ';
    write_2d(&buf[10], t.day);
    buf[12] = '/';
    write_2d(&buf[13], t.month);
    buf[15] = '/';

    utoa_dec(t.year, yearbuf);

    // Copy year
    int i = 0;
    while (yearbuf[i]) {
        buf[16 + i] = yearbuf[i];
        i++;
    }

    buf[16 + i] = '\0';

    history_push(inst, buf);
}

static void shell_print_unixtime(ShellInstance *inst) {
    struct rtc_time t;
    char buf[32];
    char tmp[32];
    int i = 0, j = 0;

    rtc_read(&t);

    uint32_t ts = (uint32_t)rtc_to_unix(&t);

    if (ts == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        history_push(inst, buf);
        return;
    }

    while (ts > 0) {
        tmp[i++] = '0' + (ts % 10);
        ts /= 10;
    }

    while (i > 0) {
        buf[j++] = tmp[--i];
    }

    buf[j] = '\0';

    history_push(inst, buf);
}

/* Internal Commands */
static void shell_read_file(ShellInstance *inst, const char *filename) {
    char buffer[FS_BLOCK_SIZE];
    int read_bytes = fs_read_file(filename, (uint8_t *)buffer, sizeof(buffer));

    if (read_bytes < 0) {
        history_push(inst, strcat("Error reading file; no read bytes", read_bytes == -1 ? " (file not found)" : " (buffer too small)"));
        return;
    }

    for (int i = 0; i < read_bytes; i++) {
        char tmp[2] = { buffer[i], '\0' };
        history_push(inst, tmp);
    }
}


static void process_command(ShellInstance *inst, const char *cmd) {
    if (!inst) return;
    if (strcmp(cmd, "") == 0) return;
    if (strcmp(cmd, "hello") == 0) {
        history_push(inst, "Hello from VESA shell!");
    } else if (strcmp(cmd, "clear") == 0) {
        inst->history_count = 0;
    } else if (strncmp(cmd, "echo ", 5) == 0) {
        history_push(inst, cmd + 5);
    } else if (strcmp(cmd, "time") == 0) {
        shell_print_time(inst);
    } else if (strcmp(cmd, "unixtime") == 0) {
        shell_print_unixtime(inst);
    } else if (strncmp(cmd, "read", 4) == 0 && (cmd[4] == ' ' || cmd[4] == '\0')) {
        // Skip spaces to get to filename
        const char *filename = cmd + 4;
        while (*filename == ' ') filename++;  // skip all spaces

        if (*filename == '\0') {
            history_push(inst, "Usage: read <filename>");
        } else {
            shell_read_file(inst, filename);
        }
    } else if (strcmp(cmd, "pwd") == 0) {
        history_push(inst, fs_get_current_dir());
    } else if (strcmp(cmd, "ls") == 0) {
        char *listing = fs_list_files_str();
        if (listing) {
            history_push(inst, listing);
            free(listing);
        }
    } else {
        history_push(inst, "Unknown command");
    }
}

void shell_app_draw(void) {
    // This function is called with the graphics target set to the window buffer.
    graphics_clear_screen(RGB(30,30,30));
    // Lookup per-window instance state using the DWM stable window id
    int win_id = dwm_get_current_window_id();
    if (win_id < 0) {
        // not rendering a window (shouldn't normally happen)
        return;
    }

    ShellInstance *inst = NULL;
    for (int i = 0; i < SHELL_MAX_INSTANCES; ++i) {
        if (instances[i].used && instances[i].id == win_id) {
            inst = &instances[i];
            break;
        }
    }
    if (!inst) {
        // allocate new instance slot
        for (int i = 0; i < SHELL_MAX_INSTANCES; ++i) {
            if (!instances[i].used) {
                instances[i].used = 1;
                instances[i].id = win_id;
                instances[i].input_buffer[0] = '\0';
                instances[i].input_pos = 0;
                instances[i].history_count = 0;
                inst = &instances[i];
                break;
            }
        }
    }
    if (!inst) return; // no slot available

    // Consume key events from central queue when this window is focused
    if (dwm_is_current_window_focused()) {
        InputEvent ev;
        while (event_pop(&ev)) {
            if (ev.type != EVENT_KEY) {
                // re-queue non-key events for the WM
                event_push(&ev);
                break;
            }
            unsigned char scancode = ev.u.key.scancode;
            keyboard_handle_modifier(scancode);
            if (!(scancode & 0x80)) {
                char c = keyboard_scancode_to_ascii(scancode);
                if (c) {
                    if (c == '\n') {
                        inst->input_buffer[inst->input_pos] = '\0';
                        history_push(inst, inst->input_buffer);
                        process_command(inst, inst->input_buffer);
                        inst->input_pos = 0;
                        inst->input_buffer[0] = '\0';
                    } else if (c == '\b') {
                        if (inst->input_pos > 0) inst->input_pos--;
                        inst->input_buffer[inst->input_pos] = '\0';
                    } else {
                        if (inst->input_pos < (INPUT_BUFFER_SIZE - 1)) {
                            inst->input_buffer[inst->input_pos++] = c;
                            inst->input_buffer[inst->input_pos] = '\0';
                        }
                    }
                }
            }
        }
    }

    // Render history
    int y = 6;
    int start = inst->history_count > 16 ? inst->history_count - 16 : 0;
    for (int i = start; i < inst->history_count; ++i) {
        graphics_draw_string(6, y, inst->history[i], RGB(220,220,220), 1);
        y += LINE_HEIGHT;
    }

    // Render prompt and current input
    char prompt[INPUT_BUFFER_SIZE + 4];
    strncpy(prompt, "> ", 3);
    strncat(prompt, inst->input_buffer, sizeof(prompt) - 3);
    graphics_draw_string(6, 200 - 18, prompt, RGB(200,255,200), 1);
}
