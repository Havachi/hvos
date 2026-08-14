#include "kernel/fs/ext2.h"
#include <stdint.h>



static void get_bgd_loc(uint32_t bg_id, uint32_t block_size, uint32_t *target_block, uint32_t *entry_index) {
	uint32_t bgd_per_block = block_size / sizeof(ext2_bgd_t);
	uint32_t bgdt_start = (1024 / block_size) + 1;
	*target_block = bgdt_start + (bg_id / bgd_per_block);
	*entry_index = bg_id % bgd_per_block;
}


ext2_bgd_t *ext2_read_bgdt_entry(block_device_t *dev, ext2_sb_t *sb, uint32_t bg_id) {
	uint32_t target_block = 0;
	uint32_t entry_idx = 0;
	get_bgd_loc(bg_id, C_BLKSZ(sb->base.s_log_block_size), &target_block, &entry_idx);

	uint8_t block_buffer[C_BLKSZ(sb->base.s_log_block_size)];
	if(dev->ops->read_blocks(dev, target_block, 1, (void *)block_buffer) != 0) {
		printf("[EXT2] Error while writing ext2 BGDT\n");
		return NULL;
	}

	ext2_bgd_t *tbl = (ext2_bgd_t *)block_buffer;
	ext2_bgd_t entry = tbl[entry_idx];
	ext2_bgd_t *ret = kmalloc(sizeof(ext2_bgd_t));
	memcpy(ret, &entry, sizeof(ext2_bgd_t));
	return ret;
}

int ext2_write_bgdt_entry(block_device_t *dev, ext2_sb_t *sb, uint32_t bg_id, ext2_bgd_t *entry) {

	uint32_t target_block = 0;
	uint32_t entry_idx = 0;
	get_bgd_loc(bg_id, C_BLKSZ(sb->base.s_log_block_size), &target_block, &entry_idx);


	uint8_t block_buffer[C_BLKSZ(sb->base.s_log_block_size)];
	if(dev->ops->read_blocks(dev, target_block, 1, (void *)block_buffer) != 0) {
		printf("[EXT2] Error while reading disk\n");
		return -IO_ERROR;
	}

	ext2_bgd_t *tbl = (ext2_bgd_t *)block_buffer;
	ext2_bgd_t cur = tbl[entry_idx];
	memcpy(&cur, entry, sizeof(ext2_bgd_t));

	if (dev->ops->write_blocks(dev, target_block, 1, (void *)block_buffer) != 0) {
		printf("[EXT2] Error while writing on disk\n");
		return -IO_ERROR;
	}
	return OPERATION_SUCCESS;
}

int mkfs_ext2_bgdt_init(block_device_t *dev, ext2_sb_t *sb) {
	uint32_t total_bg = dev->total_bg;
	uint32_t blksz = C_BLKSZ(sb->base.s_log_block_size);
	uint32_t bgd_per_block = blksz / sizeof(ext2_bgd_t);
	uint32_t bgdt_blocks = (total_bg + bgd_per_block - 1) / bgd_per_block;

	
	uint32_t blk_per_grp = sb->base.s_blocks_per_group;
	uint32_t inode_size = sb->ext.s_inode_size ? sb->ext.s_inode_size : 128;
	uint32_t inode_table_blocks = (sb->base.s_ine_per_group * inode_size + blksz - 1) / blksz;

	uint8_t blkbuf[blksz];
	memset(blkbuf, 0, blksz);

	uint32_t bgdt_start_block = (1024 / blksz) + 1;

	for (uint32_t i = 0; i < total_bg; i++) {
		uint32_t idx_in_block = i % bgd_per_block;

		if (i > 0 && idx_in_block == 0) {
			uint32_t prev_block = bgdt_start_block + (i / bgd_per_block) - 1;
			if (dev->ops->write_blocks(dev, prev_block, 1, (void *)blkbuf) != 0) return -IO_ERROR;
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

		entry->bg_block_bitmap = group_start + super_overhead;
		entry->bg_inode_bitmap = entry->bg_block_bitmap + 1;
		entry->bg_inode_table = entry->bg_inode_bitmap + 1;

		uint32_t base_overhead = super_overhead + 1 + 1 + inode_table_blocks;
		entry->bg_free_blocks_count = group_blocks - base_overhead;
		entry->bg_free_inodes_count = sb->base.s_ine_per_group;
		entry->bg_used_dirs_count = 0;

		bitmap_t *blkmap = ext2_read_bitmap(dev, entry->bg_block_bitmap, sb->base.s_blocks_per_group);
		if (blkmap == NULL) return -IO_ERROR;
		if (i == 0) {
			uint32_t grp0_alloc_blks = base_overhead + 1;
			set_bitmap_n(blkmap, 0, grp0_alloc_blks);
			
			bitmap_t *inemap = ext2_read_bitmap(dev, entry->bg_inode_bitmap, sb->base.s_ine_per_group);
			if (inemap == NULL) {
				free_bitmap(blkmap);
				return -IO_ERROR;
			}

			set_bitmap_n(inemap, 0, EXT2_RESERVED_INODES);
			if (ext2_write_bitmap(dev, entry->bg_inode_bitmap, sb->base.s_ine_per_group, inemap) != 0) {
				printf("[EXT2] failed writing inode bitmap");
				free_bitmap(blkmap);
				free_bitmap(inemap);
				return -IO_ERROR;
			}
			free_bitmap(inemap);
			
			entry->bg_free_blocks_count = group_blocks - grp0_alloc_blks;
			entry->bg_free_inodes_count = sb->base.s_ine_per_group - EXT2_RESERVED_INODES;
			entry->bg_used_dirs_count = 1;
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
			return -IO_ERROR;
		}
		free_bitmap(blkmap);
	}
	uint32_t last_bgdt_block = bgdt_start_block + bgdt_blocks - 1;
	if (dev->ops->write_blocks(dev, last_bgdt_block, 1, blkbuf) != 0) {
		return -IO_ERROR;
	}
	return OPERATION_SUCCESS;
}