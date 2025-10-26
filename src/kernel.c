#include "kernel.h"
#include "vga/vga.h"
#include "shell/shell.h"

int light_mode = 1;

void kernel_main(void) {
    vga_clear_screen();
    vga_putstr("Welcome to BottleOS Shell [light, testing branch] \n", color_green_on_black());
    shell_start();
}
