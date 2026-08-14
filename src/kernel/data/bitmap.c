#include "data_structure/bitmap.h"
#include "mem/mem.h"
#include <stddef.h>
#include <stdint.h>


bitmap_t	*alloc_bitmap(uint32_t size) {
	bitmap_t *map = NULL;
	map = kzalloc(sizeof(bitmap_t));
	if (!map)
		return NULL;
	map->nb_bits = size;
	size_t arr_size = (size + BITS_PER_ELEMENT - 1) >> BITMAP_SHIFT;

	map->data = kcalloc(arr_size, sizeof(uint32_t));
	if (!map->data) {
		kfree(map);
		return NULL;
	}
	return map;
}

void free_bitmap(bitmap_t *bitmap) {
	if (bitmap) {
		kfree(bitmap->data);
		kfree(bitmap);
	}
}