#include "kernel/vfs.h"
#include "kernel/fs/tar.h"
#include "mem/mem.h"
#include <stdbool.h>
#include <sys/types.h>

vfs_node_t *vfs_root = NULL;
static vfs_node_t root_dir_node;

dentry_t *root_dentry = NULL;

static const char *next_path_token(const char *path, char *token) {
	while (*path == '/') path++;
	if (*path == '\0') return NULL;

	int i = 0;
	while (*path != '/' && *path != '\0' && i < 31) {
		token[i++] = *path++;
	}
	token[i] = '\0';
	return path;
}


inode_t *alloc_inode(void) {
	inode_t *node = (inode_t *)kmalloc(sizeof(inode_t));
	if (!node) return NULL;

	kmemset(node, 0, sizeof(inode_t));
	node->i_ino = 0;
	node->i_mode = 0;
	node->i_size = 0;
	node->i_op = NULL;
	node->i_fop = NULL;
	node->i_private = NULL;

	return node;
}

void free_inode(inode_t *inode) {
	kfree(inode);
}

dentry_t *alloc_dentry(const char *name, inode_t *inode, dentry_t *parent) {
	dentry_t *entry = (dentry_t *)kmalloc(sizeof(dentry_t));
	if (!entry) return NULL;

	if (name) {
		kstrncpy(entry->d_name, name, sizeof(entry->d_name) - 1);
		entry->d_name[sizeof(entry->d_name)-1] = '\0';
	}

	entry->d_inode = inode;
	entry->d_parent = parent;
	return entry;
}

void free_dentry(dentry_t *dentry) {
	kfree(dentry);
}

void vfs_init(void) {

	struct limine_module_response *response = module_request.response;
	if (response == NULL || response->module_count == 0) {
		kprintf("[VFS] No ramdisk found :(\n");
		return;
	}
	
	uint8_t *tar_ptr = (uint8_t *)response->modules[0]->address;
	uint64_t tar_size = response->modules[0]->size;
	uint64_t offset = 0;

	kstrcpy(root_dir_node.name, "/");
	root_dir_node.flags = VFS_DIR;
	root_dir_node.size = 0;
	root_dir_node.ops->finddir = ramdisk_finddir;
	vfs_root = &root_dir_node;

	kprintf("[VFS] Parsing ramdisk at %016lx (%d bytes)\n", tar_ptr, tar_size);
	while (offset < tar_size) {
		tar_header_t *header = (tar_header_t *)(tar_ptr + offset);
		if (header->name[0] == '\0') break;
		uint32_t file_size = octal_to_int(header->size, 11);

		if (header->typeflag == '0' || header->typeflag == '\0') {
			if (ramdisk_node_count >= MAX_RAMDISK_FILES) break;
			vfs_node_t *node = &ramdisk_nodes[ramdisk_node_count++];
			char *final_name = header->name;
			if (final_name[0] == '.' && final_name[1] == '/') {
				final_name += 2;
			}

			kstrcpy(node->name, final_name);
			node->size = file_size;
			node->flags = VFS_FILE;

			node->data_ptr = (uint64_t)(tar_ptr + offset + 512);
			node->ops->read = ramdisk_read;

			kprintf("Found file: %s (%d bytes)\n", node->name, node->size);
		}
		offset += 512 + ((file_size + 511) & ~511);
	}
	kprintf("[VFS] Filesystem setup completed cleanly\n");
}
file_t *vfs_open(const char *path, int flags) {
	dentry_t *dentry = vfs_lookup(path);
	if (!dentry || !dentry->d_inode) return NULL;

	inode_t *inode = dentry->d_inode;
	if (inode->i_mode & S_IFDIR) return NULL;

	file_t *filp = kmalloc(sizeof(file_t));
	filp->f_dentry = dentry;
	filp->f_pos = 0;
	filp->f_ops = inode->i_fop;

	if (filp->f_ops && filp->f_ops->open) {
		if (filp->f_ops->open(inode, filp) < 0) {
			kfree(filp);
			return NULL;
		}
	}
	return filp;
}

ssize_t vfs_read(file_t* filp, char *buffer, size_t size) {
	if(!filp || !filp->f_ops || !filp->f_ops->read) return -1;
	return (filp->f_ops->read(filp, buffer, size, &filp->f_pos));
}


dentry_t *vfs_lookup(const char *path) {
	if (!vfs_root || path[0] != '/') return NULL;

	dentry_t *current = root_dentry;
	char token [32];
	const char *next = path;

	while ((next = next_path_token(next, token)) != NULL) {
		if (!(current->d_inode->i_mode & S_IFDIR) || !current->d_inode->i_op->lookup) {
			return NULL;
		}

		dentry_t *search_dentry = alloc_dentry(token, NULL, current);
		dentry_t *found = current->d_inode->i_op->lookup(current->d_inode, search_dentry);

		if (!found || !found->d_inode) {
			return NULL;
		}
		current = found;
	}
	return current;
}

bool vfs_split_path(const char *path, char *out_dir, size_t dir_max, char *out_filename, size_t file_max) {
	if (!path || *path != '/' || !out_dir || !out_filename) return false;
	const char *last_slash = kstrrchr(path, '/');
	if (!last_slash) return false;

	if (path == last_slash && *(last_slash + 1) == '\0') {
		if (dir_max < 2 || file_max < 1) return false;
		kstrcpy(out_dir, "/");
		out_filename[0] = '\0';
		return true;
	}

	size_t dir_len = last_slash - path;

	if(dir_len == 0) {
		dir_len = 1;
	}

	const char *filename_start = last_slash + 1;
	size_t file_len = kstrlen(filename_start);

	if(file_len == 0) {
		return false;
	}

	if (dir_len >= dir_max || file_len >= file_max) {
		return false;
	}

	if (last_slash == path) {
		kstrcpy(out_dir, "/");
	} else {
		kstrncpy(out_dir, path, dir_len);
		out_dir[dir_len] = '\0';
	}

	kstrcpy(out_filename, filename_start);
	return true;
}

int vfs_create(const char *path, mode_t mode) {
	char parent_dir[256];
	char file_name[32];

	if (!vfs_split_path(path, parent_dir, sizeof(parent_dir), file_name, sizeof(file_name))) {
		return -1;
	}

	dentry_t *parent = vfs_lookup(parent_dir);
	if (!parent || !parent->d_inode) {
		return -2;
	}

	if (!(parent->d_inode->i_mode & S_IFDIR)) {
		return -3;
	}

	if (!parent->d_inode->i_fop || !parent->d_inode->i_op->create) {
		return -4;
	}

	dentry_t *new_file = alloc_dentry(file_name, NULL, parent);
	if (!new_file)
		return -5;

	return parent->d_inode->i_op->create(parent->d_inode, new_file, mode);
}