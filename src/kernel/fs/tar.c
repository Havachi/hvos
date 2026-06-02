#include "kernel/fs/tar.h"
#include "kernel/fs/ramfs.h"
#include "kernel/vfs.h"
#include "mem/mem.h"

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

void ramfs_from_tar(uint8_t *tar_addr, size_t tar_size) {
	size_t offset = 0;

	while(offset + 512 <= tar_size) {
		tar_header_t *header = (tar_header_t*) (tar_addr + offset);
		if (header->name[0] == '\0') {
			break;
		}

		if (kstrncmp(header->magic, "ustar", 5) != 0) {
			break;
		}
		uint64_t file_size = octal_to_int(header->size, 12);

		char absolute_path[256];
		absolute_path[0] = '/';
		kstrncpy(absolute_path+1, header->name, sizeof(absolute_path) - 2);
		absolute_path[sizeof(absolute_path) - 1] = '\0';

		if (header->typeflag == TAR_TYPE_DIR) {
			size_t len = kstrlen(absolute_path);
			if (len > 1 && absolute_path[len - 1] == '/') {
				absolute_path[len-1] = '\0';
			}
			vfs_mkdir(absolute_path, 0775);

		} else if (header->typeflag == TAR_TYPE_NORMAL || header->typeflag == '\0') {
			vfs_create(absolute_path, 0644);
			file_t *file = vfs_open(absolute_path, 0);
			if (file) {
				const char *file_data_ptr = (const char *)(tar_addr + offset + 512);
				uint64_t write_offset = 0;
				file->f_ops->write(file, file_data_ptr, file_size, &write_offset);
				kfree(file);
			}

		}

		size_t aligned_data_size = (file_size + 511) & ~511;
		offset += 512 + aligned_data_size;
	} 
}
