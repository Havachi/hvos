#ifndef KERNEL_FS_BLOCK_DEV_H
#define KERNEL_FS_BLOCK_DEV_H
#include <stdint.h>


#define BLOCK_SIZE_DEFAULT 1024
#define CALC_LBA(start, idx, block_size, sect_size) \
	((uint64_t)(start) + \
	 ((uint64_t)(idx) << (__builtin_ctz(block_size) - __builtin_ctz(sect_size))))
	 
#define CALC_LBA_DIRECT(idx, block_size, sect_size) (CALC_LBA(0, idx, block_size, sect_size))
struct block_device_s;

typedef struct {
	int (*read_blocks)(struct block_device_s* dev, uint64_t lba, uint32_t count, void *buffer);
	int (*write_blocks)(struct block_device_s* dev, uint64_t lba, uint32_t count, const void *buffer);
	int (*stat)(struct block_device_s *dev, void *buffer);
}blkdev_ops_t;

typedef struct {
	uint64_t total_nb_lsect;
	uint64_t logical_sector_size;
	uint64_t physical_sector_size;
} blkdev_stat_t;

typedef struct block_device_s {
	char name[32];
	uint32_t block_size;
	uint32_t total_bg;
	void *priv_data;
	blkdev_ops_t *ops;
} block_device_t;

block_device_t *new_block_dev(char *name, blkdev_ops_t *ops);
int read_block_dev(block_device_t *dev, uint64_t lba, uint32_t count, void *buffer);
int write_block_dev(block_device_t *dev, uint64_t lba, uint32_t count, void *buffer);
int list_blkdevs();
block_device_t *fetch_from_list(char *name);
#endif