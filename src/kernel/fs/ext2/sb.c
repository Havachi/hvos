#include "kernel/fs/ext2.h"
#include "kernel/fs/storage.h"
#include "mem/mem.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SB_B_ADDR 1024

ext2_sb_t *alloc_sb() {
	ext2_sb_t *sb = NULL;
	sb = kzalloc(sizeof(ext2_sb_t));
	if (sb == NULL)
		return NULL;
	return sb;
}

ext2_sb_t *phys_alloc_sb() {
	uint64_t sb_phys = (uint64_t)pmm_alloc();
	ext2_sb_t *sb = (ext2_sb_t *)PHYS_TO_VIRT(sb_phys);
	return sb;
}

ext2_sb_t *init_sb(ext2_sb_t *sb) {
	uint64_t nb_blk = g_boot_drive.nb_sect / 8;
	sb->base.total_ine = 0;
	sb->base.total_blk = nb_blk;

	sb->base.log2_blk_sz = 2;
	sb->base.magic = EXT2_SIGNATURE;
	sb->base.fs_state = EXT2_STATE_CLEAN;
	sb->base.err_hndl = EXT2_ERR_IGNORE;
	sb->base.os_id = OSID_OTHER;
	sb->base.v_major = 1;
	return sb;

}

ext2_sb_t *new_sb() {
	ext2_sb_t *sb = alloc_sb();
	sb = init_sb(sb);
	return sb;
}

ext2_sb_t *read_sb() {
	ext2_sb_t *sb = phys_alloc_sb();
	
	if (!storage_read_sectors(2, 2, (void *)sb)) {
		printf("[EXT2]failed to read sb block\n");
		return NULL;
	}

	if (sb->base.magic != EXT2_SIGNATURE) {
		return NULL;
	}

	return sb;
}

void write_sb(ext2_sb_t *sb){
	if (sb == NULL) {
		return;
	}
	storage_write_sectors(2, 2, (void *)sb);
	return;
}