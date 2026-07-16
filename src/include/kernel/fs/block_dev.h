#ifndef KERNEL_FS_BLOCK_DEV_H
#define KERNEL_FS_BLOCK_DEV_H
#include <stdint.h>


typedef struct block_device_s {
	char name[32];
	uint32_t block_size;
	void *priv_data;

	int (*read_blocks)(struct block_device_s* dev, uint64_t lba, uint32_t count, void *buffer);
	int (*write_blocks)(struct block_device_s* dev, uint64_t lba, uint32_t count, const void *buffer);
} block_device_t;


#endif