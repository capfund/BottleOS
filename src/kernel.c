#include "kernel.h"
#include "vga/vga.h"
#include "shell/shell.h"
#include "fs/fs.h"
#include "multiboot.h"

int light_mode = 1;

uint8_t *disk_module_addr = 0;
uint32_t disk_module_size = 0;

void set_disk_module(uint32_t start, uint32_t end) {
    disk_module_addr = (uint8_t*)start;
    disk_module_size = end - start;
}

void kprint_num(uint32_t n) {
    char buf[12];
    int i = 10;
    buf[11] = 0;
    if (n == 0) {
        kprint("0");
        return;
    }
    while (n > 0 && i >= 0) {
        buf[i--] = '0' + (n % 10);
        n /= 10;
    }
    kprint(&buf[i+1]);
}


void kernel_main(void) {
    vga_clear_screen();
    vga_putstr("Welcome to BottleOS Shell [light, testing branch] \n", color_green_on_black());

    multiboot_info_t *mbi = (multiboot_info_t *)addr;

    if (mbi->mods_count > 0) {
        multiboot_module_t *mod = (multiboot_module_t *)mbi->mods_addr;
        disk_module_addr = (uint8_t *)mod->mod_start;
        disk_module_size = mod->mod_end - mod->mod_start;

        vga_putstr("Found disk module, size = ", color_green_on_black());
        kprint_num(disk_module_size);
        vga_putstr("\n", color_green_on_black());
    } else {
        vga_putstr("No modules loaded.\n", color_green_on_black());
    }
    fs_init();
    shell_start();
}
