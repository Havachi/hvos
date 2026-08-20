#include "kernel/fs/ext2.h"
#include <errno-list.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

int ext2_dir_find_entry(ext2_fs_t *fs, const ext2_ine_t *dir_inode, const char *name, ino_t *out_ino) {
	if (!fs ||!dir_inode || !name || !out_ino) return -EINVAL;
	uint32_t name_len = strlen(name);
	if (name_len > EXT2_NAME_LEN) return -ENAMETOOLONG;
	uint8_t *buf = kzalloc(fs->block_size);
	if (!buf) return -ENOMEM;
	uint32_t num_blocks = dir_inode->i_size / fs->block_size;
	for (uint32_t i = 0; i < num_blocks; i++) {
		uint32_t pblk;
		if (ext2_bmap(fs, (ext2_ine_t*)dir_inode, i, 0, &pblk) < 0 || pblk == 0)
			continue;
		if (ext2_read_block(fs, pblk, buf) < 0)
			continue;
		uint32_t offset = 0;
		while (offset < fs->block_size) {
			ext2_dir_t *entry = (ext2_dir_t *) (buf + offset);
			if (entry->rec_len == 0) break;
			if (entry->inode != 0 && entry->name_len == name_len) {
				if (memcmp(entry->name, name, name_len) == 0) {
					*out_ino = entry->inode;
					kfree(buf);
					return OPERATION_SUCCESS;
				}
			}
			offset += entry->rec_len;
		}
	}
	kfree(buf);
	return -ENOENT;
}

int ext2_dir_add_entry(ext2_fs_t *fs, ino_t dir_ino, const char *name, ino_t ino_id, uint8_t file_type) {
	if (!fs || !name || dir_ino == 0 || ino_id == 0) return -EINVAL;
	uint32_t name_len = strlen(name);
	if (name_len > EXT2_NAME_LEN) return -ENAMETOOLONG;

	ext2_ine_t dir_inode;
	int err = ext2_inode_read(fs, dir_ino, &dir_inode);
	if (err < 0) return err;

	uint16_t needed_len = ((sizeof(ext2_dir_t) + name_len + 3) & ~3);
	uint8_t buf[fs->block_size];

	uint32_t num_blocks = (dir_inode.i_size + fs->block_size -1) / fs->block_size;
	for (uint32_t i = 0; i < num_blocks; i++) {
		uint32_t pblk;
		if (ext2_bmap(fs, &dir_inode, i, 0, &pblk) < 0 || pblk == 0) continue;
		if (ext2_read_block(fs, pblk, buf) < 0) continue;
		uint32_t offset = 0;
		while (offset < fs->block_size) {
			ext2_dir_t *entry = (ext2_dir_t *)(buf + offset);
			uint16_t actual_len = entry->inode ? ((sizeof(ext2_dir_t) + entry->name_len + 3) & ~3) : 0;
			if (entry->rec_len - actual_len >= needed_len) {
				if (entry->inode != 0) {
					uint16_t old_rec = entry->rec_len;
					entry->rec_len = actual_len;
					offset += actual_len;
					ext2_dir_t *new_entry = (ext2_dir_t *) (buf + offset);
					new_entry->inode = ino_id;
					new_entry->rec_len = old_rec - actual_len;
					new_entry->name_len = name_len;
					new_entry->file_type = file_type;
					memcpy(new_entry->name, name, name_len);
				} else {
					entry->inode = ino_id;
					entry->name_len = name_len;
					entry->file_type = file_type;
					memcpy(entry->name, name, name_len);
				}
				ext2_write_block(fs, pblk, buf);
				return OPERATION_SUCCESS;
			}
			offset += entry->rec_len;
		}
	}

	uint32_t new_pblk;
	err = ext2_bmap(fs, &dir_inode, num_blocks, 1, &new_pblk);
	if (err < 0) return err;
	memset(buf,0,fs->block_size);
	ext2_dir_t *new_entry = (ext2_dir_t *)buf;
	new_entry->inode = ino_id;
	new_entry->rec_len = fs->block_size;
	new_entry->name_len = name_len;
	new_entry->file_type = file_type;
	memcpy(new_entry->name, name, name_len);
	ext2_write_block(fs, new_pblk, buf);
	dir_inode.i_size += fs->block_size;
	ext2_inode_write(fs, dir_ino, &dir_inode);
	return OPERATION_SUCCESS;
}

int ext2_dir_remove_entry(ext2_fs_t *fs, ino_t dir_ino, const char *name) {
	if (!fs || !name || dir_ino == 0) return -EINVAL;
	uint32_t name_len = strlen(name);
	ext2_ine_t dir_inode;
	if (ext2_inode_read(fs, dir_ino, &dir_inode) < 0) return -EIO;
	uint8_t buf[fs->block_size];

	uint32_t num_blocks = dir_inode.i_size / fs->block_size;

	for (uint32_t i = 0; i < num_blocks; i++) {
		uint32_t pblk;
		if (ext2_bmap(fs, &dir_inode, i, 0, &pblk) < 0 || pblk == 0) continue;
		if (ext2_read_block(fs, pblk, buf) < 0) continue;
		uint32_t offset = 0;
		ext2_dir_t *prev_entry = NULL;
		while (offset < fs->block_size) {
			ext2_dir_t *entry = (ext2_dir_t *) (buf + offset);
			if (entry->inode != 0 && entry->name_len == name_len) {
				if (memcmp(entry->name, name, name_len) == 0) {
					if (prev_entry) {
						prev_entry->rec_len += entry->rec_len;
					} else {
						entry->inode = 0;
					}
					ext2_write_block(fs, pblk, buf);
					return OPERATION_SUCCESS;
				}
			}
			prev_entry = entry;
			offset += entry->rec_len;
		}
	}
	return -ENOENT;
}

int ext2_dir_readdir(ext2_fs_t *fs, const ext2_ine_t *dir_inode, uint32_t *offset, ext2_dir_t *out_entry) {
	if (!fs || ! dir_inode || !offset || !out_entry) return -EINVAL;

	uint8_t buf[fs->block_size];
	
	while (*offset < dir_inode->i_size) {
		uint32_t lblk = *offset / fs->block_size;
		uint32_t blk_off = *offset % fs->block_size;

		uint32_t pblk;
		if (ext2_bmap(fs, (ext2_ine_t*)dir_inode, lblk, 0, &pblk) < 0 || pblk == 0) {
			*offset = (lblk + 1) * fs->block_size;
			continue;
		}

		if (ext2_read_block(fs, pblk, buf) < 0) {
			return -EIO;
		}

		ext2_dir_t *entry = (ext2_dir_t *) (buf + blk_off);
		if (entry->rec_len == 0) {
			*offset = (lblk + 1) * fs->block_size;
			continue;
		}

		*offset += entry->rec_len;
		if (entry->inode != 0) {
			memcpy(out_entry, entry, entry->rec_len);
			return OPERATION_SUCCESS;
		}
	}
	return OPERATION_SUCCESS;
}