#include "kernel.h"
#include "clib/clib.h"
#include "fs/fs.h"
#include "multiboot.h"
#include "shell/shell.h"
#include "display/display.h"  // Use display instead of direct VGA/VESA
#include "vesa/vesa.h"

int light_mode = 0;
int use_vesa = 1; // track which display mode we're using

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
    display_putstr("0", vesa_rgb(0, 255, 0));
    return;
  }
  while (n > 0 && i >= 0) {
    buf[i--] = '0' + (n % 10);
    n /= 10;
  }
  display_putstr(&buf[i + 1], vesa_rgb(0, 255, 0));
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

  // Debug: Print multiboot flags
  display_putstr("Multiboot flags: ", vesa_rgb(255, 255, 255));
  kprint_num(mbi->flags);
  display_putstr("\n", vesa_rgb(255, 255, 255));

  // Check for VESA framebuffer first
  if (mbi->flags & (1 << 12)) {
    display_putstr("VESA framebuffer detected!\n", vesa_rgb(0, 255, 0));
    use_vesa = 1;
    vesa_init(mbi->framebuffer_addr, mbi->framebuffer_width,
              mbi->framebuffer_height, mbi->framebuffer_pitch,
              mbi->framebuffer_bpp);
    vesa_term_init();
    display_clear();
    display_putstr("Welcome to BottleOS Shell [VESA mode, testing branch]\n",
                   vesa_rgb(0, 255, 0));
  } else {
    display_putstr("No VESA framebuffer, using VGA\n", vesa_rgb(255, 0, 0));
    use_vesa = 0;
    // VGA will be initialized by display layer
    display_clear();
    display_putstr("Welcome to BottleOS Shell [VGA text mode, testing branch]\n",
                   vesa_rgb(0, 255, 0));
  }

  if (mbi->mods_count > 0) {
    multiboot_module_t *mod = (multiboot_module_t *)mbi->mods_addr;
    disk_module_addr = (uint8_t *)mod->mod_start;
    disk_module_size = mod->mod_end - mod->mod_start;

    display_putstr("Found disk module, size = ", vesa_rgb(0, 255, 0));
    kprint_num(disk_module_size);
    display_putstr("\n", vesa_rgb(0, 255, 0));
  } else {
    display_putstr("No modules loaded.\n", vesa_rgb(255, 0, 0));
  }

  fs_init();
  shell_start();
}