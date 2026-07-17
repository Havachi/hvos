#include "kernel/fs/ext2.h"
#include "kernel/fs/block_dev.h"
#include "mem/mem.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

ext2_sb_t *drive_sb = NULL;


void init_ext2(){
	ext2_sb_t *check_sb = read_sb();
	if (check_sb == NULL){
		//No sb found
		printf("[EXT2] No superblock found on disk\n");
		ext2_sb_t *sb = phys_alloc_sb();
		sb = init_sb(sb);
		write_sb(sb);
		drive_sb = read_sb();
	} else {
		//sb found
		printf("[EXT2] Superblock found on disk\n");
	}
}

uint32_t mkfs_ext2(block_device_t *blkdev) {
	
}