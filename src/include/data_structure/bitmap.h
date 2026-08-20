#ifndef HVOS_DATA_STRUCT_BITMAP_H
#define HVOS_DATA_STRUCT_BITMAP_H

#include <stdint.h>
#include <stdlib.h>

#define BITMAP_SHIFT 5
#define BITMAP_MASK 0x1F
#define BITS_PER_ELEMENT (1 << BITMAP_SHIFT)



typedef struct bitmap_s {
	uint32_t *data;
	size_t nb_bits;
} bitmap_t;



bitmap_t	*alloc_bitmap(uint32_t size);
void		free_bitmap(bitmap_t *bitmap);

static void	set_bitmap(bitmap_t *map, size_t idx) {
	if (map == NULL || idx >= map->nb_bits) return;
	map->data[idx >> BITMAP_SHIFT] |= (1U << (idx & BITMAP_MASK));
}

static void set_bitmap_n(bitmap_t *map, uint32_t start, size_t count) {
	for (size_t i = start; i < start + count; i++) {
		set_bitmap(map, i);
	}
}

static void	clear_bitmap(bitmap_t *map, size_t idx) {
	if (map == NULL || idx >= map->nb_bits) return;
	map->data[idx >> BITMAP_SHIFT] &= ~(1U << (idx & BITMAP_MASK));
}

static void	toggle_bitmap(bitmap_t *map, size_t idx) {
	if (map == NULL || idx >= map->nb_bits) return;
	map->data[idx >> BITMAP_SHIFT] ^= (1U << (idx & BITMAP_MASK));
}

static int	test_bitmap(bitmap_t *map, size_t idx) {
	if (map == NULL || idx >= map->nb_bits) return 0;
	return (map->data[idx >> BITMAP_SHIFT] & (1U << (idx & BITMAP_MASK))) != 0;
}

static int next_free_bitmap(bitmap_t *map) {
	uint32_t i = 0;
	int result = -1;

	for (uint32_t i = 0; i < map->nb_bits; i++) {
		if (test_bitmap(map, i) == 0) {
			result = i;
			break;
		}
	}
	return result;
}

#endif