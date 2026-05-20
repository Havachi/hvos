#include "kernel/fs/ramdisk.h"

vfs_node_t ramdisk_nodes[MAX_RAMDISK_FILES];
uint32_t ramdisk_node_count = 0;



uint32_t ramdisk_read(vfs_node_t* node, uint32_t offset, uint32_t size, uint8_t *buffer) {
	if (offset >= node->size) return 0;
	if (offset + size > node->size) {
		size = node->size - offset;
	}

	memcpy(buffer, (uint8_t *)(node->data_ptr + offset), size);
	return size;
}

vfs_node_t *ramdisk_finddir(vfs_node_t *node, const char *name) {
	(void)node;
	for (uint32_t i = 0; i < ramdisk_node_count; i++) {
		if (strcmp(ramdisk_nodes[i].name, name) == 0) {
			return &ramdisk_nodes[i];
		}
	}
	return NULL;
}


