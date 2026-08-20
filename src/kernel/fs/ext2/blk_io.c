#include "kernel/fs/ext2.h"
#include <errno-list.h>
#include <stdint.h>


int ext2_read_block_ptr(ext2_fs_t *fs, uint32_t pblk, uint32_t *buf) {
	return ext2_read_block(fs, pblk, (uint8_t *)buf);
}

int ext2_read_block(ext2_fs_t *fs, uint32_t blk_id, uint8_t *buf) {
	if (fs == NULL || fs->dev == NULL || fs->dev->ops == NULL || fs->dev->ops->read_blocks == NULL)
		return -DEVICE_UNAVAILABLE;
	if (buf == NULL)
		return -OPERATION_FAILED;
	ext2_read_blocks(fs->dev, blk_id, blk_id, buf);
	return OPERATION_SUCCESS;
}

int ext2_read_blocks(block_device_t *dev, uint32_t blk_start, uint32_t blk_last, uint8_t *buf) {
	if (dev == NULL || dev->ops == NULL || dev->ops->read_blocks == NULL)
		return -DEVICE_UNAVAILABLE;
	if (buf == NULL)
		return -OPERATION_FAILED;
	if (blk_start > blk_last)
		return -OPERATION_FAILED;
	uint32_t count = (blk_last - blk_start) + 1;
	if (dev->ops->read_blocks(dev, blk_start, count, buf)) {
		return -DEVICE_READ_ERROR;
	}
	return OPERATION_SUCCESS;
}

int ext2_write_block_ptr(ext2_fs_t *fs, uint32_t pblk, const uint32_t *buf) {
	return ext2_write_block(fs, pblk, (const uint8_t *)buf);
}


int ext2_write_block(ext2_fs_t *fs, uint32_t blk_id, const uint8_t *buf) {
	return ext2_write_blocks(fs->dev, blk_id, blk_id, buf);
}

int ext2_write_blocks(block_device_t *dev, uint32_t blk_start, uint32_t blk_last, const uint8_t *buf) {
	if (dev == NULL || dev->ops == NULL || dev->ops->write_blocks == NULL)
		return -DEVICE_UNAVAILABLE;
	if (buf == NULL)
		return -OPERATION_FAILED;
	if (blk_start > blk_last)
		return -OPERATION_FAILED;
	uint32_t count = (blk_last - blk_start) + 1;
	if (dev->ops->write_blocks(dev, blk_start, count, buf) != 0) {
		return -DEVICE_WRITE_ERROR;
	}
	return OPERATION_SUCCESS;
}

int ext2_alloc_block(ext2_fs_t *fs, uint32_t group_hint, uint32_t *out_pblk) {
	if (!fs || !out_pblk) return -EINVAL;
	if (!fs->dev || !fs->dev->ops || !fs->sb) return -DEVICE_UNAVAILABLE;
	if (fs->sb->base.s_free_blocks_count == 0) return -ENOSPC;

	uint32_t group = group_hint % fs->group_count;
	int found_group = -1;

	for (uint32_t i = 0; i < fs->group_count; i++) {
		if (fs->bgdt[group].bg_free_blocks_count > 0){
			found_group = (int)group;
			break;
		}
		group = (group + 1) % fs->group_count;
	}

	if(found_group == -1) return -ENOSPC;
	ext2_bgd_t *bgd = &fs->bgdt[found_group];
	bitmap_t *bmap = ext2_read_bitmap(fs->dev, bgd->bg_block_bitmap, fs->sb->base.s_blocks_per_group);
	if (!bmap) return -OPERATION_FAILED;
	int bit_index = next_free_bitmap(bmap);
	if (bit_index == -1) {
		free_bitmap(bmap);
		return -ENOSPC;
	}

	set_bitmap(bmap, bit_index);

	int err = ext2_write_bitmap(fs->dev, bgd->bg_block_bitmap, fs->sb->base.s_blocks_per_group, bmap);
	if (err < 0) return -DEVICE_WRITE_ERROR;

	bgd->bg_free_blocks_count--;
	fs->bgdt_dirty = true;
	fs->sb->base.s_free_blocks_count--;
	fs->sb_dirty = true;

	uint32_t first_block = fs->sb->base.s_first_data_block;
	uint32_t pblk = first_block + (found_group * fs->sb->base.s_blocks_per_group) + bit_index;

	*out_pblk = pblk;
	free_bitmap(bmap);
	return OPERATION_SUCCESS;
}

int ext2_free_block(ext2_fs_t *fs, uint32_t pblk) {
	if (!fs) return -EINVAL;
	if (!fs->dev || !fs->dev->ops || !fs->sb) return -DEVICE_UNAVAILABLE;
	uint32_t first_block = fs->sb->base.s_first_data_block;
	if (pblk < first_block || pblk >= fs->block_count) return -EINVAL;

	uint32_t relative_blk = pblk - first_block;
	uint32_t group = relative_blk / fs->sb->base.s_blocks_per_group;
	uint32_t bit_index = relative_blk % fs->sb->base.s_blocks_per_group;

	if (group >= fs->group_count) return -EINVAL;

	ext2_bgd_t *bgd = &fs->bgdt[group];

	bitmap_t *bmap = ext2_read_bitmap(fs->dev, bgd->bg_block_bitmap, fs->sb->base.s_blocks_per_group);
	if (!bmap) return -OPERATION_FAILED;

	clear_bitmap(bmap, bit_index);
	int err = ext2_write_bitmap(fs->dev, bgd->bg_block_bitmap, fs->sb->base.s_blocks_per_group, bmap);
	if (err < 0) return -DEVICE_WRITE_ERROR;

	bgd->bg_free_blocks_count++;
	fs->bgdt_dirty = true;
	fs->sb->base.s_free_blocks_count++;
	fs->sb_dirty = true;
	free_bitmap(bmap);
	return OPERATION_SUCCESS;
}