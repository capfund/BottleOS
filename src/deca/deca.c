#include "deca.h"
#include "../vga/vga.h"
#include "../keyboard/keyboard.h"
#include "../fs/fs.h"
#include "../clib/clib.h"
#include "../kernel.h"
#include <stdint.h>
#include "../commands/commands.h"

#define DECA_MAX_LINES 50 // formerly 1024
#define DECA_LINE_LEN  80

static void press_any_key_to_continue(void) {
    vga_putstr("\nPress any key to continue... [You are in Deca.]", color_white_on_black());
    while (1) {
        unsigned char scancode = keyboard_get_scancode();
        if (!(scancode & 0x80)) // key pressed (not released)
            break;
    }
}

static int int_to_str(char *buf, int val) {
    if (!buf) return 0;
    char tmp[16];
    int i = 0;
    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return 1;
    }
    while (val > 0 && i < 15) {
        tmp[i++] = '0' + (val % 10);
        val /= 10;
    }
    int len = i;
    for (int j = 0; j < len; ++j)
        buf[j] = tmp[len - j - 1];
    buf[len] = '\0';
    return len;
}

/* editor state */
static char buffer[DECA_MAX_LINES][DECA_LINE_LEN + 1]; // NUL-terminated lines
static int total_lines = 1;
static int cur_row = 0;
static int cur_col = 0;
static int view_row = 0;   // topmost visible buffer row
static char cur_filename[FS_FILENAME_LEN];
static int modified = 0;
static char clip_line[DECA_LINE_LEN + 1];

/* --- helpers --- */

static void deca_clear_buffer(void) {
    for (int r = 0; r < DECA_MAX_LINES; ++r)
        for (int c = 0; c <= DECA_LINE_LEN; ++c)
            buffer[r][c] = '\0';
    total_lines = 1;
    cur_row = 0;
    cur_col = 0;
    view_row = 0;
    cur_filename[0] = '\0';
    modified = 0;
    clip_line[0] = '\0';
}

/* load file into buffer */
static void deca_load_file(const char *fname) {
    if (!fname || fname[0] == '\0') return;

    /* FIX: clear buffer *before* setting filename */
    deca_clear_buffer();

    strncpy(cur_filename, fname, FS_FILENAME_LEN - 1);
    cur_filename[FS_FILENAME_LEN - 1] = '\0';

    uint8_t *tmp = (uint8_t *)malloc(DECA_MAX_LINES * DECA_LINE_LEN);
    if (!tmp) return;

    int r = fs_read_file(fname, tmp, DECA_MAX_LINES * DECA_LINE_LEN);
    if (r <= 0) {
        free(tmp);
        return;
    }

    int row = 0, col = 0;
    for (int i = 0; i < r && row < DECA_MAX_LINES; ++i) {
        char ch = (char)tmp[i];
        if (ch == '\r') continue;
        if (ch == '\n') {
            buffer[row][col] = '\0';
            row++;
            col = 0;
            continue;
        }
        if (col < DECA_LINE_LEN) buffer[row][col++] = ch;
    }
    total_lines = row > 0 ? row + 1 : 1;

    free(tmp);
}

/* helper: remove leading slash for our FS (fs.find_entry expects names without leading '/') */
static void sanitize_filename(char *out, const char *in, size_t outlen) {
    if (!in || !out || outlen == 0) return;
    if (in[0] == '/') {
        size_t i;
        for (i = 0; i < outlen - 1 && in[i + 1]; ++i)
            out[i] = in[i + 1];
        out[i] = '\0';
    } else {
        size_t i;
        for (i = 0; i < outlen - 1 && in[i]; ++i)
            out[i] = in[i];
        out[i] = '\0';
    }
}

