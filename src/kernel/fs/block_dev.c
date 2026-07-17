#include "kernel/fs/block_dev.h"
#include "mem/mem.h"
#include <string.h>

block_device_t *new_block_dev(char *name) {
	block_device_t *dev = kzalloc(sizeof(block_device_t));
	if (dev == NULL)
		return NULL;
	strncpy(dev->name, name, 20);

	return dev;
}

int read_block_dev(block_device_t *dev, uint64_t lba, uint32_t count, void *buffer) {
	if (dev == NULL || dev->read_blocks == NULL || count == 0 || buffer == NULL){
		return -1;
	}

	return dev->read_blocks(dev, lba, count, buffer);
}

int write_block_dev(block_device_t *dev, uint64_t lba, uint32_t count, void *buffer) {
	if (dev == NULL || dev->write_blocks == NULL || count == 0 || buffer == NULL){
		return -1;
	}

	return dev->write_blocks(dev, lba, count, buffer);
}