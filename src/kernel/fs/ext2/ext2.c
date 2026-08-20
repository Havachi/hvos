#include "kernel/fs/ext2.h"
#include "kernel/time.h"
#include <errno-list.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>


#define cpu_to_le16(v) ((uint16_t)(((uint16_t)(v) >> 8) | ((uint16_t)(v) << 8)))
#define cpu_to_le32(v) ((uint32_t) ((uint32_t)(__builtin_bswap32(v))))

ext2_sb_t *drive_sb = NULL;

bool inode_table_cache_stale = false;
ext2_ine_t *inode_table_cache = NULL;

static int mkfs_ext2_sb(block_device_t *blkdev, ext2_sb_t *sb_out) {
	if (!blkdev || !sb_out)
		return -EINVAL;
	if (!blkdev->ops || !blkdev->ops->write_blocks || !blkdev->ops->read_blocks)
		return -DEVICE_UNAVAILABLE;
	
	int err = ext2_sb_read_raw(blkdev, sb_out);
	if (err < 0) return err;
	if (sb_out == NULL) return -OPERATION_FAILED;

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

	sb_out->base.s_inodes_count      = total_inodes;
	sb_out->base.s_blocks_count      = total_blocks;
	sb_out->base.s_r_blocks_count    = 0;
	sb_out->base.s_free_blocks_count = (total_blocks - 22) - 1;
	sb_out->base.s_free_inode_count  = (total_inodes - EXT2_RESERVED_INODES);
	sb_out->base.s_first_data_block  = (blkdev->block_size > 1024 ? (0) : 1);
	sb_out->base.s_log_block_size    = 0;
	sb_out->base.s_log_frag_size     = 0;
	sb_out->base.s_blocks_per_group  = BLOCK_PER_GROUP;
	sb_out->base.s_frags_per_group 	 = BLOCK_PER_GROUP;
	sb_out->base.s_ine_per_group     = INODE_PER_GROUP;

	// 16-bit fields
	sb_out->base.s_magic             = EXT2_SIGNATURE;
	sb_out->base.s_state             = EXT2_STATE_CLEAN;
	sb_out->base.s_errors            = EXT2_ERR_IGNORE;
	
	// 32-bit fields
	sb_out->base.s_creator_os        = OSID_OTHER;
	sb_out->base.s_rev_level         = 1;
	sb_out->ext.s_first_ino          = (EXT2_RESERVED_INODES + 1);
	
	// 16-bit field
	sb_out->ext.s_inode_size         = 256;
	
	// 32-bit timestamps
	
	sb_out->base.s_wtime             = (uint32_t)(time(NULL));

	gen_uuid(sb_out->ext.uuid);


	sb_out->ext.s_feature_ro_compat += RO_FEAT_SPARSE;
	err = ext2_update_sb_raw(blkdev, sb_out);
	if (err < 0) {
		printf("[EXT2] Error while writing ext2 superblock\n");
		return err;
	}
	return OPERATION_SUCCESS;
}

static int init_root_inode(ext2_fs_t *fs, uint32_t root_dir_blk) {
	ext2_ine_t *root_inode = NULL;
	root_inode = kzalloc(sizeof(ext2_ine_t));
	int err = ext2_inode_read(fs, EXT2_ROOT_INO, root_inode);
	if (err < 0) {
		return err;
	} 

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
	ext2_inode_write(fs, EXT2_ROOT_INO, root_inode);
	kfree(root_inode);
	return OPERATION_SUCCESS;
}

