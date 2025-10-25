#include "kernel.h"
#include "vga/vga.h"
#include "shell/shell.h"

void kernel_main(void) {
    vga_clear_screen();
    vga_putstr("Welcome to BottleOS Shell\n", GREEN_ON_BLACK);
    shell_start();
}
