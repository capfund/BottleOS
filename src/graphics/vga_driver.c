#include "vga_driver.h"
#include "../kernel.h"  // for ioport_in/out
#include <string.h>      // for memcpy

#define VGA_ADDRESS 0xA0000

// Helper start
void ioport_out(unsigned short port, unsigned char val) {
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}

unsigned char ioport_in(unsigned short port) {
    unsigned char ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void write_regs(unsigned char *regs)
{
    unsigned i;

    // write MISC
    ioport_out(VGA_MISC_WRITE, *regs);
    regs++;
    // write SEQ
    for(i = 0; i < VGA_NUM_SEQ_REGS; i++) {
        ioport_out(VGA_SEQ_INDEX, i);
        ioport_out(VGA_SEQ_DATA, *regs);
        regs++;
    }
    // unlock CRTC
    ioport_out(VGA_CRTC_INDEX, 0x03);
    ioport_out(VGA_CRTC_DATA, ioport_in(VGA_CRTC_DATA) | 0x80);
    ioport_out(VGA_CRTC_INDEX, 0x11);
    ioport_out(VGA_CRTC_DATA, ioport_in(VGA_CRTC_DATA) & ~0x80);
    regs[0x03] |= 0x80;
    regs[0x11] &= ~0x80;
    // write CRTC
    for(i = 0; i < VGA_NUM_CRTC_REGS; i++) {
        ioport_out(VGA_CRTC_INDEX, i);
        ioport_out(VGA_CRTC_DATA, *regs);
        regs++;
    }
    // write GC
    for(i = 0; i < VGA_NUM_GC_REGS; i++) {
        ioport_out(VGA_GC_INDEX, i);
        ioport_out(VGA_GC_DATA, *regs);
        regs++;
    }
    // write AC
    for(i = 0; i < VGA_NUM_AC_REGS; i++) {
        (void)ioport_in(VGA_INSTAT_READ);
        ioport_out(VGA_AC_INDEX, i);
        ioport_out(VGA_AC_WRITE, *regs);
        regs++;
    }
    (void)ioport_in(VGA_INSTAT_READ);
    ioport_out(VGA_AC_INDEX, 0x20);
}
// helper end

static unsigned char g_320x200x256[] = {
    /* VGA registers */
    0x63,0x03,0x01,0x0F,0x00,0x0E,
    0x5F,0x4F,0x50,0x82,0x54,0x80,0xBF,0x1F,
    0x00,0x41,0x00,0x00,0x00,0x00,0x00,0x00,
    0x9C,0x0E,0x8F,0x28,0x40,0x96,0xB9,0xA3,
    0xFF,0x00,0x00,0x00,0x00,0x00,0x40,0x05,
    0x0F,0xFF,0x00,0x01,0x02,0x03,0x04,0x05,
    0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,
    0x0E,0x0F,0x41,0x00,0x0F,0x00,0x00
};

// ** DOUBLE BUFFERING **
static unsigned char front_buffer[320*200]; // current frame on screen
static unsigned char back_buffer[320*200];  // off-screen buffer for drawing

static void vga_init() {
    write_regs(g_320x200x256);
    memset(front_buffer, 0, sizeof(front_buffer));
    memset(back_buffer, 0, sizeof(back_buffer));
    memcpy((unsigned char*)VGA_ADDRESS, front_buffer, sizeof(front_buffer));
}

static void vga_clear_screen(graphics_color_t color) {
    memset(back_buffer, color, sizeof(back_buffer));
}

static void vga_plot_pixel(int x, int y, graphics_color_t color) {
    if (x < 0 || x >= 320 || y < 0 || y >= 200) return;
    back_buffer[x + y*320] = color;
}

// Flush back buffer to VGA (swap buffers)
void vga_flush() {
    unsigned char *VGA = (unsigned char*)VGA_ADDRESS;
    memcpy(VGA, back_buffer, sizeof(back_buffer));
    // optionally copy to front_buffer if you need it for logic
    memcpy(front_buffer, back_buffer, sizeof(back_buffer));
}

// VGA driver instance
graphics_driver_t graphics_vga_driver = {
    .init = vga_init,
    .clear_screen = vga_clear_screen,
    .plot_pixel = vga_plot_pixel
};
