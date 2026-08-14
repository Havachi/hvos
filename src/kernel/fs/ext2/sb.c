#include "kernel/fs/ext2.h"
#include "kernel/fs/storage.h"
#include "mem/mem.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SB_B_ADDR 1024

/*
	Read 1024 bytes at offset 1024(b) to see if a superblock exist on disk
	Return null if:
		- failed to read disk
		- signature doesn't match ext2 signature
*/

ext2_sb_t *ext2_read_sb(block_device_t *dev) {
	ext2_sb_t *sb = NULL;
	uint8_t blkbuf[dev->block_size];

	if (dev == NULL || dev->ops == NULL || dev->ops->read_blocks == NULL)
		return NULL;

	
	uint32_t sb_lba = (dev->block_size > 1024) ? 0 : 1;
	if (dev->ops->read_blocks(dev, sb_lba, 1, blkbuf) != 0) {
		printf("Failed reading superblock from disk %s", dev->name);
		return NULL;
	}

	sb = kzalloc(sizeof(ext2_sb_t));
	memcpy(sb, blkbuf, sizeof(ext2_sb_t));
	return (sb);
}

int ext2_write_sb(block_device_t *dev, const ext2_sb_t *sb){
	uint8_t blkbuf[dev->block_size];
	if (dev == NULL || dev->ops == NULL || dev->ops->write_blocks == NULL)
		return -DEVICE_UNAVAILABLE;

	uint32_t sb_lba = (dev->block_size > 1024) ? 0 : 1;
	if ((uint32_t)C_BLKSZ(sb->base.s_log_block_size) != dev->block_size)
		return -DEVICE_NEED_REFORMAT;
	memcpy(blkbuf, sb, dev->block_size);

	if (dev->ops->write_blocks(dev, sb_lba, 1, blkbuf) != 0) {
		return -DEVICE_WRITE_ERROR;
	}
	return 0;
}


int ext2_write_backup_sb(block_device_t *dev, const ext2_sb_t *sb) {
	if (dev == NULL || dev->ops == NULL || dev->ops->write_blocks == NULL)
		return -DEVICE_UNAVAILABLE;

	if ((uint32_t)C_BLKSZ(sb->base.s_log_block_size) != dev->block_size || dev->block_size < 1024)
		return -DEVICE_NEED_REFORMAT;
	uint8_t blkbuf[dev->block_size];
	memset(blkbuf, 0, dev->block_size);
	size_t sb_copy_bytes = (sizeof(ext2_sb_t) < dev->block_size) ? sizeof(ext2_sb_t) : dev->block_size;
	memcpy(blkbuf, sb, sb_copy_bytes);
	
	for (uint32_t i = 1; i < dev->total_bg; i++) {
		if (blkgrp_has_super(i)) {
			uint64_t start_blk = (uint64_t)sb->base.s_first_data_block + ((uint64_t) i * sb->base.s_blocks_per_group);
			if (dev->ops->write_blocks(dev, start_blk, 1, blkbuf) != 0) {
				return -DEVICE_WRITE_ERROR;
			}
		}
	}
	return OPERATION_SUCCESS;
}

int ext2_update_sb(block_device_t *dev, const ext2_sb_t *sb) {
	if (ext2_write_sb(dev, sb) != 0)
		return -OPERATION_FAILED;
	if (ext2_write_backup_sb(dev, sb))
		return -OPERATION_FAILED;
	return OPERATION_SUCCESS;
}