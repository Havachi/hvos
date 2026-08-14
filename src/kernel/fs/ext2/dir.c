#include "kernel/fs/ext2.h"

ext2_dir_t *ext2_get_dir(block_device_t *dev, ext2_sb_t *sb, ino_t inode_number, char *name) {
	ext2_ine_t *inode = ext2_read_inode(dev, sb, inode_number);
	if (inode == NULL) return NULL;
	if (inode->i_mode != INE_TYPE_DIR) {
		kfree(inode);
		return NULL;
	}

	uint32_t blksz = C_BLKSZ(sb->base.s_log_block_size);

	uint8_t blkbuf[blksz];
	uint32_t dir_block = inode->i_block[0];
	if (dev->ops->read_blocks(dev, dir_block, 1, blkbuf) != 0) {
		return NULL;
	}

	uint32_t offset = 0;
	while (offset < blksz && offset < inode->i_size) {
		ext2_dir_t *entry = (ext2_dir_t *)(blkbuf + offset);
		if (entry->rec_len == 0){
			break;
		}
		if (entry->inode != 0) {
			if (memcmp(name, entry->name, entry->name_len) == 0) {
				kfree(inode);
				return entry;
			}
		}
		offset += entry->rec_len;
	}
	return NULL;
}