static int ext2_create_dot_dirs(block_device_t *dev, ext2_sb_t *sb, uint32_t root_dir_blk) {
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

static int update_block_usage(ext2_fs_t *fs) {
	uint32_t total = 0;

	if (!fs->dev || !fs->dev->ops->read_blocks || !fs->dev->ops->write_blocks) return -DEVICE_UNAVAILABLE;
	if (!fs->sb) return -OPERATION_FAILED;
	uint32_t bit_per_map = fs->sb->base.s_blocks_per_group;

	for(uint32_t i = 0; i < fs->dev->total_bg; i++) {
		uint32_t local_total = 0;
		ext2_bgd_t *entry = NULL;
		entry = kzalloc(sizeof(ext2_bgd_t));
		if (entry == NULL){
			return -ENOMEM;
		}
		if(ext2_bgdt_read_entry(fs, i, entry) != OPERATION_SUCCESS) {
			return -OPERATION_FAILED;
		}

		bitmap_t *blkmap = ext2_read_bitmap(fs->dev, entry->bg_block_bitmap, bit_per_map);
		if (blkmap == NULL){
			return -OPERATION_FAILED;
		}
		for (uint32_t j = 0; j < bit_per_map; j++){
			if (test_bitmap(blkmap, j) == 1) {
				total++;
				local_total++;
			}
		}
		entry->bg_free_blocks_count = fs->sb->base.s_blocks_per_group - local_total;
		if (ext2_bgdt_write_entry(fs, i, entry) != 0) {
			return -IO_ERROR;
		};
		free_bitmap(blkmap);
		kfree(entry);
	}
	fs->sb->base.s_free_blocks_count = fs->sb->base.s_blocks_count - total;
	ext2_update_sb(fs, fs->sb);
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


int mkfs_ext2(block_device_t *dev) {
	if (!dev || !dev->ops->read_blocks || !dev->ops->write_blocks)
		return -DEVICE_UNAVAILABLE;

	//Setup the superblock
	ext2_sb_t *sb = NULL;
	sb = kzalloc(sizeof(ext2_sb_t));
	if (!sb) return -ENOMEM;
	int err = mkfs_ext2_sb(dev, sb);
	if (err < 0) return err;

	//Setup the bgdt
	ext2_bgd_t *root_bgd = NULL;
	root_bgd = kzalloc(sizeof(ext2_bgd_t));
	if (root_bgd == NULL) return -ENOMEM;
	err = mkfs_ext2_bgdt_init(dev, sb, root_bgd);
	if (err < 0)
		return -err;

	uint32_t blksz = dev->block_size;
	uint32_t root_dir_blk = root_bgd->bg_inode_table + (sb->base.s_ine_per_group * sb->ext.s_inode_size + blksz - 1) / blksz;
	kfree(root_bgd);
	kfree(sb);

	ext2_fs_t *fs = kzalloc(sizeof(ext2_fs_t));
	if (!fs) {
		return -ENOMEM;
	}
	//Temporarly mount the fs
	err = ext2_mount(dev, &fs);
	if (err < 0){
		printf("[EXT2] Failed internal mount (%d)\n", err);
		kfree(fs);
		return err;
	}

	err = init_root_inode(fs, root_dir_blk);
	if (err < 0){
		printf("[EXT2] Failed creating root inode (%d)\n", err);
		ext2_unmount(fs);
		return err;
	}
	err = ext2_create_dot_dirs(fs->dev, fs->sb, root_dir_blk);
	if (err < 0){
		printf("[EXT2] Failed creating root inode dot dirs (%d)\n", err);
		ext2_unmount(fs);
		return err;
	}

	err = update_block_usage(fs);
	if(err) {
		printf("[EXT2] Failed updating block usage\n");
		ext2_unmount(fs);
		return err;
	}

	printf("[EXT2] Successfully created ext2 filesystem\n");
	ext2_unmount(fs);
	return OPERATION_SUCCESS;
}


/* API for VFS */


int ext2_mount(block_device_t *dev, ext2_fs_t **out_fs ) {
	if (!dev) return -EINVAL;

	ext2_sb_t *sb = kzalloc(sizeof(ext2_sb_t));
	if (!sb) return -ENOMEM;

	if (ext2_sb_read_raw(dev, sb) < 0) {
		kfree(sb);
		return -EIO;
	}

	if (sb->base.s_magic != 0xEF53){
		kfree(sb);
		return -DEVICE_NEED_REFORMAT;
	}

	ext2_fs_t *fs = kzalloc(sizeof(ext2_fs_t));
	if (!fs) {
		kfree(sb);
		return -ENOMEM;
	}

	fs->dev = dev;
	fs->sb = sb;

	fs->block_size = 1024U << fs->sb->base.s_log_block_size;
	fs->block_count = fs->sb->base.s_blocks_count;
	fs->group_count = (fs->sb->base.s_blocks_count + fs->sb->base.s_blocks_per_group - 1) / fs->sb->base.s_blocks_per_group;
	fs->inodes_per_block = fs->block_size / sizeof(ext2_ine_t);
	fs->pointer_per_block = fs->block_size / sizeof(uint32_t);
	
	uint32_t bgdt_bytes = fs->group_count * sizeof(ext2_bgd_t);
	fs->bgdt_blocks = (bgdt_bytes + fs->block_size - 1) / fs->block_size;
	uint32_t bgdt_start_block = (fs->block_size == 1024) ? 2 : 1;

	fs->bgdt = kzalloc(fs->bgdt_blocks * fs->block_size);
	if (!fs->bgdt) {
		kfree(sb);
		kfree(fs);
		return -ENOMEM;
	}


	if (ext2_read_blocks(fs->dev, bgdt_start_block,
	                     bgdt_start_block + fs->bgdt_blocks - 1,
	                     (uint8_t *)fs->bgdt) < 0) {
		kfree(fs->bgdt);
		kfree(sb);
		kfree(fs);
		return -EIO;
	}
	fs->sb->base.s_mtime = (uint32_t)(time(NULL));
	fs->sb_dirty = false;
	fs->bgdt_dirty = false;

	*out_fs = fs;
	return OPERATION_SUCCESS;
}

int ext2_unmount(ext2_fs_t *fs) {
	if(!fs) return -EINVAL;
	int status = OPERATION_SUCCESS;

	//Flush metadata
	if (fs->sb) {
		fs->sb->base.s_state = EXT2_STATE_CLEAN;
		fs->sb->base.s_wtime = (uint64_t)time(NULL);
		if (ext2_sb_sync(fs) < 0) {
			return -IO_ERROR;
		}

		if (ext2_bgdt_sync(fs) < 0) {
			return -IO_ERROR;
		}
	}

	kfree(fs->bgdt);
	kfree(fs->sb);
	kfree(fs->dev);
	kfree(fs);
	return OPERATION_SUCCESS;
}

int ext2_sync(ext2_fs_t *fs) {
	if (!fs) return -EINVAL;
	int err = 0;

	if (fs->sb_dirty){
		err = ext2_sb_write(fs, fs->sb);
		if (err < 0) return err;
		fs->sb_dirty = false;
	}

	if (fs->bgdt_dirty){
		uint32_t bgdt_start = (fs->block_size == 1024) ? 2 : 1;
		err = ext2_write_blocks(fs->dev, bgdt_start,
		                        bgdt_start + fs->bgdt_blocks - 1,
		                        (const uint8_t *)fs->bgdt);
		if (err < 0) return err;
		fs->bgdt_dirty = false;
	}

	return OPERATION_SUCCESS;
}

static int get_or_alloc_indirect(ext2_fs_t *fs, uint32_t *table_blk, uint32_t index, int create, uint32_t *out_pblk) {
	if (table_blk == 0) {
		if (!create) {
			*out_pblk = 0;
			return 0;
		}
		uint32_t new_table;
		int err = ext2_alloc_block(fs, 0, &new_table);
		if (err < 0) return err;
	
		uint8_t blkbuf[fs->block_size];
		ext2_write_block(fs, new_table, blkbuf);
		*table_blk = new_table;
	}
	uint8_t ptrs[fs->block_size];
	if (ext2_read_block(fs, *table_blk, (uint8_t *)ptrs) < 0) {
		return -DEVICE_READ_ERROR;
	}

	uint32_t target_pblk = ptrs[index];
	if (target_pblk == 0 && create) {
		int err = ext2_alloc_block(fs, 0, &target_pblk);
		if (err < 0)
			return err;
		ptrs[index] = target_pblk;
		ext2_write_block(fs, *table_blk, (uint8_t *)ptrs);
	}
	*out_pblk = target_pblk;
	return 0;
}

int ext2_bmap(ext2_fs_t *fs, ext2_ine_t *inode, uint32_t lblk, int create_if_missing, uint32_t *out_pblk) {
	if (!fs || !inode || !out_pblk) return -EINVAL;

	uint32_t np = fs->pointer_per_block;
	uint32_t direct_limit = 12;
	uint32_t single_limit = direct_limit + np;
	uint32_t double_limit = single_limit + (np * np);
	uint32_t triple_limit = double_limit + (np * np * np);
	if (lblk >=  triple_limit) return -EFBIG;
	
	if (lblk < direct_limit) {
		if (inode->i_block[lblk] == 0 && create_if_missing) {
			uint32_t new_blk;
			int err = ext2_alloc_block(fs, 0, &new_blk);
			if (err < 0) return err;
			inode->i_block[lblk] = new_blk;
		}
		*out_pblk = inode->i_block[lblk];
		return 0;
	}
	lblk -= direct_limit;
	if (lblk < np) {
		uint32_t table_blk = inode->i_block[EXT2_IND_BLOCK];
		int err = get_or_alloc_indirect(fs, &table_blk, lblk, create_if_missing, out_pblk);
		if (inode->i_block[EXT2_IND_BLOCK] != table_blk)
			inode->i_block[EXT2_IND_BLOCK] = table_blk;
		return err;
	}
	lblk -= np;
	if (lblk < (np * np)) {
		uint32_t idx1 = lblk / np;
		uint32_t idx2 = lblk % np;
		uint32_t table_blk1 = inode->i_block[EXT2_DIND_BLOCK];
		uint32_t tablk_blk2;
		int err = get_or_alloc_indirect(fs, &table_blk1, idx1, create_if_missing, &tablk_blk2);
		if (err < 0) return err;
		if (inode->i_block[EXT2_DIND_BLOCK] != table_blk1)
			inode->i_block[EXT2_DIND_BLOCK] = table_blk1;
		if (tablk_blk2 == 0) {
			*out_pblk = 0;
			return 0;
		}
		return get_or_alloc_indirect(fs, &tablk_blk2, idx2, create_if_missing, out_pblk);
	}

	lblk -= (np * np);

	uint32_t idx1 = lblk / (np * np);
	uint32_t idx2 = (lblk / np) % np;
	uint32_t idx3 = lblk % np;
	uint32_t table_blk1 = inode->i_block[EXT2_TIND_BLOCK];
	uint32_t table_blk2, table_blk3;
	int err = get_or_alloc_indirect(fs, &table_blk1, idx1, create_if_missing, &table_blk2);
	if (err < 0) return err;
	if (inode->i_block[EXT2_TIND_BLOCK] != table_blk1) {
		inode->i_block[EXT2_TIND_BLOCK] = table_blk1;
	}

	if (table_blk2 == 0){
		*out_pblk = 0;
		return 0;
	}
	err = get_or_alloc_indirect(fs, &table_blk2, idx2, create_if_missing, &table_blk3);
	if (err < 0 || table_blk3 == 0) {
		*out_pblk = 0;
		return err;
	}
	return get_or_alloc_indirect(fs, &table_blk3, idx3, create_if_missing, out_pblk);
}

int ext2_lookup_path(ext2_fs_t *fs, const char *path, ino_t cwd_ino, ino_t *out_ino) {
	if (!fs || !path || !out_ino) return -EINVAL;
	if (path[0] == '\0') return -ENOENT;
	ino_t current_ino;
	if (path[0] == '/') current_ino = EXT2_ROOT_INO;
	else current_ino = (cwd_ino != 0) ? cwd_ino : EXT2_ROOT_INO;

	char *path_copy = strdup(path);
	if (!path_copy) return -ENOMEM;

	char *curr = path_copy;
	ext2_ine_t current_inode;
	int err = ext2_inode_read(fs, current_ino, &current_inode);
	if (err < 0) {
		kfree(path_copy);
		return err;
	}
	uint32_t depth = 0;
	while (*curr != '\0') {
		while (*curr == '/') {
			curr++;
		}
		if (*curr == '\0') break;
		char *comp_start = curr;
		while (*curr != '\0' && *curr != '/') {
			curr++;
		}

		if (*curr == '/') {
			*curr = '\0';
			curr++;
		}

		if (++depth > MAX_PATH_DEPTH) {
			kfree(path_copy);
			return -ELOOP;
		}

		if ((current_inode.i_mode & S_IFMT) != INE_TYPE_DIR) {
			kfree(path_copy);
			return -ENOTDIR;
		}

		ino_t next_ino;
		err = ext2_dir_find_entry(fs, &current_inode, comp_start, &next_ino);
		if (err < 0) {
			kfree(path_copy);
			return err;
		}
		current_ino = next_ino;
        err = ext2_inode_read(fs, current_ino, &current_inode);
        if (err < 0) {
            kfree(path_copy);
            return err;
        }
	}
	kfree(path_copy);
	*out_ino = current_ino;
	return OPERATION_SUCCESS;
}