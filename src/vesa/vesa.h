#ifndef VESA_H
#define VESA_H

#include <stdint.h>
#include <stddef.h>
typedef struct {
    uint16_t attributes;
    uint8_t  window_a;
    uint8_t  window_b;
    uint16_t granularity;
    uint16_t window_size;
    uint16_t segment_a;
    uint16_t segment_b;
    uint32_t win_func_ptr;
    uint16_t pitch;
    uint16_t width;
    uint16_t height;
    uint8_t  w_char;
    uint8_t  y_char;
    uint8_t  planes;
    uint8_t  bpp;
    uint8_t  banks;
    uint8_t  memory_model;
    uint8_t  bank_size;
    uint8_t  image_pages;
    uint8_t  reserved0;
    
    uint8_t  red_mask;
    uint8_t  red_position;
    uint8_t  green_mask;
    uint8_t  green_position;
    uint8_t  blue_mask;
    uint8_t  blue_position;
    uint8_t  reserved_mask;
    uint8_t  reserved_position;
    uint8_t  direct_color_attributes;
    
    uint32_t framebuffer;
    uint32_t off_screen_mem_off;
    uint16_t off_screen_mem_size;
    uint8_t  reserved1[206];
} __attribute__((packed)) vesa_mode_info_t;


// Initialize VESA (get info from multiboot)
int vesa_init(uint32_t framebuffer_addr, uint32_t width, uint32_t height, uint32_t pitch, uint8_t bpp);


// Pixel operations
void vesa_put_pixel(uint32_t x, uint32_t y, uint32_t color);
uint32_t vesa_rgb(uint8_t r, uint8_t g, uint8_t b);


// Drawing primitives
void vesa_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
void vesa_clear_screen(uint32_t color);


// Text rendering (simple 8x8 font)
void vesa_putchar(char c, uint32_t x, uint32_t y, uint32_t fg, uint32_t bg);
void vesa_putstr(const char *str, uint32_t x, uint32_t y, uint32_t fg, uint32_t bg);


// Terminal emulation
void vesa_term_init(void);
void vesa_term_putchar(char c, uint32_t color);
void vesa_term_putstr(const char *str, uint32_t color);
void vesa_term_clear(void);


#endif