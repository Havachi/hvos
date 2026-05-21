#include "mem/mem.h"

uint8_t *bitmap;
uint64_t bitmap_size;

void bitmap_set(uint64_t page_index) {
	bitmap[(page_index / 8)] |= (1 << (page_index % 8));
}

void bitmap_clear(uint64_t page_index) {
	bitmap[(page_index / 8)] &= ~(1 << (page_index % 8));
}

int32_t bitmap_test(uint64_t page_index) {
	return (bitmap[(page_index / 8)] >> (page_index % 8)) & 1;
}

