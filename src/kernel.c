#include "kernel.h"
#include "clib/clib.h"
#include "fs/fs.h"
#include "multiboot.h"
#include "shell/shell.h"
#include "vga/vga.h"
#include "vesa/vesa.h" 

int light_mode = 1;
int use_vesa = 0;  // track which display mode we're using

uint8_t *disk_module_addr = 0;
uint32_t disk_module_size = 0;

void set_disk_module(uint32_t start, uint32_t end) {
  disk_module_addr = (uint8_t *)start;
  disk_module_size = end - start;
}

void kprint_num(uint32_t n) {
  char buf[12];
  int i = 10;
  buf[11] = 0;
  if (n == 0) {
    if (use_vesa) {
      vesa_term_putstr("0", vesa_rgb(0, 255, 0));
    } else {
      vga_putstr("0", color_green_on_black());
    }
    return;
  }
  while (n > 0 && i >= 0) {
    buf[i--] = '0' + (n % 10);
    n /= 10;
  }
  if (use_vesa) {
    vesa_term_putstr(&buf[i + 1], vesa_rgb(0, 255, 0));
  } else {
    vga_putstr(&buf[i + 1], color_green_on_black());
  }
}

int k_create_file(const char *name) { return fs_create_file(name); }

int k_write_file(const char *name, const char *content) {
  uint32_t len = strlen(content);
  return fs_write_file(name, (const uint8_t *)content, len);
}

int k_read_file(const char *name, char *buffer, uint32_t size) {
  return fs_read_file(name, (uint8_t *)buffer, size);
}

void kernel_main(uint32_t magic, uint32_t addr) {
  (void)magic;
  multiboot_info_t *mbi = (multiboot_info_t *)addr;

  // Check for VESA framebuffer first
  if (mbi->flags & (1 << 12)) {
    use_vesa = 1;
    vesa_init(
        mbi->framebuffer_addr,
        mbi->framebuffer_width,
        mbi->framebuffer_height,
        mbi->framebuffer_pitch,
        mbi->framebuffer_bpp
    );
    vesa_term_init();
  } else {
    use_vesa = 0;
    vga_clear_screen();
  }

  if (mbi->mods_count > 0) {
    multiboot_module_t *mod = (multiboot_module_t *)mbi->mods_addr;
    disk_module_addr = (uint8_t *)mod->mod_start;
    disk_module_size = mod->mod_end - mod->mod_start;

    if (use_vesa) {
      vesa_term_putstr("Found disk module, size = ", vesa_rgb(0, 255, 0));
      kprint_num(disk_module_size);
      vesa_term_putstr("\n", vesa_rgb(0, 255, 0));
    } else {
      vga_putstr("Found disk module, size = ", color_green_on_black());
      kprint_num(disk_module_size);
      vga_putstr("\n", color_green_on_black());
    }
  } else {
    if (use_vesa) {
      vesa_term_putstr("No modules loaded.\n", vesa_rgb(255, 0, 0));
    } else {
      vga_putstr("No modules loaded.\n", color_green_on_black());
    }
  }

  if (use_vesa) {
    vesa_term_clear();
    vesa_term_putstr("Welcome to BottleOS Shell [VESA mode, testing branch]\n",
                     vesa_rgb(0, 255, 0));
  } else {
    vga_clear_screen();
    vga_putstr("Welcome to BottleOS Shell [VGA text mode, testing branch]\n",
               color_green_on_black());
  }

  fs_init();
  shell_start();
}