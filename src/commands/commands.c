#include "commands.h"
#include "../clib/clib.h"
#include "../kernel.h"
#include "../vga/vga.h"

void cmd_hello() { 
    vga_putstr("Hello, user!\n", color_green_on_black()); 
}

void cmd_clear() { 
    vga_clear_screen(); 
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

int cmd_bye(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    vga_putstr("Shutting down...\n", color_green_on_black());
    while (1) {
        __asm__("hlt");
    }
    return 0;
}
