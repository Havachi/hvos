#ifndef HVOS_FS_RAMFS_H
#define HVOS_FS_RAMFS_H

#include "kernel/vfs.h"
#include "kernel/fs/tar.h"

#define RAMFS_MAX_CHILDREN 32


typedef struct ramfs_dir_data_s {
	dentry_t *children[RAMFS_MAX_CHILDREN];
	size_t child_count;
} ramfs_dir_data_t;


typedef struct ramfs_file_data_s {
	char *buffer;
	size_t capacity
} ramfs_file_data_t ;


void ramfs_init(void);

#endif