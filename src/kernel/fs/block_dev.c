#include "kernel/fs/block_dev.h"
#include "data_structure/list.h"
#include "mem/mem.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

list_t *blkdevs_list = NULL;
block_device_t *selected_blkdev = NULL;

void init_blkdev_reg() {
	blkdevs_list = init_list();
}

block_device_t *new_block_dev(char *name, blkdev_ops_t *ops) {
	block_device_t *dev = kzalloc(sizeof(block_device_t));
	if (dev == NULL)
		return NULL;
	strncpy(dev->name, name, 20);
	dev->ops = ops;
	if (blkdevs_list == NULL) {
		blkdevs_list = init_list();
	}
	#ifndef BLOCK_SIZE
	dev->block_size = BLOCK_SIZE_DEFAULT;
	#else
	dev->block_size = BLOCK_SIZE;
	#endif
	list_append(blkdevs_list, (void *)dev);
	return dev;
}

int read_block_dev(block_device_t *dev, uint64_t lba, uint32_t count, void *buffer) {
	if (dev == NULL || dev->ops == NULL || dev->ops->read_blocks || count == 0 || buffer == NULL){
		return -1;
	}
	return dev->ops->read_blocks(dev, lba, count, buffer);
}

int write_block_dev(block_device_t *dev, uint64_t lba, uint32_t count, void *buffer) {
	if (dev == NULL || dev->ops == NULL || dev->ops->write_blocks == NULL || count == 0 || buffer == NULL){
		return -1;
	}

	return dev->ops->write_blocks(dev, lba, count, buffer);
}

int list_blkdevs() {
	if (blkdevs_list == NULL || blkdevs_list->count == 0) {
		printf("No block devices found\n");
		return 0;
	}
	printf("NAME\tMAJ:MIN\tRM\tSIZE\tRO\tTYPE\tMOUNTPOINTS\n");
	for (uint32_t i = 0; i < blkdevs_list->count; i++) {
		block_device_t *blk = list_get_at(blkdevs_list, i);
		printf("%s\n", blk->name);
	}
	return 0;
}

block_device_t *fetch_from_list(char *name) {
	if (blkdevs_list == NULL || blkdevs_list->count == 0){
		return NULL;
	}
	block_device_t *blk = NULL;
	for (uint32_t i = 0; i < blkdevs_list->count; i++) {
		blk = list_get_at(blkdevs_list, i);
		if (strncmp(blk->name, name, 3) == 0) {
			return blk;
		}
	}
	return NULL;
}