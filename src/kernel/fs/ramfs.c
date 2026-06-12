#include "kernel/vfs.h"
#include "mem/mem.h"
#include <sys/types.h>
#include "kernel/fs/ramfs.h"
#include <string.h>
#include <stdlib.h>

static inode_ops_t ramfs_iops;
static file_ops_t ramfs_fops;

static dentry_t * ramfs_lookup(inode_t *dir, dentry_t *dentry) {
	ramfs_dir_data_t *dir_data = (ramfs_dir_data_t *)dir->i_private;
	if (!dir_data) return NULL;

	for (size_t i = 0; i < dir_data->child_count; i++) {
		dentry_t *child = dir_data->children[i];
		if (strcmp(child->d_name, dentry->d_name) == 0) {
			free_dentry(dentry);
			return child;
		}
	}
	return NULL;
}


static inode_t *ramfs_create_inode(mode_t mode) {
	inode_t *ino = alloc_inode();
	if (!ino) return NULL;
	static uint32_t next_ino_num = 100;
	ino->i_ino = next_ino_num++;
	ino->i_mode = mode;
	ino->i_op = &ramfs_iops;
	ino->i_fop = &ramfs_fops;

	if (mode & S_IFDIR) {
		ramfs_dir_data_t *ddata = kmalloc(sizeof(ramfs_dir_data_t));
		memset(ddata, 0, sizeof(ramfs_dir_data_t));
		ino->i_private = ddata;
		ino->i_size = 0;
	} else {
		ramfs_file_data_t *fdata = kmalloc(sizeof(ramfs_file_data_t));
		memset(fdata, 0, sizeof(ramfs_file_data_t));
		ino->i_private = fdata;
		ino->i_size = 0;
	}
	return ino;
}

static int ramfs_mkdir(inode_t *dir, dentry_t *dentry, mode_t mode) {
	ramfs_dir_data_t *dir_data = (ramfs_dir_data_t *)dir->i_private;
	if (!dir_data ||dir_data->child_count >= RAMFS_MAX_CHILDREN) return -1;

	inode_t *new_inode = ramfs_create_inode(S_IFDIR | mode);
	if (!new_inode) return -1;

	dentry->d_inode = new_inode;
	dir_data->children[dir_data->child_count++] = dentry;
	return 0;
}

static int ramfs_create(inode_t *dir, dentry_t *dentry, mode_t mode) {
	ramfs_dir_data_t *dir_data = (ramfs_dir_data_t *)dir->i_private;
	if (!dir_data || dir_data->child_count >= RAMFS_MAX_CHILDREN) return -1;
	
	inode_t *new_inode = ramfs_create_inode(S_IFREG | mode);
	if (!new_inode) return -1;

	dentry->d_inode = new_inode;
	dir_data->children[dir_data->child_count++] = dentry;
	return 0;
}


static ssize_t ramfs_read(file_t *filp, char *buf, size_t size, uint64_t *offset) {
	inode_t *inode = filp->f_dentry->d_inode;
	ramfs_file_data_t *fdata = (ramfs_file_data_t *)inode->i_private;
	if (!fdata || !fdata->buffer || *offset >= inode->i_size) return 0;

	if (*offset + size > inode->i_size) {
		size += inode->i_size - *offset;
	}

	memcpy(buf, fdata->buffer, size);
	*offset += size;
	return size;
}

static ssize_t ramfs_write(file_t *filp, const char *buf, size_t size, uint64_t *offset) {
	inode_t *inode = filp->f_dentry->d_inode;
	ramfs_file_data_t *fdata = (ramfs_file_data_t *)inode->i_private;
	if (!fdata) return -1;
	uint64_t target_size = *offset + size;

	if (target_size > fdata->capacity) {
		size_t new_cap = fdata->capacity == 0 ? 64 : fdata->capacity * 2;
		while (new_cap < target_size) new_cap *= 2;

		char *new_buffer = kmalloc(new_cap);
		if (!new_buffer) return -1;
		if (fdata->buffer) {
			memcpy(new_buffer, fdata->buffer, inode->i_size);
			kfree(fdata->buffer);
		}
		fdata->buffer = new_buffer;
		fdata->capacity = new_cap;
	}

	memcpy(fdata->buffer + *offset, buf, size);
	*offset += size;

	if (*offset > inode->i_size) {
		inode->i_size = *offset;
	}
	return size;
}



void ramfs_init() {
	inode_t *root_inode = kmalloc(sizeof(inode_t));
	memset(root_inode, 0, sizeof(inode_t));
	root_inode->i_ino = 2;
	root_inode->i_mode = S_IFDIR;
	root_inode->i_op = &ramfs_iops;
	root_inode->i_fop = &ramfs_fops;

	ramfs_dir_data_t *ddata = kmalloc(sizeof(ramfs_dir_data_t));
	memset(ddata, 0, sizeof(ramfs_dir_data_t));
	root_inode->i_private = ddata;

	root_dentry = alloc_dentry("/", root_inode, NULL);
}


static inode_ops_t ramfs_iops = {
	.lookup = ramfs_lookup,
	.mkdir = ramfs_mkdir,
	.create = ramfs_create
};

static file_ops_t ramfs_fops = {
	.read = ramfs_read,
	.write = ramfs_write,
	.open = NULL,
	.close = NULL
};