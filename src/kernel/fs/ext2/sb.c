#include "kernel/fs/ext2.h"
#include "kernel/fs/storage.h"
#include "mem/mem.h"
#include <errno-list.h>
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

int ext2_sb_read(ext2_fs_t *fs, ext2_sb_t *out_sb) {
	if (!fs || !out_sb) return -EINVAL;
	if (!fs->dev || !fs->dev->ops || !fs->dev->ops->read_blocks) return -EINVAL;
	uint8_t blkbuf[fs->block_size];
	uint32_t sb_lba = (fs->block_size > 1024) ? 0 : 1;
	int err = ext2_read_block(fs, sb_lba, blkbuf);
	if (err < 0) return err;
	memcpy(out_sb, blkbuf, sizeof(ext2_sb_t));
	return OPERATION_SUCCESS;
}

int ext2_sb_read_raw(block_device_t *dev, ext2_sb_t *out_sb) {
	if (!out_sb || !dev) return -EINVAL;
	if (!dev->ops || !dev->ops->read_blocks) return -DEVICE_UNAVAILABLE;

	uint8_t *blkbuf = kzalloc(dev->block_size);
	uint32_t sb_lba = (dev->block_size > 1024) ? 0 : 1;
	if (dev->ops->read_blocks(dev, sb_lba, 1, blkbuf) != 0) {
		printf("Failed reading superblock from disk %s", dev->name);
		return -DEVICE_READ_ERROR;
	}
	memcpy(out_sb, blkbuf, sizeof(ext2_sb_t));
	kfree(blkbuf);
	return OPERATION_SUCCESS;
}

int ext2_sb_write(ext2_fs_t *fs, const ext2_sb_t *sb){
	if (!fs || !sb) return -EINVAL;

	if (!fs->dev|| !fs->dev->ops || !fs->dev->ops->write_blocks)
		return -DEVICE_UNAVAILABLE;

	uint8_t blkbuf[fs->block_size];
	uint32_t sb_lba = (fs->block_size > 1024) ? 0 : 1;

	memcpy(blkbuf, sb, fs->block_size);
	int err = ext2_write_block(fs, sb_lba, blkbuf);
	if (err < 0) return err;
	return OPERATION_SUCCESS;
}


int ext2_sb_write_raw(block_device_t *dev, const ext2_sb_t *sb){
	if (!dev || !sb) return -EINVAL;
	if (dev == NULL || dev->ops == NULL || dev->ops->write_blocks == NULL)
		return -DEVICE_UNAVAILABLE;

	uint8_t blkbuf[dev->block_size];
	uint32_t sb_lba = (dev->block_size > 1024) ? 0 : 1;
	if ((uint32_t)C_BLKSZ(sb->base.s_log_block_size) != dev->block_size)
		return -DEVICE_NEED_REFORMAT;
	memcpy(blkbuf, sb, dev->block_size);

	if (dev->ops->write_blocks(dev, sb_lba, 1, blkbuf) != 0) {
		return -DEVICE_WRITE_ERROR;
	}
	return OPERATION_SUCCESS;
}


static int ext2_write_backup_sb(block_device_t *dev, const ext2_sb_t *sb) {
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

int ext2_update_sb(ext2_fs_t *fs, const ext2_sb_t *sb) {
	if (ext2_sb_write(fs, sb) != 0)
		return -OPERATION_FAILED;
	if (ext2_write_backup_sb(fs->dev, sb))
		return -OPERATION_FAILED;
	return OPERATION_SUCCESS;
}

int ext2_update_sb_raw(block_device_t *dev, const ext2_sb_t *sb) {
	if (ext2_sb_write_raw(dev, sb) < 0)
		return -OPERATION_FAILED;
	if (ext2_write_backup_sb(dev, sb))
		return -OPERATION_FAILED;
	return OPERATION_SUCCESS;
}

int ext2_sb_sync(ext2_fs_t *fs) {
	
}