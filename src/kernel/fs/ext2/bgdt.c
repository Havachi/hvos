#include "kernel/fs/ext2.h"
#include <errno-list.h>
#include <stdint.h>



static void get_bgd_loc(uint32_t bg_id, uint32_t block_size, uint32_t *target_block, uint32_t *entry_index) {
	uint32_t bgd_per_block = block_size / sizeof(ext2_bgd_t);
	uint32_t bgdt_start = (1024 / block_size) + 1;
	*target_block = bgdt_start + (bg_id / bgd_per_block);
	*entry_index = bg_id % bgd_per_block;
}


int ext2_bgdt_read_entry(ext2_fs_t *fs, uint32_t bg_id, ext2_bgd_t *out) {
	if (out == NULL)
		return -OPERATION_FAILED;
	uint32_t target_block = 0;
	uint32_t entry_idx = 0;
	get_bgd_loc(bg_id, fs->block_size, &target_block, &entry_idx);

	uint8_t block_buffer[fs->block_size];
	if (ext2_read_block(fs, target_block, block_buffer) != OPERATION_SUCCESS) {
		printf("[EXT2] Error while writing ext2 BGDT\n");
		return -DEVICE_WRITE_ERROR;
	}

	ext2_bgd_t *tbl = (ext2_bgd_t *)block_buffer;
	ext2_bgd_t entry = tbl[entry_idx];
	memcpy(out, &entry, sizeof(ext2_bgd_t));
	return OPERATION_SUCCESS;
}

int ext2_bgdt_read_entry_raw(block_device_t *dev, uint32_t bg_id, ext2_bgd_t *out) {
	if (out == NULL)
		return -OPERATION_FAILED;
	uint32_t target_block = 0;
	uint32_t entry_idx = 0;
	get_bgd_loc(bg_id, dev->block_size, &target_block, &entry_idx);
	uint8_t block_buffer[dev->block_size];
	if (ext2_read_blocks(dev, target_block, target_block, block_buffer) != OPERATION_SUCCESS) {
		printf("[EXT2] Error while writing ext2 BGDT\n");
		return -DEVICE_READ_ERROR;
	}
	ext2_bgd_t *tbl = (ext2_bgd_t *)block_buffer;
	ext2_bgd_t entry = tbl[entry_idx];
	memcpy(out, &entry, sizeof(ext2_bgd_t));
	return OPERATION_SUCCESS;
}

int ext2_bgdt_write_entry(ext2_fs_t *fs, uint32_t bg_id, const ext2_bgd_t *entry) {
	uint32_t target_block = 0;
	uint32_t entry_idx = 0;
	get_bgd_loc(bg_id, fs->block_size, &target_block, &entry_idx);


	uint8_t block_buffer[fs->block_size];
	if(ext2_read_block(fs, target_block, block_buffer) != 0) {
		printf("[EXT2] Error while reading disk\n");
		return -DEVICE_READ_ERROR;
	}

	ext2_bgd_t *tbl = (ext2_bgd_t *)block_buffer;
	ext2_bgd_t cur = tbl[entry_idx];
	memcpy(&cur, entry, sizeof(ext2_bgd_t));

	if (ext2_write_block(fs, target_block, block_buffer)) {
		printf("[EXT2] Error while writing on disk\n");
		return -DEVICE_WRITE_ERROR;
	}
	return OPERATION_SUCCESS;
}

