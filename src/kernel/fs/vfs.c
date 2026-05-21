#include "kernel/vfs.h"

vfs_node_t *vfs_root = NULL;
static vfs_node_t root_dir_node;

static uint32_t octal_to_int(const char *str, int size) {
	uint32_t n = 0;
	for (int i = 0; i < size; i++){
		if (str[i] == '\0' || str[i] == ' ' || str[i] < '0' || str[i] > '7') {
            break;
        }
		n = n * 8 + (str[i]-'0');
	}
	return n;
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
	root_dir_node.finddir = ramdisk_finddir;
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
			node->read = ramdisk_read;

			kprintf("Found file: %s (%d bytes)\n", node->name, node->size);
		}
		offset += 512 + ((file_size + 511) & ~511);
	}
	kprintf("[VFS] Filesystem setup completed cleanly\n");
}

uint32_t vfs_read(vfs_node_t* node, uint32_t offset, uint32_t size, uint8_t *buffer) {
	if(node == NULL || node->read == NULL)
		return 0;
	return node->read(node, offset, size, buffer);
}

vfs_node_t *vfs_finddir(vfs_node_t *node, const char* name) {
	if(node == NULL || node->finddir == NULL)
		return 0;
	return node->finddir(node, name);
}