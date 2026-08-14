#include "kernel/fs/ext2.h"



static void ext2_get_inode_location(block_device_t *dev, ext2_sb_t *sb, uint32_t inode_id, uint32_t *target_block, uint32_t *byte_offset) {
	uint32_t blksz = C_BLKSZ(sb->base.s_log_block_size);
	uint32_t ipg = sb->base.s_ine_per_group;
	uint32_t ine_sz = sb->ext.s_inode_size ? sb->ext.s_inode_size : 128;
	uint32_t ino_idx = inode_id - 1;
	ext2_bgd_t *bgd = ext2_read_bgdt_entry(dev, sb, (ino_idx / ipg));
	uint32_t total_byte_offset = (ino_idx % ipg) * ine_sz;
	*target_block = bgd->bg_inode_table + (total_byte_offset / blksz);
	*byte_offset = total_byte_offset % blksz;
}

ext2_ine_t *ext2_get_inode_table(block_device_t *dev, ext2_sb_t *sb, ext2_bgd_t *bgd) {
	uint32_t inode_table_loc = bgd->bg_inode_table;
	uint32_t blksz = C_BLKSZ(sb->base.s_log_block_size);
	uint32_t inode_table_blocks = ((sb->base.s_ine_per_group * sb->ext.s_inode_size) / blksz);

	uint8_t buf[(blksz * inode_table_blocks)];
	memset(buf, 0, (blksz * inode_table_blocks));
	if (dev->ops->read_blocks(dev, inode_table_loc, inode_table_blocks, buf) != 0) {
		return NULL;
	}
	ext2_ine_t *tbl = NULL;
	tbl = kzalloc((blksz * inode_table_blocks));
	memcpy(tbl, buf, (blksz * inode_table_blocks));
	
	return tbl;
}

ext2_ine_t *ext2_read_inode(block_device_t *dev, ext2_sb_t *sb, ino_t inode_id) {
	uint32_t target_block, byte_offset;
	uint32_t blksz = C_BLKSZ(sb->base.s_log_block_size);
	ext2_get_inode_location(dev, sb, inode_id, &target_block, &byte_offset);

	uint8_t blkbuf[blksz];
	
	if (dev->ops->read_blocks(dev, target_block, 1, blkbuf) != 0) {
		return NULL;
	}

	ext2_ine_t *inode = kzalloc(sizeof(ext2_ine_t));
	memcpy(inode, blkbuf + byte_offset, sizeof(ext2_ine_t));
	return inode;
}

int  ext2_write_inode(block_device_t *dev, ext2_sb_t *sb, ino_t inode_id, ext2_ine_t *inode) {
	uint32_t target_block, byte_offset;
	uint32_t blksz = C_BLKSZ(sb->base.s_log_block_size);
	ext2_get_inode_location(dev, sb, inode_id, &target_block, &byte_offset);

	uint8_t blkbuf[blksz];

	if (dev->ops->read_blocks(dev, target_block, 1, blkbuf) != 0) {
		return -IO_ERROR;
	}
	memcpy(blkbuf + byte_offset, inode, sizeof(ext2_ine_t));

	if (dev->ops->write_blocks(dev, target_block, 1, blkbuf) != 0) {
		return -IO_ERROR;
	}
	return OPERATION_SUCCESS;
}