int mkfs_ext2_bgdt_init(block_device_t *dev, ext2_sb_t *sb, ext2_bgd_t *grp0_bgd) {
	if (!dev || !sb || !grp0_bgd) {
		return -EINVAL;
	}

	uint32_t total_bg = dev->total_bg;
	uint32_t blksz = C_BLKSZ(sb->base.s_log_block_size);
	uint32_t bgd_per_block = blksz / sizeof(ext2_bgd_t);
	uint32_t bgdt_blocks = (total_bg + bgd_per_block - 1) / bgd_per_block;

	
	uint32_t blk_per_grp = sb->base.s_blocks_per_group;
	uint32_t inode_size = sb->ext.s_inode_size ? sb->ext.s_inode_size : 128;
	uint32_t inode_table_blocks = (sb->base.s_ine_per_group * inode_size + blksz - 1) / blksz;

	uint8_t *blkbuf = kmalloc(blksz);
	if (!blkbuf) return -ENOMEM;
	memset(blkbuf, 0, blksz);

	uint32_t bgdt_start_block = sb->base.s_first_data_block + 1;
	int status = OPERATION_SUCCESS;

	for (uint32_t i = 0; i < total_bg; i++) {
		uint32_t idx_in_block = i % bgd_per_block;

		if (i > 0 && idx_in_block == 0) {
			uint32_t prev_block = bgdt_start_block + (i / bgd_per_block) - 1;
			if (dev->ops->write_blocks(dev, prev_block, 1, (void *)blkbuf) != 0) {
				status = -IO_ERROR;
				goto cleanup;
			}
			memset(blkbuf, 0, blksz);
		}

		ext2_bgd_t *entry = &((ext2_bgd_t *)blkbuf)[idx_in_block];

		uint32_t group_start = sb->base.s_first_data_block + (i * blk_per_grp);
		uint32_t group_blocks = blk_per_grp;

		if ( i == total_bg - 1) {
			uint32_t remaining = sb->base.s_blocks_count - group_start;
			if (remaining < blk_per_grp) {
				group_blocks = remaining;
			}
		}

		uint32_t super_overhead = blkgrp_has_super(i) ? (1 + bgdt_blocks) : 0;
		uint32_t base_overhead = super_overhead + 1 + 1 + inode_table_blocks;

		entry->bg_block_bitmap = group_start + super_overhead;
		entry->bg_inode_bitmap = entry->bg_block_bitmap + 1;
		entry->bg_inode_table = entry->bg_inode_bitmap + 1;


		bitmap_t *blkmap = ext2_read_bitmap(dev, entry->bg_block_bitmap, sb->base.s_blocks_per_group);
		if (blkmap == NULL) {
			status = -IO_ERROR;
			goto cleanup;
		}

		if (i == 0) {
			uint32_t grp0_alloc_blks = base_overhead + 1;
			set_bitmap_n(blkmap, 0, grp0_alloc_blks);
			
			bitmap_t *inemap = ext2_read_bitmap(dev, entry->bg_inode_bitmap, sb->base.s_ine_per_group);
			if (inemap == NULL) {
				free_bitmap(blkmap);
				status = -IO_ERROR;
				goto cleanup;
			}

			set_bitmap_n(inemap, 0, EXT2_RESERVED_INODES);
			if (ext2_write_bitmap(dev, entry->bg_inode_bitmap, sb->base.s_ine_per_group, inemap) != 0) {
				printf("[EXT2] failed writing inode bitmap");
				free_bitmap(blkmap);
				free_bitmap(inemap);
				status = -IO_ERROR;
				goto cleanup;
			}
			free_bitmap(inemap);
			
			entry->bg_free_blocks_count = group_blocks - grp0_alloc_blks;
			entry->bg_free_inodes_count = sb->base.s_ine_per_group - EXT2_RESERVED_INODES;
			entry->bg_used_dirs_count = 1;
			if (grp0_bgd) {
				*grp0_bgd = *entry;
			}
		} else {
			entry->bg_free_blocks_count = group_blocks - base_overhead;
			entry->bg_free_inodes_count = sb->base.s_ine_per_group;
			entry->bg_used_dirs_count = 0;
			set_bitmap_n(blkmap, 0, base_overhead);
		}
		if (group_blocks < sb->base.s_blocks_per_group) {
			set_bitmap_n(blkmap, group_blocks, sb->base.s_blocks_per_group - group_blocks);
		}
		if (ext2_write_bitmap(dev, entry->bg_block_bitmap, sb->base.s_blocks_per_group, blkmap) != 0) {
			free_bitmap(blkmap);
			status = -IO_ERROR;
			goto cleanup;
		}
		free_bitmap(blkmap);
	}
	uint32_t last_bgdt_block = bgdt_start_block + bgdt_blocks - 1;
	if (dev->ops->write_blocks(dev, last_bgdt_block, 1, blkbuf) != 0) {
		status = -IO_ERROR;
		goto cleanup;
	}

cleanup:
	kfree(blkbuf);
	return status;
}


int ext2_bgdt_sync(ext2_fs_t *fs) {
	if (!fs || !fs->dev || !fs->sb || !fs->bgdt) return -EINVAL;
	uint32_t bgd_per_block = fs->block_size / sizeof(ext2_bgd_t);
	uint32_t bgdt_blocks = (fs->group_count + bgd_per_block - 1) / bgd_per_block;
	uint32_t bgdt_start_block = fs->sb->base.s_first_data_block + 1;
	
	if (ext2_write_blocks(fs->dev, bgdt_start_block,
	                      bgdt_start_block + bgdt_blocks - 1,
	                      (void *)fs->bgdt) < 0) {
		return -EIO;
	}
	return OPERATION_SUCCESS;
}