/* save file from buffer */
static void deca_save_file(void) {
    if (cur_filename[0] == '\0') {
        vga_putstr("deca: no filename (set from shell)\n", color_white_on_black());
        return;
    }

    vga_putstr(cur_filename, color_white_on_black());

    /* sanitize filename to match fs expectations */
    char fname[FS_FILENAME_LEN];
    sanitize_filename(fname, cur_filename, sizeof(fname));

    int maxsize = DECA_MAX_LINES * (DECA_LINE_LEN + 1);
    uint8_t *flat = (uint8_t *)malloc(maxsize);
    if (!flat) {
        vga_putstr("deca: out of memory saving file\n", color_white_on_black());
        return;
    }

    int pos = 0;
    for (int r = 0; r < total_lines; ++r) {
        int len = 0;
        while (len < DECA_LINE_LEN && buffer[r][len]) len++;
        for (int i = 0; i < len; ++i) {
            if (pos >= maxsize) break;
            flat[pos++] = (uint8_t)buffer[r][i];
        }
        if (pos < maxsize) flat[pos++] = '\n';
        else break;
    }

    char numbuf[16];
    int_to_str(numbuf, pos);
    vga_putstr("deca: saving bytes=", color_white_on_black());
    vga_putstr(numbuf, color_white_on_black());
    vga_putchar('\n', color_white_on_black());

    int res = fs_write_file(fname, flat, (uint32_t)pos);
    if (res < 0) {
        vga_putstr("deca: fs_write_file failed: ", color_white_on_black());
        char rc[8]; int_to_str(rc, res);
        vga_putstr(rc, color_white_on_black());
        vga_putchar('\n', color_white_on_black());

        if (res == -1) {
            int c = fs_create_file(fname);
            if (c == 0) {
                vga_putstr("deca: file created, retrying write...\n", color_white_on_black());
                int r2 = fs_write_file(fname, flat, (uint32_t)pos);
                if (r2 >= 0) {
                    vga_putstr("deca: save OK after create\n", color_green_on_black());
                    modified = 0;
                    free(flat);
                    return;
                } else {
                    vga_putstr("deca: write still failed\n", color_white_on_black());
                }
            } else {
                vga_putstr("deca: fs_create_file failed\n", color_white_on_black());
            }
        }
    } else {
        vga_putstr("deca: save OK\n", color_green_on_black());
        modified = 0;
    }

    free(flat);
}

/* ensure viewport contains cursor */
static void deca_ensure_viewport(void) {
    if (cur_row < view_row) view_row = cur_row;
    if (cur_row >= view_row + VGA_MEM_HEIGHT - 2)
        view_row = cur_row - (VGA_MEM_HEIGHT - 3);
    if (view_row < 0) view_row = 0;
}

/* draw editor + status bar */
static void deca_draw(void) {
    static char front_buf[VGA_MEM_HEIGHT][VGA_MEM_WIDTH * 2];
    static int first_draw = 1;
    char *video = (char *)VIDEO_MEMORY;

    for (int r = 0; r < VGA_MEM_HEIGHT - 2 && (view_row + r) < total_lines; ++r) {
        int buf_row = view_row + r;
        for (int c = 0; c < VGA_MEM_WIDTH; ++c) {
            char ch = buffer[buf_row][c];
            if (!ch) ch = ' ';
            uint8_t color = color_white_on_black();
            int offset = (r * VGA_MEM_WIDTH + c) * 2;

            if (first_draw || front_buf[r][c * 2] != ch || front_buf[r][c * 2 + 1] != color) {
                video[offset] = ch;
                video[offset + 1] = color;
                front_buf[r][c * 2] = ch;
                front_buf[r][c * 2 + 1] = color;
            }
        }
    }

    /* status bar */
    char status[VGA_MEM_WIDTH + 1];
    for (int i = 0; i < VGA_MEM_WIDTH; i++) status[i] = ' ';
    status[VGA_MEM_WIDTH] = '\0';

    int pos = 0;
    if (cur_filename[0]) {
        int len = strlen(cur_filename);
        for (int i = 0; i < len && pos < VGA_MEM_WIDTH; ++i) status[pos++] = cur_filename[i];
    }

    char coords[16];
    int colpos = cur_col + 1;
    int rowpos = cur_row + 1;
    int n = int_to_str(coords, rowpos);
    for (int i = 0; i < n && pos < VGA_MEM_WIDTH; ++i) status[pos++] = coords[i];
    status[pos++] = ',';
    n = int_to_str(coords, colpos);
    for (int i = 0; i < n && pos < VGA_MEM_WIDTH; ++i) status[pos++] = coords[i];

    int row = VGA_MEM_HEIGHT - 1;
    for (int c = 0; c < VGA_MEM_WIDTH; c++) {
        char ch = status[c];
        uint8_t color = color_green_on_black();
        int offset = (row * VGA_MEM_WIDTH + c) * 2;
        if (first_draw || front_buf[row][c * 2] != ch || front_buf[row][c * 2 + 1] != color) {
            video[offset] = ch;
            video[offset + 1] = color;
            front_buf[row][c * 2] = ch;
            front_buf[row][c * 2 + 1] = color;
        }
    }

    first_draw = 0;
}

/* editing primitives */
static void deca_insert_char(char ch) {
    if (cur_row >= DECA_MAX_LINES) return;
    int len = strlen(buffer[cur_row]);
    if (len >= DECA_LINE_LEN && total_lines < DECA_MAX_LINES) {
        for (int i = total_lines; i > cur_row + 1; --i)
            memcpy(buffer[i], buffer[i - 1], DECA_LINE_LEN + 1);
        buffer[cur_row + 1][0] = '\0';
        total_lines++;
    }

    for (int i = DECA_LINE_LEN - 1; i > cur_col; --i)
        buffer[cur_row][i] = buffer[cur_row][i - 1];

    buffer[cur_row][cur_col] = ch;
    buffer[cur_row][DECA_LINE_LEN] = '\0';
    cur_col++;
    if (cur_col > (int)strlen(buffer[cur_row])) cur_col = strlen(buffer[cur_row]);
    modified = 1;
}

