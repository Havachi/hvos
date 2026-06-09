#ifndef HVOS_VFS_H
#define HVOS_VFS_H

#include "fdtable.h"
#include "sync.h"
#include <stdint.h>
#include <sys/types.h>
#include <stddef.h>

#define VFS_FILE 0x01
#define VFS_DIR 0x02
#define MAX_RAMDISK_FILES 64


//Other may execute
#define S_IXOTH 0x1
//Other may write
#define S_IWOTH 0x2
//Other may read
#define S_IROTH 0x4

//Group may execute
#define S_IXGRP 0x8
//Group may write
#define S_IWGRP 0x10
//Group may read
#define S_IRGRP 0x20

//Owner may execute
#define S_IXUSR 0x40
//Owner may write
#define S_IWUSR 0x80
//Owner may read
#define S_IRUSR 0x100

//Sticky bit
#define S_ISVTX 0x200
//Set GID
#define S_ISGID 0x400
//Set UID
#define S_ISUID 0x800

#define S_IFIFO 0x1000
#define S_IFCHR 0x2000
#define S_IFDIR 0x4000
#define S_IFBLK 0x6000
#define S_IFREG 0x8000
#define S_IFLNK 0xA000
#define S_IFSOCK 0xC000

struct vfs_node_s;

typedef struct vfs_operations_s {
	uint32_t (*read)(struct vfs_node_s* node, uint32_t offset, uint32_t size, uint8_t* buffer);
	uint32_t (*write)(struct vfs_node_s* node, uint32_t offset, uint32_t size, uint8_t* buffer);
	struct vfs_node_s* (*readdir)(struct vfs_node_s* node, uint32_t index);
	struct vfs_node_s* (*finddir)(struct vfs_node_s* node, const char* name);
} vfs_operations_t ;

typedef struct vfs_node_s {
	char name[128];
	uint32_t flags;
	uint32_t size;
	uint32_t inode;
	uint64_t data_ptr;
	vfs_operations_t *ops;
} vfs_node_t;

typedef struct vfs_file_system_type_s {
	const char *name;
} vfs_file_system_type_t;

typedef struct vfs_super_ops_s {

} vfs_super_ops_t;


struct inode_s;
struct dentry_s;
struct file_s;

typedef struct inode_ops_s {
	struct dentry_s * (*lookup)(struct inode_s *dir, struct dentry_s *dentry);
	int (*mkdir)(struct inode_s *dir, struct dentry_s *dentry, mode_t mode);
	int (*create)(struct inode_s *dir, struct dentry_s *dentry, mode_t mode);
} inode_ops_t;

typedef struct file_ops_s {
	int (*open) (struct inode_s *inode, struct file_s *filp);
	ssize_t (*read)(struct file_s *filp, char *buf, size_t count, uint64_t *offset);
	ssize_t (*write)(struct file_s *filp, const char *buf, size_t count, uint64_t *offset);
	int (*close)(struct inode_s *inode, struct file_s *filp);
} file_ops_t;

typedef struct inode_s {
	ino_t i_ino;
	mode_t i_mode;
	size_t i_size;
	inode_ops_t *i_op;
	file_ops_t *i_fop;
	void * i_private;
} inode_t;

typedef struct dentry_s {
	char d_name[32];
	inode_t *d_inode;
	struct dentry_s *d_parent;
} dentry_t;

typedef struct file_s {
	spinlock_t f_lock;
	dentry_t *f_dentry;
	uint64_t f_pos;
	file_ops_t *f_ops;
} file_t;

extern dentry_t *root_dentry;

extern volatile struct limine_module_request module_request;



void vfs_init(void);

inode_t *alloc_inode(void);
void free_inode (inode_t *inode);
dentry_t *alloc_dentry(const char *name, inode_t *inode, dentry_t *parent);
void free_dentry (dentry_t *inode);
dentry_t *vfs_lookup(const char *path);
file_t *vfs_open(const char *path, int flags);
ssize_t vfs_read(file_t* filp, char *buffer, size_t size);
int vfs_create(const char *path, mode_t mode);
int vfs_mkdir(const char *path, mode_t mode);

extern vfs_node_t *ramdisk_finddir(vfs_node_t *node, const char *name);
extern uint32_t ramdisk_read(vfs_node_t* node, uint32_t offset, uint32_t size, uint8_t *buffer);
extern uint32_t ramdisk_node_count;
extern vfs_node_t ramdisk_nodes[MAX_RAMDISK_FILES];


#endif