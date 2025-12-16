#include "kernel.h"
#include "clib/clib.h"
#include "fs/fs.h"
#include "multiboot.h"
#include "shell/shell.h"
#include "vga/vga.h"

/* Last modified to refactor code 
- removed disk module functionality (replaced with ata pio)
- removed redundant helpers
- added test funct

TODO: Extend fb functionality :)*/

int light_mode = 0; // global var, i hate light mode :<    

void kprint_num(uint32_t n) {
    char buf[12];
    int i = 10;
    buf[11] = 0;

    if (n == 0) {
        vga_putstr("0", color_green_on_black());
        return;
    }

    while (n > 0 && i >= 0) {
        buf[i--] = '0' + (n % 10);
        n /= 10;
    }

    vga_putstr(&buf[i + 1], color_green_on_black());
}

// Kernel entry point
void kernel_main(uint32_t magic, uint32_t addr) {
    (void)magic;
    multiboot_info_t *mbi = (multiboot_info_t *)addr;

    // Clear screen and print welcome message
    vga_clear_screen();
    vga_putstr("Welcome to BottleOS Shell [light, testing branch]\n", 
               color_green_on_black());

    // Simple test: print memory info if available
    if (mbi->flags & 0x1) { // bit 0 = mem info present
        vga_putstr("Lower memory: ", color_green_on_black());
        kprint_num(mbi->mem_lower);
        vga_putstr(" KB\n", color_green_on_black());

        vga_putstr("Upper memory: ", color_green_on_black());
        kprint_num(mbi->mem_upper);
        vga_putstr(" KB\n", color_green_on_black());
    } else {
        vga_putstr("No memory info provided.\n", color_green_on_black());
    }

    // Initialize filesystem and start shell
    fs_init();
    shell_start();
}
