#include "kernel/fs/storage.h"
#include <stdint.h>

boot_drive_t g_boot_drive = {0};

extern bool ahci_read(hba_port_t *port, uint32_t startl, uint32_t starth, uint32_t count, uint16_t *buf);
extern bool ahci_write(hba_port_t *port, uint32_t startl, uint32_t starth, uint32_t count, uint16_t *buf);
extern uint64_t ahci_get_sector_count(hba_port_t *port);

bool storage_read_sectors(uint64_t lba, uint32_t count, void *buffer) {
	if (!g_boot_drive.is_initialized) {
		return false;
	}

	uint32_t startl = (uint32_t)(lba & 0xFFFFFFFF);
	uint32_t starth = (uint32_t)((lba >> 32) & 0xFFFFFFFF);

	return ahci_read(g_boot_drive.port, startl, starth, count, (uint16_t *)buffer);
}

bool storage_write_sectors(uint64_t lba, uint32_t count, const void *buffer) {
	if (!g_boot_drive.is_initialized) {
		return false;
	}
	uint32_t startl = (uint32_t)(lba & 0xFFFFFFFF);
	uint32_t starth = (uint32_t)((lba >> 32) & 0xFFFFFFFF);
	return ahci_write(g_boot_drive.port, startl, starth, count, (uint16_t *)buffer);
}

void storage_register_drive(hba_port_t *port){
	g_boot_drive.port = port;
	g_boot_drive.is_initialized = true;
}

void storage_get_sect_count() {
	if (g_boot_drive.is_initialized)
		g_boot_drive.nb_sect = ahci_get_sector_count(g_boot_drive.port);
}