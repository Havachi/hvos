#ifndef HVOS_VFS_H
#define HVOS_VFS_H

#include <stdint.h>
#include <stddef.h>
#include "kernel/boot.h"
#include "klibc/printf.h"
#include "klibc/string.h"


#define VFS_FILE 0x01
#define VFS_DIR 0x02
#define MAX_RAMDISK_FILES 64

typedef struct vfs_node_s {
	char name[128];
	uint32_t flags;
	uint32_t size;
	uint32_t inode;
	uint64_t data_ptr;
	uint32_t (*read)(struct vfs_node_s* node, uint32_t offset, uint32_t size, uint8_t* buffer);
	uint32_t (*write)(struct vfs_node_s* node, uint32_t offset, uint32_t size, uint8_t* buffer);
	struct vfs_node_s* (*readdir)(struct vfs_node_s* node, uint32_t index);
	struct vfs_node_s* (*finddir)(struct vfs_node_s* node, const char* name);
} vfs_node_t;

typedef struct
{
	char name[100];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char chksum[8];
	char typeflag;
	char linkname[100];
	char magic[6];
	char version[2];
} __attribute__((packed)) tar_header_t;

extern vfs_node_t* vfs_root;
extern volatile struct limine_module_request module_request;

void vfs_init(void);
uint32_t vfs_read(vfs_node_t* node, uint32_t offset, uint32_t size, uint8_t *buffer);
vfs_node_t *vfs_finddir(vfs_node_t *node, const char* name);

extern vfs_node_t *ramdisk_finddir(vfs_node_t *node, const char *name);
extern uint32_t ramdisk_read(vfs_node_t* node, uint32_t offset, uint32_t size, uint8_t *buffer);
extern uint32_t ramdisk_node_count;
extern vfs_node_t ramdisk_nodes[MAX_RAMDISK_FILES];
#endif