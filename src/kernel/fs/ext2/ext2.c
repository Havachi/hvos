#include "kernel/fs/ext2.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>


#define cpu_to_le16(v) ((uint16_t)(((uint16_t)(v) >> 8) | ((uint16_t)(v) << 8)))
#define cpu_to_le32(v) ((uint32_t) ((uint32_t)(__builtin_bswap32(v))))

ext2_sb_t *drive_sb = NULL;

bool inode_table_cache_stale = false;
ext2_ine_t *inode_table_cache = NULL;

static ext2_sb_t *mkfs_ext2_sb(block_device_t *blkdev) {
	ext2_sb_t *sb = NULL;
	
	if (blkdev == NULL) return NULL;
	if (blkdev->block_size == 0) return NULL;
	if (blkdev->ops == NULL || blkdev->ops->write_blocks == NULL) return NULL;
	
	sb = ext2_read_sb(blkdev);

	if (sb == NULL) return NULL;

	//if total_bg is not set, initialize it
	if (blkdev->total_bg == 0) {
		uint8_t buf [1024];
		blkdev->ops->stat(blkdev, buf);
		blkdev_stat_t *st = (blkdev_stat_t *)buf;
		uint64_t sect_ratio = blkdev->block_size / st->logical_sector_size;
		blkdev->total_bg = (st->total_nb_lsect / sect_ratio) / BLOCK_PER_GROUP;
	}

	uint32_t total_blocks = blkdev->total_bg * BLOCK_PER_GROUP;
	uint32_t total_inodes = blkdev->total_bg * INODE_PER_GROUP;

	sb->base.s_inodes_count      = total_inodes;
	sb->base.s_blocks_count      = total_blocks;
	sb->base.s_r_blocks_count    = 0;
	sb->base.s_free_blocks_count = (total_blocks - 22) - 1;
	sb->base.s_free_inode_count  = (total_inodes - EXT2_RESERVED_INODES);
	sb->base.s_first_data_block  = (blkdev->block_size > 1024 ? (0) : 1);
	sb->base.s_log_block_size    = 0;
	sb->base.s_log_frag_size     = 0;
	sb->base.s_blocks_per_group  = BLOCK_PER_GROUP;
	sb->base.s_frags_per_group 	 = BLOCK_PER_GROUP;
	sb->base.s_ine_per_group     = INODE_PER_GROUP;

	// 16-bit fields
	sb->base.s_magic             = EXT2_SIGNATURE;   // Will correctly write as 53 ef
	sb->base.s_state             = EXT2_STATE_CLEAN;
	sb->base.s_errors            = EXT2_ERR_IGNORE;
	
	// 32-bit fields
	sb->base.s_creator_os        = OSID_OTHER;
	sb->base.s_rev_level         = 1;
	sb->ext.s_first_ino          = (EXT2_RESERVED_INODES + 1);
	
	// 16-bit field
	sb->ext.s_inode_size         = 256;
	
	// 32-bit timestamps
	
	sb->base.s_mtime             = (1786113181);
	sb->base.s_wtime             = (1786113182);

	gen_uuid(sb->ext.uuid);


	sb->ext.s_feature_ro_compat += RO_FEAT_SPARSE;

	if (ext2_update_sb(blkdev, sb) != 0) {
		printf("[EXT2] Error while writing ext2 superblock\n");
		return NULL;
	}
	return sb;
}

static void init_root_inode(block_device_t *dev, ext2_sb_t *sb, uint32_t root_dir_blk) {
	ext2_ine_t *root_inode = NULL;
	root_inode = ext2_read_inode(dev, sb, EXT2_ROOT_INO);
	root_inode->i_mode = INE_TYPE_DIR + 0x01ED;
	root_inode->i_uid = 0;
	root_inode->i_gid = 0;
	root_inode->i_size = 1024;
	root_inode->i_links_count = 2;
	root_inode->i_blocks = 2;
	root_inode->i_atime = 0x756A9DEC;
	root_inode->i_ctime = 0x756A9DEC;
	root_inode->i_mtime = 0x756A9DEC;
	root_inode->i_block[0] = root_dir_blk;
	ext2_write_inode(dev, sb, EXT2_ROOT_INO, root_inode);
	kfree(root_inode);
}

static int ext2_create_dot_dirs(block_device_t *dev, ext2_sb_t *sb, ext2_bgd_t *grp0_bgd, uint32_t root_dir_blk) {
	uint32_t blksz = C_BLKSZ(sb->base.s_log_block_size);

	uint8_t blkbuf[blksz];
	memset(blkbuf, 0, blksz);

	ext2_dir_t *dot = (ext2_dir_t *)blkbuf;
	dot->inode = EXT2_ROOT_INO;
	dot->rec_len = 12;
	dot->name_len = 1;
	dot->file_type = DIR_TYPE_DIR;
	dot->name[0] = '.';

	ext2_dir_t *dotdot = (ext2_dir_t *)(blkbuf + 12);
	dotdot->inode = EXT2_ROOT_INO;
	dotdot->rec_len = blksz - 12;
	dotdot->name_len = 2;
	dotdot->file_type = DIR_TYPE_DIR;
	dotdot->name[0] = '.';
	dotdot->name[1] = '.';

	if (dev->ops->write_blocks(dev, root_dir_blk, 1, blkbuf) != 0) {
		return -IO_ERROR;
	}
	return OPERATION_SUCCESS;
}

