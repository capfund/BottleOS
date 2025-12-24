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
#define SHELL_WIDTH 40

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
    if (!inst || !s) return;

    const char *start = s;
    const char *p = s;

    while (1) {
        if (*p == '\n' || *p == '\0') {
            // Extract line from start..p
            char line[128];
            int len = p - start;
            if (len >= (int)sizeof(line))
                len = sizeof(line) - 1;
            for (int i = 0; i < len; i++)
                line[i] = start[i];
            line[len] = '\0';

            // Push line to history
            if (inst->history_count < MAX_HISTORY_LINES) {
                strncpy(inst->history[inst->history_count++], line, sizeof(inst->history[0]) - 1);
                inst->history[inst->history_count-1][sizeof(inst->history[0]) - 1] = '\0';
            } else {
                // Roll history
                for (int i = 1; i < MAX_HISTORY_LINES; ++i)
                    strncpy(inst->history[i-1], inst->history[i], sizeof(inst->history[0]));
                strncpy(inst->history[MAX_HISTORY_LINES-1], line, sizeof(inst->history[0]) - 1);
                inst->history[MAX_HISTORY_LINES-1][sizeof(inst->history[0]) - 1] = '\0';
            }

            if (*p == '\0')
                break; // done
            start = p + 1; // skip newline
        }
        p++;
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

    char *btmp = malloc(read_bytes + 1);
    if (!btmp) {
        history_push(inst, "Memory allocation error");
        return;
    }

    for (int i = 0; i < read_bytes; i++) {
        btmp[i] = buffer[i];
    }
    btmp[read_bytes] = '\0';
    history_push(inst, btmp);
    free(btmp);
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
    } else if (strncmp(cmd, "cat", 3) == 0 && (cmd[3] == ' ' || cmd[3] == '\0')) {
        // Skip spaces to get to filename
        const char *filename = cmd + 3;
        while (*filename == ' ') filename++;  // skip all spaces

        if (*filename == '\0') {
            history_push(inst, "Usage: cat <filename>");
        } else {
            shell_read_file(inst, filename);
        }
    } else if (strcmp(cmd, "pwd") == 0) {
        history_push(inst, fs_get_current_dir());
    } else if (strcmp(cmd, "ls") == 0) {
        char *listing = fs_list_files_str();
        char *ln = strtok(listing, "\n");
        while (ln) {
            history_push(inst, ln);
            ln = strtok(NULL, "\n");
        } 
        free(listing);
    } else {
        history_push(inst, "Unknown command");
    }
}

void shell_app_draw(void) {
    // Clear window buffer
    graphics_clear_screen(RGB(30,30,30));

    // Lookup current window instance
    int win_id = dwm_get_current_window_id();
    if (win_id < 0) return;

    ShellInstance *inst = NULL;
    for (int i = 0; i < SHELL_MAX_INSTANCES; ++i) {
        if (instances[i].used && instances[i].id == win_id) {
            inst = &instances[i];
            break;
        }
    }
    if (!inst) {
        for (int i = 0; i < SHELL_MAX_INSTANCES; ++i) {
            if (!instances[i].used) {
                instances[i].used = 1;
                instances[i].id = win_id;
                inst = &instances[i];
                inst->input_buffer[0] = '\0';
                inst->input_pos = 0;
                inst->history_count = 0;
                break;
            }
        }
    }
    if (!inst) return;

    // Consume key events
    if (dwm_is_current_window_focused()) {
        InputEvent ev;
        while (event_pop(&ev)) {
            if (ev.type != EVENT_KEY) {
                event_push(&ev);
                break;
            }
            unsigned char scancode = ev.u.key.scancode;
            keyboard_handle_modifier(scancode);
            if (!(scancode & 0x80)) { // key press
                char c = keyboard_scancode_to_ascii(scancode);
                if (!c) continue;

                if (c == '\n') {
                    // Push typed command as "bottleOS <cwd> > command"
                    if (inst->input_pos > 0) {
                        inst->input_buffer[inst->input_pos] = '\0';

                        char cmdline[INPUT_BUFFER_SIZE + 64];
                        const char *cwd = fs_get_current_dir();
                        strncpy(cmdline, "bottleOS ", sizeof(cmdline));
                        strncat(cmdline, cwd, sizeof(cmdline)-strlen(cmdline)-1);
                        strncat(cmdline, " > ", sizeof(cmdline)-strlen(cmdline)-1);
                        strncat(cmdline, inst->input_buffer, sizeof(cmdline)-strlen(cmdline)-1);

                        history_push(inst, cmdline);

                        // Process the command
                        process_command(inst, inst->input_buffer);

                        inst->input_pos = 0;
                        inst->input_buffer[0] = '\0';
                    }
                } else if (c == '\b') {
                    if (inst->input_pos > 0) inst->input_pos--;
                    inst->input_buffer[inst->input_pos] = '\0';
                } else {
                    if (inst->input_pos < INPUT_BUFFER_SIZE - 1) {
                        inst->input_buffer[inst->input_pos++] = c;
                        inst->input_buffer[inst->input_pos] = '\0';
                    }
                }
            }
        }
    }

    // wrap text
    int y = 6;
    int start = inst->history_count > 16 ? inst->history_count - 16 : 0;

    for (int i = start; i < inst->history_count; ++i) {
        const char *s = inst->history[i];
        int col = 0;
        for (const char *p = s; *p; p++) {
            if (*p == '\n' || col >= SHELL_WIDTH) {
                y += LINE_HEIGHT;
                col = 0;
                if (*p == '\n') continue;
            }
            char buf[2] = { *p, '\0' };
            graphics_draw_string(6 + col * 8, y, buf, RGB(220,220,220), 1);
            col++;
        }
        y += LINE_HEIGHT; // extra spacing after each history entry
    }

    // Render typing prompt with cwd
    char typing[INPUT_BUFFER_SIZE + 64];
    const char *cwd = fs_get_current_dir();
    strncpy(typing, "bottleOS ", sizeof(typing));
    strncat(typing, cwd, sizeof(typing)-strlen(typing)-1);
    strncat(typing, " > ", sizeof(typing)-strlen(typing)-1);
    if (inst->input_pos > 0) {
        strncat(typing, inst->input_buffer, sizeof(typing)-strlen(typing)-1);
    }

    int col = 0;
    for (const char *p = typing; *p; p++) {
        if (*p == '\n' || col >= SHELL_WIDTH) {
            y += LINE_HEIGHT;
            col = 0;
            if (*p == '\n') continue;
        }
        char buf[2] = { *p, '\0' };
        graphics_draw_string(6 + col * 8, y, buf, RGB(200,255,200), 1);
        col++;
    }
}
