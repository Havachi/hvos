#ifndef KERNEL_FS_STORAGE_H
#define KERNEL_FS_STORAGE_H

#include "kernel/sync.h"
typedef struct hba_port hba_port_t;

typedef struct {
	hba_port_t *port;
	size_t nb_sect;
	bool is_initialized;
} boot_drive_t;

bool storage_read_sectors(uint64_t lba, uint32_t count, void *buffer);
bool storage_write_sectors(uint64_t lba, uint32_t count, const void *buffer);
void storage_register_drive(hba_port_t *port);
#endif