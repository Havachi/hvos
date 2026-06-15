#include "kernel/video.h"
#include <stdint.h>
#include <string.h>

static const uint32_t font_width = 8;
static const uint32_t font_height = 16;

const uint32_t term_margin = 10;
static uint32_t line = 0;
static uint32_t col = 0;
static uint32_t max_col;
static uint32_t max_line;
const uint32_t tab_space = 8;

static uint32_t fgc = WHITE;
static uint32_t bgc = BLACK;

bool cursor_visible = false;

uint32_t rgb_to_color(uint8_t r, uint8_t g, uint8_t b) {
    return (0xff << 24) | (((uint32_t)r) << 16) | (((uint32_t)g) << 8) | ((uint32_t)b);
}

static void _print_cursor(bool show) {
    uint32_t posx = col * font_width + term_margin, posy = line * font_height + term_margin;
    uint32_t color = show ? fgc : bgc;
    for (uint64_t y = 0; y < font_height; y++) {
        for (uint64_t x = 0; x < 2; x++) {
            if ((posx + x) < fb->width && (posy + y) < fb->height) {
                put_pixel(posx + x, posy + y, color);
            }
        }
    }
}

void draw_cursor() {
    if (!cursor_visible)
        _print_cursor(true);
    cursor_visible = true;
}

void hide_cursor(){
    if (cursor_visible)
        _print_cursor(false);
    cursor_visible = false;
}

void put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= fb->width || y >= fb->height) return;
    
    uint8_t *fb_bytes = (uint8_t *)fb->address;
    uint32_t *pixel = (uint32_t *)(fb_bytes + (y * fb->pitch) + (x * 4));
    *pixel = color;
}

void draw_rect(uint32_t posx, uint32_t posy, uint32_t w, uint32_t h, uint32_t color) {
    for (size_t y = posy; y < (posy + h); y++) {
        for (size_t x = posx; x < (posx + w); x++) {
            put_pixel(x, y, color);
        }
    }       
}

void draw_glyph(uint32_t x, uint32_t y, uint32_t fgc, uint32_t bgc, char *g) {
    uint32_t target_height = font_height;
    uint32_t target_width = font_width;
    size_t stride = (8 + 7) / 8;
    
    for (size_t dy = 0; dy < target_height; ++dy) {
        size_t src_y = (dy * 16) / target_height;
        const uint8_t *row = (const uint8_t *)g + src_y * stride;
        for (size_t dx = 0; dx < target_width; ++dx) {
            size_t src_x = (dx * 8) / target_width;
            size_t byte_index = src_x >> 3;
            uint8_t mask = 0x80 >> (src_x & 7);
            
            int px = x + dx;
            int py = y + dy;
            
            if (row[byte_index] & mask) {
                put_pixel(px, py, fgc);
            } else {
                put_pixel(px, py, bgc); // Cleanly redraw character cell background
            }
        }
    }
}

static void _newline() {
    hide_cursor();
    line++;
    col = 0;
    draw_cursor();
}

// FIXED: Hardware-accelerated hardware scrolling using raw VRAM blocks
void _scroll_up() {
    uint8_t *fb_bytes = (uint8_t *)fb->address;
    size_t bytes_per_line = fb->pitch;
    size_t scroll_dist_bytes = font_height * bytes_per_line;
    size_t total_fb_size = fb->height * bytes_per_line;
    size_t copy_size_bytes = total_fb_size - scroll_dist_bytes;

    // Shift screen text up cleanly directly on the physical graphic card buffer
    memmove(fb_bytes, fb_bytes + scroll_dist_bytes, copy_size_bytes);

    // Wipe the trailing row to the clean terminal background color
    uint32_t pixels_per_pitch_row = fb->pitch / 4;
    for (uint32_t y = fb->height - font_height; y < fb->height; y++) {
        volatile uint32_t *row = (volatile uint32_t *)(fb_bytes + (y * fb->pitch));
        // Clear all the way to the stride boundary to wipe uninitialized data artifacts
        for (uint32_t x = 0; x < pixels_per_pitch_row; x++) {
            row[x] = bgc;
        }
    }
}

void put_char(char c) {
    char *g = (char *)font8x16[(uint8_t)c];
    hide_cursor();
    if (c == '\n') {
        _newline();
        return;
    }
    if (c == '\t') {
        uint32_t mod = col % tab_space;
        uint32_t next_col = (col - mod) + tab_space;
        col = next_col;
        return;
    }
    if (c == '\b') {
        move_cursor_left();
        put_char(' ');
        move_cursor_left();
        return;
    }
    if (col >= max_col) {
        _newline();
    }
    if (line >= max_line) {
        _scroll_up();
        line -= 1;
    }

    draw_glyph(col * font_width + term_margin, line * font_height + term_margin, fgc, bgc, g);
    move_cursor_right();
    draw_cursor();
}

static int _strlen(char *str) {
    int i = 0;
    while (*str) { str++; i++; }
    return i;
}

static void _set_fg(uint32_t color) { fgc = color; }
static void _set_bg(uint32_t color) { bgc = color; }
static void _reset_colors() { fgc = WHITE; bgc = BLACK; }

static void _print(char *str) {
    int len = _strlen(str);
    for (int i = 0; i < len; i++) {
        put_char(str[i]);
    }
}


void print(char *str) { _print(str); }

void print_error(char *str) {
    _set_fg(RED);
    _print(str);
    _reset_colors();
}

void clear_screen() {
    uint8_t *fb_bytes = (uint8_t *)fb->address;
    uint32_t pixels_per_pitch_row = fb->pitch / 4;
    
    // Clear visible screen and the hardware tracking gaps completely
    for (uint32_t y = 0; y < fb->height; y++) {
        volatile uint32_t *row = (volatile uint32_t *)(fb_bytes + (y * fb->pitch));
        for (uint32_t x = 0; x < pixels_per_pitch_row; x++) {
            row[x] = BLACK;
        }
    }
	col = 0;
	line = 0;
}

void init_fb() {
    fb = framebuffer_request.response->framebuffers[0];
    max_col = (fb->width - (term_margin * 2)) / font_width;
    max_line = (fb->height - (term_margin * 2)) / font_height - 1;
    
    clear_screen();
}


void move_cursor_right() {
    hide_cursor();
    if (col < max_col)
        col++;
    draw_cursor();
}

void move_cursor_left() {
    hide_cursor();
    if (col > 0)
        col--;
    draw_cursor();
}


void move_cursor_up() {
    hide_cursor();
    if (line > 0)
        line--;
    draw_cursor();
}

void move_cursor_down() {
    hide_cursor();
    if (line < max_line)
        line++;
    draw_cursor();
}

void update_cursor() {
    static uint64_t ticks = 0;
    ticks++;
    if (ticks % 50 == 0) {
        if (cursor_visible)
            hide_cursor();
        else
            draw_cursor();
    }
}

uint32_t get_fgc() {
    return fgc;
}

void set_fgc(uint32_t nfgc) {
    fgc = nfgc;
}