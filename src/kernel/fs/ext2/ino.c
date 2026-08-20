#include "kernel/fs/ext2.h"
#include <errno-list.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>



static void ext2_get_inode_location(ext2_fs_t *fs, uint32_t inode_id, uint32_t *target_block, uint32_t *byte_offset) {
	uint32_t ine_sz = (fs->sb->ext.s_inode_size > 0) ? fs->sb->ext.s_inode_size : 128;
	uint32_t ino_idx = inode_id - 1;
	uint32_t group = ino_idx / fs->sb->base.s_ine_per_group;
	uint32_t index_in_group = ino_idx % fs->sb->base.s_ine_per_group;

	ext2_bgd_t *bgd = &fs->bgdt[group];
	uint32_t total_byte_offset = index_in_group * ine_sz;

	*target_block = bgd->bg_inode_table + (total_byte_offset / fs->block_size);
	*byte_offset = total_byte_offset % fs->block_size;
}

int ext2_inode_read(ext2_fs_t *fs, ino_t ino_id, ext2_ine_t *inode) {
	if ( !fs || !inode || ino_id == 0)
		return -EINVAL;
	if (!fs->dev || !fs->dev->ops || !fs->dev->ops->read_blocks)
		return -DEVICE_UNAVAILABLE;

	uint32_t target_blk, byte_offset;
	ext2_get_inode_location(fs, ino_id, &target_blk, &byte_offset);
	
	uint8_t blkbuf[fs->block_size];
	int err = ext2_read_block(fs, target_blk, blkbuf);
	if (err < 0) return err;

	memcpy(inode, blkbuf + byte_offset, sizeof(ext2_ine_t));
	return OPERATION_SUCCESS;
}

int ext2_inode_write(ext2_fs_t *fs, ino_t ino_id, const ext2_ine_t *inode) {
	if (!inode || !fs || ino_id == 0)
		return -EINVAL;
	if (!fs->dev || !fs->dev->ops || !fs->dev->ops->write_blocks || !fs->dev->ops->read_blocks)
		return -DEVICE_UNAVAILABLE;

	uint32_t target_blk, byte_offset;
	ext2_get_inode_location(fs, ino_id, &target_blk, &byte_offset);

	uint8_t blkbuf[fs->block_size];
	int err = ext2_read_block(fs, target_blk, blkbuf);
	if (err < 0) return err;

	memcpy(blkbuf + byte_offset, inode, sizeof(ext2_ine_t));
	err = ext2_write_block(fs, target_blk, blkbuf);
	if (err < 0) return err;
	
	return OPERATION_SUCCESS;
}

int ext2_inode_alloc(ext2_fs_t *fs, uint32_t group_hint, uint16_t mode, ino_t *out_ino_id) {
	if (!fs || !out_ino_id)
		return -EINVAL;
	if (!fs->dev || !fs->dev->ops || !fs->dev->ops->read_blocks || !fs->sb)
		return -DEVICE_UNAVAILABLE;
	if(fs->sb->base.s_free_blocks_count == 0)
		return -ENOSPC;

	uint32_t group = group_hint % fs->group_count;
	int found_group = -1;

	for (uint32_t i = 0; i < fs->group_count; i++) {
		if (fs->bgdt[group].bg_free_inodes_count > 0){
			found_group = (int)group;
			break;
		}
		group = (group + 1) % fs->group_count;
	}
	if(found_group == -1) return -ENOSPC;

	ext2_bgd_t *bgd = &fs->bgdt[found_group];
	
	//Inode bitmap modifications
	bitmap_t *bmap = ext2_read_bitmap(fs->dev, bgd->bg_inode_bitmap, fs->sb->base.s_ine_per_group);
	if (bmap == NULL) return -OPERATION_FAILED;
	int bit_index = next_free_bitmap(bmap);
	if (bit_index == -1) {
		kfree(bmap);
		return -ENOSPC;
	}
	set_bitmap(bmap, bit_index);
	ext2_write_bitmap(fs->dev, bgd->bg_inode_bitmap, fs->sb->base.s_ine_per_group, bmap);
	kfree(bmap);

	bgd->bg_free_inodes_count--;
	if ((mode & S_IFMT) == INE_TYPE_DIR){
		bgd->bg_used_dirs_count++;
	}
	fs->bgdt_dirty = true;
	fs->sb->base.s_free_inode_count--;
	fs->sb_dirty = true;

	ino_t ino = (found_group * fs->sb->base.s_ine_per_group) + bit_index + 1;
	ext2_ine_t new_ino = {0};
	new_ino.i_mode = mode;
	new_ino.i_links_count = 1;
	int err = ext2_inode_write(fs, ino, &new_ino);
	if (err < 0) return err;

	*out_ino_id = ino;
	return OPERATION_SUCCESS;
}

int ext2_inode_free(ext2_fs_t *fs, ino_t ino_id) {
	if (!fs || ino_id == 0)
		return -EINVAL;
	if (!fs->dev || !fs->dev->ops || !fs->dev->ops->read_blocks || !fs->sb)
		return -DEVICE_UNAVAILABLE;

	uint32_t ipg = fs->sb->base.s_ine_per_group;
	uint32_t ino_idx = ino_id - 1;
	uint32_t group = ino_idx / ipg;
	uint32_t bit_index = ino_idx % ipg;
	if (group >= fs->group_count)
		return -EINVAL;
	ext2_ine_t inode;
	int err = ext2_inode_read(fs, ino_id, &inode);
	if (err < 0) return err;
	bool is_dir = ((inode.i_mode & S_IFMT) == INE_TYPE_DIR);
	ext2_bgd_t *bgd = &fs->bgdt[group];
	bitmap_t *bmap = ext2_read_bitmap(fs->dev, bgd->bg_inode_bitmap, ipg);
	if (!bmap) return -OPERATION_FAILED;
	clear_bitmap(bmap, bit_index);
	uint32_t written = ext2_write_bitmap(fs->dev, bgd->bg_inode_bitmap, ipg, bmap);
	kfree(bmap);
	if (written == 0) return -DEVICE_WRITE_ERROR;

	memset(&inode, 0, sizeof(ext2_ine_t));
	inode.i_dtime = 0;
	inode.i_links_count = 0;
	err = ext2_inode_write(fs, ino_id, &inode);
	if (err < 0) return err;

	bgd->bg_free_inodes_count++;
	if (is_dir && bgd->bg_used_dirs_count > 0)
		bgd->bg_used_dirs_count--;
	fs->bgdt_dirty = true;
	fs->sb->base.s_free_inode_count++;
	fs->sb_dirty = true;
	return OPERATION_SUCCESS;
}

int ext2_free_inode_blocks(ext2_fs_t *fs, ext2_ine_t *inode) {
	if (!fs || !inode) return -EINVAL;
	uint32_t blocks_in_inode = (inode->i_size + fs->block_size - 1) / fs->block_size;

	for (uint32_t lblk = 0; lblk < blocks_in_inode; lblk++){
		uint32_t pblk = 0;
		if (ext2_bmap(fs, inode, lblk, 0, &pblk) == 0 && pblk != 0)
			ext2_free_block(fs, pblk);
	}

	inode->i_blocks = 0;
	inode->i_size = 0;
	memset(inode->i_block, 0, sizeof(inode->i_block));
	return OPERATION_SUCCESS;
}