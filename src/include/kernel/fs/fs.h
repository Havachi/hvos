#ifndef KERNEL_FS_H
#define KERNEL_FS_H

#include "kernel/vfs.h"



typedef struct {
	const char *name;
	int flags;

} file_system_type_t;

typedef struct super_operation_s {
	inode_t *(*alloc_inode)(struct super_block *sb);
	void (*destroy_inode)(inode_t *);
	void (*free_inode)(inode_t *);
} super_operation_t;

int register_filesystem(file_system_type_t *);
int unregister_filesystem(file_system_type_t *);

#endif