static void deca_backspace(void) {
    if (cur_col > 0) {
        for (int i = cur_col - 1; i < DECA_LINE_LEN - 1; ++i)
            buffer[cur_row][i] = buffer[cur_row][i + 1];
        buffer[cur_row][DECA_LINE_LEN - 1] = '\0';
        cur_col--;
        modified = 1;
    } else if (cur_row > 0) {
        int prev_len = strlen(buffer[cur_row - 1]);
        int cur_len = strlen(buffer[cur_row]);
        if (prev_len + cur_len <= DECA_LINE_LEN) {
            strncat(buffer[cur_row - 1], buffer[cur_row], DECA_LINE_LEN - prev_len);
            for (int i = cur_row; i < total_lines - 1; ++i)
                memcpy(buffer[i], buffer[i + 1], DECA_LINE_LEN + 1);
            buffer[total_lines - 1][0] = '\0';
            total_lines--;
            cur_row--;
            cur_col = prev_len;
            modified = 1;
        }
    }
}

static void deca_enter(void) {
    if (total_lines >= DECA_MAX_LINES) return;
    char newline[DECA_LINE_LEN + 1] = {0};
    int cur_len = strlen(buffer[cur_row]);
    for (int i = cur_col; i <= cur_len; ++i) newline[i - cur_col] = buffer[cur_row][i];
    buffer[cur_row][cur_col] = '\0';

    for (int i = total_lines; i > cur_row + 1; --i)
        memcpy(buffer[i], buffer[i - 1], DECA_LINE_LEN + 1);

    strcpy(buffer[cur_row + 1], newline);
    total_lines++;
    cur_row++;
    cur_col = 0;
    modified = 1;
}

static void deca_cut_line(void) {
    if (cur_row < 0 || cur_row >= total_lines) return;
    strncpy(clip_line, buffer[cur_row], DECA_LINE_LEN);
    clip_line[DECA_LINE_LEN] = '\0';
    for (int i = cur_row; i < total_lines - 1; ++i)
        memcpy(buffer[i], buffer[i + 1], DECA_LINE_LEN + 1);
    buffer[total_lines - 1][0] = '\0';
    total_lines--;
    if (cur_row >= total_lines) cur_row = total_lines - 1;
    cur_col = 0;
    modified = 1;
}

static void deca_paste_line(void) {
    if (!clip_line[0] || total_lines >= DECA_MAX_LINES) return;
    for (int i = total_lines; i > cur_row; --i)
        memcpy(buffer[i], buffer[i - 1], DECA_LINE_LEN + 1);
    strncpy(buffer[cur_row], clip_line, DECA_LINE_LEN);
    buffer[cur_row][DECA_LINE_LEN] = '\0';
    total_lines++;
    modified = 1;
}

/* main entry */
void deca_start(const char *name) {
    char filename[FS_FILENAME_LEN] = {0};
    if (name)
        strncpy(filename, name, FS_FILENAME_LEN - 1);

    vga_putstr("filename:", color_white_on_black());
    vga_putstr(filename, color_white_on_black());
    vga_putchar('\n', color_white_on_black());
    press_any_key_to_continue();
    vga_clear_screen();

    if (filename[0])
        deca_load_file(filename);

    deca_ensure_viewport();
    deca_draw();

    while (1) {
        deca_ensure_viewport();
        deca_draw();

        unsigned char sc = keyboard_poll_scancode();
        if (!sc) continue;

        keyboard_handle_modifier(sc);

        if (sc & 0x80) continue; // key release

        char ch = keyboard_scancode_to_ascii(sc);

        /* Ctrl commands */
        if (keyboard_is_ctrl_pressed()) {
            switch (ch) {
                case 'x': deca_save_file(); vga_clear_screen(); return;
                case 'o': deca_save_file(); continue;
                case 'k': deca_cut_line(); continue;
                case 'u': deca_paste_line(); continue;
            }
        }

        /* arrow keys */
        switch (sc) {
            case 0x48: if (cur_row > 0) cur_row--; break;       // up
            case 0x50: if (cur_row + 1 < total_lines) cur_row++; break; // down
            case 0x4B: if (cur_col > 0) cur_col--; break;       // left
            case 0x4D: if (cur_col < (int)strlen(buffer[cur_row])) cur_col++; break; // right
        }

        /* ASCII input */
        if (ch) {
            if (ch == '\b') deca_backspace();
            else if (ch == '\n') deca_enter();
            else deca_insert_char(ch);
        }

        if (cur_row < 0) cur_row = 0;
        if (cur_row >= total_lines) cur_row = total_lines - 1;
        if (cur_col < 0) cur_col = 0;
        if (cur_col > (int)strlen(buffer[cur_row])) cur_col = strlen(buffer[cur_row]);
    }
}
