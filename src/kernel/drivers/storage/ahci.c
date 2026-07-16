#include "drivers/ahci.h"
#include "drivers/ata.h"
#include "drivers/pci.h"
#include "kernel/fs/block_dev.h"
#include "mem/mem.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define HBA_LEN 0x10FF

pci_device_t *ahci_controller = NULL;
uint64_t ahci_mem_addr = 0x0;
hba_mem_t *ahci_mem = NULL;
uint64_t ahci_cmd_slot = 0;

extern void storage_get_sect_count();

void find_ahci_controller(){
	pci_device_t **devices = NULL;
	devices = pci_get_all_device();
	uint32_t nb_pci_devices = pci_get_nb_devices();
	
	for (uint32_t i = 0; i < nb_pci_devices; i++) {
		if (devices[i]->dev->class_code == 0x01 && devices[i]->dev->subclass == 0x06) {
			printf("[AHCI] Found controller at pci(%x:%x.%x)!\n", devices[i]->bus_id, devices[i]->dev_id, devices[i]->fn_id);
			ahci_controller = devices[i];
			ahci_mem_addr = (uint64_t)(uintptr_t)(ahci_controller->dev->gendev_header.base_addr5);
			break;
		}

	}
	if (ahci_controller == NULL) {
		printf("[AHCI] No AHCI controller found\n");
	} else {
		//I think I might need another page for this, but for now it's ok
		map_page(kernel_pml4, ahci_mem_addr+hhdm_offset, ahci_mem_addr, PTE_PRESENT | PTE_WRITABLE);
		ahci_mem_addr = ahci_mem_addr + hhdm_offset;
		ahci_mem = (hba_mem_t *)ahci_mem_addr;
		hba_mem_t *mem = (hba_mem_t *)ahci_mem_addr;
		printf("[AHCI] AHCI Version %d.%d\n", mem->vs >> 16, mem->vs & 0xFF);
	}
}

static int check_type(hba_port_t *port) {
	uint32_t ssts = port->ssts;
	uint8_t ipm = (ssts >> 8) & 0x0F;
	uint8_t det = ssts & 0x0F;
	if (det != HBA_PORT_DET_PRESENT)
		return AHCI_DEV_NULL;
	if (ipm != HBA_PORT_IPM_ACTIVE)
		return AHCI_DEV_NULL;
	switch (port->sig) {
		case SATA_SIG_ATAPI:
			return AHCI_DEV_SATAPI;
		case SATA_SIG_SEMB:
			return AHCI_DEV_SEMB;
		case SATA_SIG_PM:
			return AHCI_DEV_PM;
		default:
			return AHCI_DEV_SATA;
	}
}

static void stop_cmd(hba_port_t *port) {
	port->cmd &= ~HBA_PxCMD_ST;
	port->cmd &= ~HBA_PxCMD_FRE;
	while (1) {
		if (port->cmd & HBA_PxCMD_FR)
			continue;
		if (port->cmd & HBA_PxCMD_CR)
			continue;
		break;
	}
}

static void start_cmd(hba_port_t *port) {
	while(port->cmd & HBA_PxCMD_CR)
		;
	port->cmd |= HBA_PxCMD_FRE;
	port->cmd |= HBA_PxCMD_ST;	
}

static hba_cmd_header_t *get_port_cmdheader(hba_port_t *port, int slot) {
	uint64_t phys_base = ((uint64_t)port->clbu >> 32 | (uint64_t)port->clb);
	hba_cmd_header_t *cmd_list_base = (hba_cmd_header_t *)PHYS_TO_VIRT(phys_base);
	return &cmd_list_base[slot];
}

static hba_cmd_table_t *get_port_cmdtbl(hba_port_t *port, int slot) {
	hba_cmd_header_t *cmd_header = get_port_cmdheader(port, slot);
	uint64_t table_phys_addr = ((uint64_t)cmd_header->ctbau >> 32 | (uint64_t)cmd_header->ctba);
	return (hba_cmd_table_t *)PHYS_TO_VIRT(table_phys_addr);
}

static void ahci_bswap_buffer(uint16_t *buf, uint32_t sector_count) {
	uint32_t total_words = sector_count * (SECTOR_SIZE / 2);
	for (uint32_t i = 0; i < total_words; i++) {
		buf[i] = __builtin_bswap16(buf[i]);
	}
}

