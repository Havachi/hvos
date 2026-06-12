#ifndef HVOS_VIDEO_H
#define HVOS_VIDEO_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "kernel/glyphs.h"
#include "limine.h"
#include <string.h>
#include "kernel/boot.h"

#define WHITE 0xFFFFFFFF
#define BLACK 0x00
#define RED 0xFFFF0000

extern struct limine_framebuffer *fb;

uint32_t rgb_to_color(uint8_t r, uint8_t g, uint8_t b);
void draw_rect(uint32_t posx, uint32_t posy, uint32_t w, uint32_t h, uint32_t color);
void put_pixel(uint32_t x, uint32_t y, uint32_t color);
void draw_glyph(uint32_t x, uint32_t y,
			uint32_t fgc, uint32_t bgc, char *g);
void put_char(char c);
void print(char *str);
void print_error(char *str);
void clear_screen();
void init_fb();
void move_cursor_right();
void move_cursor_left();
void move_cursor_up();
void move_cursor_down();
void draw_cursor();
void hide_cursor();

uint32_t get_fgc();
void set_fgc(uint32_t nfgc);
#endif
