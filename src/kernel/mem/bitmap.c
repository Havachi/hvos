#include "mem/mem.h"
#include <stdio.h>

#ifdef BIG_BITMAP
uint32_t *bitmap;
void bitmap_set(uint64_t page_index) {
	bitmap[(page_index / 32)] |= (1 << (page_index % 32));
}

void bitmap_clear(uint64_t page_index) {
	bitmap[(page_index / 32)] &= ~(1 << (page_index % 32));
}

int32_t bitmap_test(uint64_t page_index) {
	return bitmap[(page_index / 32)] & (1 << (page_index % 32));
}

#else

uint8_t *map;
void bitmap_set(uint64_t page_index) {
	map[(page_index / 8)] |= (1 << (page_index % 8));
}

void bitmap_clear(uint64_t page_index) {
	map[(page_index / 8)] &= ~(1 << (page_index % 8));
}

int32_t bitmap_test(uint64_t page_index) {
	return (map[page_index / 8] & (1U << (page_index % 8))) != 0;
}
#endif