void ahci_port_rebase(hba_port_t *port, int portno) {
	stop_cmd(port);
	io_wait();
	io_wait();
	io_wait();

	uintptr_t phys_page = (uintptr_t)pmm_alloc_n(3);
	uintptr_t virt_port_addr = PHYS_TO_VIRT(phys_page);
	for (uintptr_t p = 0; p < 3; p++){
		map_page(kernel_pml4, virt_port_addr + (p * PAGE_SIZE), phys_page + (p * PAGE_SIZE), PTE_PRESENT | PTE_WRITABLE);
	}
	memset((void *)virt_port_addr, 0, 3 * 4096);


	uintptr_t clb_phys_addr = phys_page;
	uintptr_t fb_phys_addr = phys_page + 1024;
	uintptr_t cmdtbl_phys_base = phys_page + 1280;

	port->clb = (uint32_t)(clb_phys_addr & 0xFFFFFFFF);
	port->clbu = (uint32_t)((clb_phys_addr >> 32) & 0xFFFFFFFF);

	port->fb = (uint32_t)(fb_phys_addr & 0xFFFFFFFF);
	port->fbu = (uint32_t)((fb_phys_addr >> 32) & 0xFFFFFFFF);

	for (int i = 0; i < 32; i++) {
		uintptr_t cmdtbl_addr = cmdtbl_phys_base + (i * 256);
		hba_cmd_header_t *cmd_header = get_port_cmdheader(port, i);
		
		cmd_header[i].prdtl = 8;
		cmd_header[i].ctba = (uint32_t)(cmdtbl_addr & 0xFFFFFFFF);
		cmd_header[i].ctbau = (uint32_t)((cmdtbl_addr >> 32) & 0xFFFFFFFF);
	}
	start_cmd(port);
}

static int find_cmdslot(hba_port_t *port) {
	uint32_t slots = (port->sact | port->ci);
	for (int i = 0; i < ((hba_cap_t)ahci_mem->cap).ncs; i++) {
		if ((slots&1) == 0)
			return i;
		slots >>= 1;
	}
	return -1;
}

static bool ahci_io_sectors(hba_port_t *port, uint32_t startl, uint32_t starth, uint32_t count, void *buffer, bool is_write, uint8_t ata_command) {
	port->is = 0xFFFFFFFF;
	int slot = find_cmdslot(port);
	if (slot == -1)
		return false;

	hba_cmd_header_t *cmd_header = get_port_cmdheader(port, slot);
	hba_cmd_table_t *cmdtbl = get_port_cmdtbl(port, slot);

	cmd_header->cfl = sizeof(fis_reg_h2d_t)/(sizeof(uint32_t));
	cmd_header->w = is_write ? 1 : 0;
	if (is_write) {
		ahci_bswap_buffer((uint16_t *)buffer, count);
	}
	uint32_t sectors_per_prdt = 128;
	uint32_t bytes_per_prdt = sectors_per_prdt * SECTOR_SIZE;
	uint16_t prdt_entries = (uint16_t)((count + sectors_per_prdt - 1) / sectors_per_prdt);
	cmd_header->prdtl = prdt_entries;

	memset(cmdtbl, 0, sizeof(hba_cmd_table_t));
	memset(cmdtbl->prdt_entry, 0, prdt_entries * sizeof(hba_prdt_entry_t));

	uint64_t current_buf_addr = (uint64_t) VIRT_TO_PHYS(buffer);
	uint32_t sectors_left = count;

	for (int i = 0; i < prdt_entries; i++) {
		cmdtbl->prdt_entry[i].dba = (uint32_t)(current_buf_addr & 0xFFFFFFFF);
		cmdtbl->prdt_entry[i].dbau = (uint32_t)((current_buf_addr >> 32) & 0xFFFFFFFF);
		cmdtbl->prdt_entry[i].i = 0;

		if(sectors_left <= sectors_per_prdt) {
			cmdtbl->prdt_entry[i].dbc = (sectors_left * SECTOR_SIZE) - 1;
			break;
		} else {
			cmdtbl->prdt_entry[i].dbc = bytes_per_prdt - 1;
			current_buf_addr += bytes_per_prdt;
			sectors_left -= sectors_per_prdt;
		}
	}

	fis_reg_h2d_t *cmdfis = (fis_reg_h2d_t *)(&cmdtbl->cfis);
	cmdfis->fis_type = FIS_TYPE_REG_H2D;
	cmdfis->c = 1;
	cmdfis->command = ata_command;


	if (ata_command == ATA_CMD_IDENTIFY) {
		cmdfis->device = 0;
		cmdfis->lba0 = 0;
		cmdfis->lba1 = 0;
		cmdfis->lba2 = 0;
		cmdfis->lba3 = 0;
		cmdfis->lba4 = 0;
		cmdfis->lba5 = 0;
		cmdfis->countl = 0;
		cmdfis->counth = 0;
	} else{
		cmdfis->device = 1 << 6;
		cmdfis->lba0 = (uint8_t)startl;
		cmdfis->lba1 = (uint8_t)(startl >> 8);
		cmdfis->lba2 = (uint8_t)(startl >> 16);
		cmdfis->lba3 = (uint8_t)(startl >> 24);
		cmdfis->lba4 = (uint8_t)(starth);
		cmdfis->lba5 = (uint8_t)(starth >> 8);
	}

	cmdfis->countl = count & 0xFF;
	cmdfis->counth = (count >> 8) & 0xFF;

	int spin = 0;
	while ((port->tfd & (0x80 | 0x08)) && spin < 1000000) {
		spin++;
	}
	if (spin == 1000000){
		printf("[AHCI] Disk hung");
		return false;
	}

	port->ci = 1 << slot;

	while(1) {
		if ((port->ci & (1 << slot)) == 0)
			break;
		if (port->is & (1 << 30)) {
			printf("[AHCI] Read Disk Error\n");
			return false;
		}
	}

	if (port->is & (1 << 30)) {
			printf("[AHCI] Read Disk Error\n");
			return false;
	}

	if (!is_write) {
		ahci_bswap_buffer((uint16_t *) buffer, count);
	}

	return true;
}

