#include <stddef.h>
#include <stdint.h>

#define VIDEO_MEMORY ((char *)0xb8000)

#define WHITE_ON_BLACK 0x0f
#define GREEN_ON_BLACK 0x02

#define VGA_MEM_WIDTH 80
#define VGA_MEM_HEIGHT 25

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define INPUT_BUFFER_SIZE 128
#define MAX_ARGS 16

// -------------------- Minimal string functions --------------------
int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return (unsigned char)(*s1) - (unsigned char)(*s2);
}

size_t strlen(const char *s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

// -------------------- Global state --------------------
static unsigned int cursor_row = 0;
static unsigned int cursor_col = 0;

char input_buffer[INPUT_BUFFER_SIZE];
unsigned int input_pos = 0;

static int shift_pressed = 0;
static int ctrl_pressed = 0;
static int caps_lock_on = 0;

// -------------------- VGA functions --------------------
void clear_screen() {
    char *video_memory = VIDEO_MEMORY;
    for (unsigned int i = 0; i < VGA_MEM_WIDTH * VGA_MEM_HEIGHT; i++) {
        video_memory[i * 2] = ' ';
        video_memory[i * 2 + 1] = WHITE_ON_BLACK;
    }
    cursor_row = 0;
    cursor_col = 0;
}

void scroll() {
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
        video_memory[((VGA_MEM_HEIGHT - 1) * VGA_MEM_WIDTH + col) * 2 + 1] = WHITE_ON_BLACK;
    }

    if (cursor_row > 0) cursor_row--;
}

void putchar(char c, unsigned char color) {
    char *video_memory = VIDEO_MEMORY;

    if (c == '\n') {
        cursor_col = 0;
        cursor_row++;
        if (cursor_row >= VGA_MEM_HEIGHT) scroll();
        return;
    }

    video_memory[(cursor_row * VGA_MEM_WIDTH + cursor_col) * 2] = c;
    video_memory[(cursor_row * VGA_MEM_WIDTH + cursor_col) * 2 + 1] = color;

    cursor_col++;
    if (cursor_col >= VGA_MEM_WIDTH) {
        cursor_col = 0;
        cursor_row++;
        if (cursor_row >= VGA_MEM_HEIGHT) scroll();
    }
}

void putstr(const char *str, unsigned char color) {
    for (unsigned int i = 0; str[i] != '\0'; i++) {
        putchar(str[i], color);
    }
}

