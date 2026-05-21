#ifndef HVOS_FS_RAMDISK
#define HVOS_FS_RAMDISK
#include "kernel/vfs.h"




#define MAX_RAMDISK_FILES 64

void init_ramdisk(void);
uint32_t ramdisk_read(vfs_node_t* node, uint32_t offset, uint32_t size, uint8_t *buffer);
vfs_node_t *ramdisk_finddir(vfs_node_t *node, const char *name);
extern vfs_node_t ramdisk_nodes[MAX_RAMDISK_FILES];
extern uint32_t ramdisk_node_count;
#endif