bool ahci_read(hba_port_t *port, uint32_t startl, uint32_t starth, uint32_t count, uint16_t *buffer){
	return ahci_io_sectors(port, startl, starth, count, buffer, false, ATA_CMD_READ_DMA_EXT);
}

bool ahci_write(hba_port_t *port, uint32_t startl, uint32_t starth, uint32_t count, uint16_t *buffer) {
	return ahci_io_sectors(port, startl, starth, count, buffer, true, ATA_CMD_WRITE_DMA_EXT);
}

void ahci_probe_port() {
	hba_mem_t *mem = (hba_mem_t *)ahci_mem_addr;
	uint32_t pi = mem->pi;
	int i = 0;

	while(i < 32) {
		if (pi & 1) {
			int dt = check_type(&mem->ports[i]);
		
			switch (dt) {
				case AHCI_DEV_SATA:
					storage_register_drive(&mem->ports[i]);
					printf("[AHCI] SATA drive found at port %d\n", i);
					break;
				case AHCI_DEV_SATAPI:
					printf("[AHCI] SATAPI drive found at port %d\n", i);
					break;
				case AHCI_DEV_SEMB:
					printf("[AHCI] SEMB drive found at port %d\n", i);
					break;
				case AHCI_DEV_PM:
					printf("[AHCI] PM drive found at port %d\n", i);
					break;
				default:
					//printf("No drive found at port %d\n", i);
					break;
			}
		}
		pi >>= 1;
		i++;
	}
}

void init_ahci(){
	find_ahci_controller();
	ahci_probe_port();
	ahci_mem = (hba_mem_t *)ahci_mem_addr;
	ahci_port_rebase(&ahci_mem->ports[0], 0);
	ahci_port_rebase(&ahci_mem->ports[1], 1);
	storage_get_sect_count();
}

int ahci_read_blocks(block_device_t *dev, uint64_t lba, uint32_t count, void* buffer){
	hba_port_t *port = (hba_port_t *)dev->priv_data;
	return ahci_read(port, ((uint32_t)(lba & 0xFFFFFFFF)), ((uint32_t)((lba >> 32) & 0xFFFFFFFF)), count, (uint16_t *)buffer);
}

int ahci_write_blocks(block_device_t *dev, uint64_t lba, uint32_t count, const void* buffer){
	hba_port_t *port = (hba_port_t *)dev->priv_data;
	return ahci_write(port, ((uint32_t)(lba & 0xFFFFFFFF)), ((uint32_t)((lba >> 32) & 0xFFFFFFFF)), count, (uint16_t *)buffer);
}

uint64_t ahci_get_sector_count(hba_port_t *port) {
	uint64_t buf_phys = (uint64_t)pmm_alloc();
	map_page(kernel_pml4, PHYS_TO_VIRT(buf_phys), buf_phys, PTE_PRESENT | PTE_WRITABLE);

	uint16_t *identify_buf = (uint16_t *)(PHYS_TO_VIRT(buf_phys));
	if (!identify_buf) {
		return 0;
	}
	bool success = ahci_io_sectors(port, 0, 0, 1, (void *)PHYS_TO_VIRT(buf_phys), false, ATA_CMD_IDENTIFY);
	if (!success) {
		printf("[AHCI] Failed to IDENTIFY disk\n");
		kfree(identify_buf);
		return 0;
	}

	uint64_t total_sectors = 0;
	if (identify_buf[83] & (1 << 10)){
		total_sectors = ((uint64_t)identify_buf[103] << 48) | 
						((uint64_t)identify_buf[102] << 32) |
						((uint64_t)identify_buf[101] << 16) |
						((uint64_t)identify_buf[100]);
	} else {
		total_sectors =	((uint64_t)identify_buf[61] << 16) |
						((uint64_t)identify_buf[60]);
	}
	pmm_free((void *)buf_phys);	
	return total_sectors;
}