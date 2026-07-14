#include "kernel/fs/ext2.h"
#include "mem/mem.h"

#define SB_B_ADDR 1024

ext2_sb_t *alloc_sb() {
	ext2_sb_t *sb = NULL;
	sb = kzalloc(sizeof(ext2_sb_t));
	if (sb == NULL)
		return NULL;
	return sb;
}

ext2_sb_t *read_sb() {

}

void write_sb(ext2_sb_t *sb){

}