// -------------------- Keyboard --------------------
unsigned char inb(unsigned short port) {
    unsigned char ret;
    __asm__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void handle_modifier(unsigned char scancode) {
    switch (scancode) {
        case 0x2A: shift_pressed = 1; break;  // Left Shift
        case 0x36: shift_pressed = 1; break;  // Right Shift
        case 0xAA: shift_pressed = 0; break;  // Left Shift release
        case 0xB6: shift_pressed = 0; break;  // Right Shift release
        case 0x1D: ctrl_pressed = 1; break;   // Left Ctrl
        case 0x9D: ctrl_pressed = 0; break;   // Left Ctrl release
        case 0x3A: caps_lock_on ^= 1; break;  // Caps Lock toggle
    }
}

char scancode_to_ascii(unsigned char scancode) {
    static char normal[128] = {
        0, 27,'1','2','3','4','5','6','7','8','9','0','-','=','\b',
        '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',0,
        'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\',
        'z','x','c','v','b','n','m',',','.','/',0,'*',0,' ',0
    };

    static char shifted[128] = {
        0, 27,'!','@','#','$','%','^','&','*','(',')','_','+','\b',
        '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',0,
        'A','S','D','F','G','H','J','K','L',':','"','~',0,'|',
        'Z','X','C','V','B','N','M','<','>','?',0,'*',0,' ',0
    };

    if (scancode >= 128) return 0;

    char c = normal[scancode];

    if ((c >= 'a' && c <= 'z')) {
        if ((shift_pressed && !caps_lock_on) || (!shift_pressed && caps_lock_on))
            c -= 32;  // uppercase letters
    } else if (shift_pressed) {
        c = shifted[scancode];  // shifted symbols
    }

    return c;
}

unsigned char get_scancode() {
    while (!(inb(KEYBOARD_STATUS_PORT) & 1)) {}
    return inb(KEYBOARD_DATA_PORT);
}

// -------------------- Utilities --------------------
void parse_input(char *input, char *argv[], int *argc) {
    *argc = 0;
    char *p = input;
    while (*p && *argc < MAX_ARGS) {
        while (*p == ' ') p++; // skip spaces
        if (*p == '\0') break;
        argv[*argc] = p;
        (*argc)++;
        while (*p && *p != ' ') p++;
        if (*p) *p++ = '\0'; // terminate argument
    }
}

// -------------------- Commands --------------------
void cmd_hello() {
    putstr("Hello, user!\n", GREEN_ON_BLACK);
}

void cmd_clear() {
    clear_screen();
}

void cmd_echo(int argc, char *argv[]) {
    int newline = 1;
    int start = 1;
    if (argc > 1 && strcmp(argv[1], "-n") == 0) {
        newline = 0;
        start = 2;
    }
    for (int i = start; i < argc; i++) {
        putstr(argv[i], WHITE_ON_BLACK);
        if (i < argc - 1) putchar(' ', WHITE_ON_BLACK);
    }
    if (newline) putchar('\n', WHITE_ON_BLACK);
}

int cmd_bye(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    putstr("Shutting down...\n", GREEN_ON_BLACK);
    while (1) { __asm__("hlt"); }
    return 0;
}

// -------------------- Shell --------------------
void shell() {
    char *argv[MAX_ARGS];
    int argc;

    putstr("> ", WHITE_ON_BLACK);

    while (1) {
        unsigned char scancode = get_scancode();
        handle_modifier(scancode);

        if (scancode & 0x80) continue; // ignore key releases

        char c = scancode_to_ascii(scancode);
        if (!c) continue;

        // Handle Ctrl shortcuts
        if (ctrl_pressed) {
            switch (c) {
                case 'c':  // Ctrl+C clears input
                    input_pos = 0;
                    putchar('\n', WHITE_ON_BLACK);
                    putstr("> ", WHITE_ON_BLACK);
                    continue;
                case 'a':  // Ctrl+A moves cursor to start
                    input_pos = 0;
                    continue;
                case 'x':  // Ctrl+X
                case 'v':  // Ctrl+V
                    // Clipboard not implemented
                    continue;
            }
        }

        if (c == '\n') {
            input_buffer[input_pos] = '\0';
            putchar('\n', WHITE_ON_BLACK);

            parse_input(input_buffer, argv, &argc);

            if (argc > 0) {
                if (strcmp(argv[0], "hello") == 0) {
                    cmd_hello();
                } else if (strcmp(argv[0], "clear") == 0) {
                    cmd_clear();
                } else if (strcmp(argv[0], "echo") == 0) {
                    cmd_echo(argc, argv);
                } else if (strcmp(argv[0], "bye") == 0 ||
                           strcmp(argv[0], "exit") == 0 ||
                           strcmp(argv[0], "shutdown") == 0) {
                    cmd_bye(argc, argv);
                } else {
                    putstr("Unknown command\n", WHITE_ON_BLACK);
                }
            }

            input_pos = 0;
            putstr("> ", WHITE_ON_BLACK);

        } else if (c == '\b') {
            if (input_pos > 0) {
                input_pos--;
                cursor_col--;
                if (cursor_col >= VGA_MEM_WIDTH) {
                    cursor_col = VGA_MEM_WIDTH - 1;
                    if (cursor_row > 0) cursor_row--;
                }
                putchar(' ', WHITE_ON_BLACK);
                cursor_col--;
            }
        } else {
            if (input_pos < INPUT_BUFFER_SIZE - 1) {
                input_buffer[input_pos++] = c;
                putchar(c, WHITE_ON_BLACK);
            }
        }
    }
}

// -------------------- Kernel entry --------------------
void kernel_main(void) {
    clear_screen();
    putstr("Welcome to BottleOS Shell\n", GREEN_ON_BLACK);
    shell();
}
