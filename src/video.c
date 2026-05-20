#include "kernel/video.h"
#include <string.h>

static const uint32_t font_width = 8;
static const uint32_t font_height = 16;

const uint32_t term_margin = 10;
static uint32_t line = 1;
static uint32_t col = 1;
static uint32_t max_col;
static uint32_t max_line;
const uint32_t tab_space = 8;

static uint32_t fgc = WHITE;
static uint32_t bgc = BLACK;

uint32_t rgb_to_color(uint8_t r, uint8_t g, uint8_t b) {
    return (0xff << 24) | (((uint32_t)r) << 16) | (((uint32_t)g) << 8) | ((uint32_t)b);
}

// FIXED: Writes directly to Limine's hardware video address using precise byte-pitch steps
void put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= fb->width || y >= fb->height) return;
    
    // Explicit byte-offset stride calculation to match GPU memory geometry
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
        const uint8_t *row = g + src_y * stride;
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

// REMOVED: _flush() is no longer needed since put_pixel writes directly to video memory.
static void _flush() {
    // No-op
}

static void _newline() {
    line++;
    col = 1;
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
    char *g = font8x16[(uint8_t)c];

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
        col--;
    }
    if (col >= max_col) {
        _newline();
    }
    if (line >= max_line) {
        _scroll_up();
        line -= 1;
    }

    draw_glyph(col * font_width + term_margin, line * font_height + term_margin, fgc, bgc, g);
    col++;
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
	col = 1;
	line = 1;
}

void init_fb() {
    fb = framebuffer_request.response->framebuffers[0];
    max_col = (fb->width - (term_margin * 2)) / font_width;
    max_line = (fb->height - (term_margin * 2)) / font_height - 1;
    
    clear_screen();
}