static int update_block_usage(block_device_t *dev, ext2_sb_t *sb) {
	uint32_t total = 0;
	uint32_t bit_per_map = sb->base.s_blocks_per_group;

	if (dev == NULL || dev->ops->read_blocks == NULL || dev->ops->write_blocks == NULL) return -DEVICE_UNAVAILABLE;
	if (sb == NULL) return -OPERATION_FAILED;

	for(uint32_t i = 0; i < dev->total_bg; i++) {
		uint32_t local_total = 0;
		ext2_bgd_t *entry = NULL; 
		entry = ext2_read_bgdt_entry(dev, sb, i);
		if (entry == NULL) {
			return -IO_ERROR;
		}
		bitmap_t *blkmap = ext2_read_bitmap(dev, entry->bg_block_bitmap, bit_per_map);
		if (blkmap == NULL){
			return -OPERATION_FAILED;
		}
		for (uint32_t j = 0; j < bit_per_map; j++){
			if (test_bitmap(blkmap, j) == 1) {
				total++;
				local_total++;
			}
		}
		entry->bg_free_blocks_count = sb->base.s_blocks_per_group - local_total;
		if (ext2_write_bgdt_entry(dev, sb, i, entry) != 0) {
			return -IO_ERROR;
		};
		free_bitmap(blkmap);
		kfree(entry);
	}
	sb->base.s_free_blocks_count = sb->base.s_blocks_count - total;
	ext2_update_sb(dev, sb);
	return OPERATION_SUCCESS;
}

static int is_power_of(uint32_t a, uint32_t b) {
	while (a > 1) {
		if (a % b != 0) return 0;
		a /= b;
	}
	return 1;
}

int blkgrp_has_super(uint32_t group) {
	if (group <= 1) return 1;
	return is_power_of(group, 3) ||is_power_of(group, 5) || is_power_of(group, 7);
}



int ext2_mkdir(block_device_t *dev, ext2_sb_t *sb, ino_t parent_ino, const char *name) {
	uint32_t blksz = C_BLKSZ(sb->base.s_log_block_size);
	uint32_t name_len = strlen(name);
	uint32_t req_entry_size = EXT2_DIR_REC_LEN(name_len);
}

bitmap_t *ext2_read_bitmap(block_device_t *dev, uint32_t loc, uint32_t total_bits) {
	bitmap_t *map = NULL;
	map = alloc_bitmap(total_bits);
	if (map == NULL) return NULL;
	uint32_t blksz = dev->block_size;
	uint8_t blkbuf[blksz];
	memset(blkbuf, 0, dev->block_size);

	if (dev->ops->read_blocks(dev, loc, 1, blkbuf) != 0){
		return NULL;
	}

	memcpy(map->data, blkbuf, (total_bits + 7) / 8);
	return map;
}

uint32_t ext2_write_bitmap(block_device_t *dev, uint32_t loc, uint32_t total_bits, bitmap_t *map) {
	uint32_t bitmap_bytes = (total_bits + 7) / 8;

	uint8_t blkbuf[dev->block_size];
	memset(blkbuf, 0, dev->block_size);
	memcpy(blkbuf, map->data, bitmap_bytes);
	if (dev->ops->write_blocks(dev, loc, 1, blkbuf) != 0) return -1;
	return OPERATION_SUCCESS;
}


uint32_t mkfs_ext2(block_device_t *blkdev) {
	//Setup the superblock
	ext2_sb_t *sb = mkfs_ext2_sb(blkdev);
	if (sb == NULL)
		return -OPERATION_FAILED;

	for (uint32_t i = 0; i < blkdev->total_bg; i++) {
		if (blkgrp_has_super(i)) {
		}
	}

	//Setup the bgdt
	if (mkfs_ext2_bgdt_init(blkdev, sb) != 0)
		return -OPERATION_FAILED;
	

	//get the bgd for group 0
	ext2_bgd_t *root_bgd = ext2_read_bgdt_entry(blkdev, sb, 0);

	uint32_t blksz = C_BLKSZ(sb->base.s_log_block_size);
	uint32_t root_dir_blk = root_bgd->bg_inode_table + (sb->base.s_ine_per_group * sb->ext.s_inode_size + blksz - 1) / blksz;
	
	init_root_inode(blkdev, sb, root_dir_blk);
	ext2_create_dot_dirs(blkdev, sb, root_bgd, root_dir_blk);
	if(update_block_usage(blkdev, sb) != OPERATION_SUCCESS) {
		printf("[EXT2] Failed updating block usage\n");
		return -OPERATION_FAILED;
	}

	printf("[EXT2] Successfully created ext2 filesystem\n");
	return OPERATION_SUCCESS;
}

/* API for VFS */


