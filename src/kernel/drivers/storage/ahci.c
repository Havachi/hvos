#include "drivers/ahci.h"
#include "drivers/ata.h"
#include "drivers/pci.h"
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

void ahci_identify(){

}

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

static hba_cmd_header_t *get_port_cmdheader(hba_port_t *port) {
	return (hba_cmd_header_t *)PHYS_TO_VIRT((uint64_t)(((uint64_t)port->clbu << 32) | ((uint64_t)port->clb)));
}

static hba_cmd_table_t *get_port_cmdtbl(hba_port_t *port) {
	hba_cmd_header_t *cmd_header = get_port_cmdheader(port);
	return (hba_cmd_table_t *)PHYS_TO_VIRT((uint64_t)(((uint64_t)cmd_header->ctbau << 32) | ((uint64_t) cmd_header->ctba)));;
}

void ahci_port_rebase(hba_port_t *port, int portno) {
	stop_cmd(port);
	io_wait();
	io_wait();
	io_wait();
	uintptr_t phys_page = (uintptr_t)pmm_alloc();
	uintptr_t virt_port_addr = ((uintptr_t)phys_page) + hhdm_offset;
	map_page(kernel_pml4, virt_port_addr, phys_page, PTE_PRESENT | PTE_WRITABLE);
	ahci_port_mem_t* port_mem = (ahci_port_mem_t *)virt_port_addr;

	memset(port_mem, 0, sizeof(ahci_port_mem_t));
	uintptr_t clb_phys_addr = phys_page;
	uintptr_t fb_phys_addr = phys_page + 1024;

	port->clb = clb_phys_addr & 0xFFFFFFFF;
	port->clbu = (clb_phys_addr >> 32) & 0xFFFFFFFF;
	port->fb = fb_phys_addr & 0xFFFFFFFF;
	port->fbu = (fb_phys_addr >> 32) & 0xFFFFFFFF;
	hba_cmd_header_t *cmd_header = get_port_cmdheader(port);
	for (int i = 0; i < 32; i++) {
		uintptr_t addr = phys_page + (40<<10) + (portno << 13) + (i << 8);
		cmd_header[i].prdtl = 8;
		cmd_header[i].ctba = addr & 0xFFFFFFFF;
		cmd_header[i].ctbau = (addr >> 32) & 0xFFFFFFFF;
	}
	start_cmd(port);
}

static int find_cmdslot(hba_port_t *port) {
	uint32_t slots = (port->sact | port->ci);
	for (int i = 0; i < ahci_mem->cap.ncs; i++) {
		if ((slots&1) == 0)
			return i;
		slots >>= 1;
	}
	return -1;
}

bool ahci_read(hba_port_t *port, uint32_t startl, uint32_t starth, uint32_t count, uint16_t *buf){
	port->is = (uint32_t) -1;
	int spin = 0;
	int slot = find_cmdslot(port);
	if (slot == -1)
		return false;
	hba_cmd_header_t *cmd_header = get_port_cmdheader(port);
	
	cmd_header += slot;
	cmd_header->cfl = sizeof(fis_reg_h2d_t)/(sizeof(uint32_t));
	cmd_header->w = 0;
	cmd_header->prdtl = (uint16_t)((count-1) >> 4) + 1;

	hba_cmd_table_t *cmdtbl = get_port_cmdtbl(port);
	memset(cmdtbl, 0, sizeof(hba_cmd_table_t) +
		(cmd_header->prdtl-1) * sizeof(hba_prdt_entry_t));
	
	for(int i = 0; i < cmd_header->prdtl-1; i++) {
		cmdtbl->prdt_entry[i].dba = (uint32_t) buf;
		cmdtbl->prdt_entry[i].dbc = 8*1024-1;
		cmdtbl->prdt_entry[i].i = 1;
		buf += 4*1024;
		count -= 16;
	}
	cmdtbl->prdt_entry[cmd_header->prdtl-1].dba = (uint32_t) buf;
	cmdtbl->prdt_entry[cmd_header->prdtl-1].dbc = (count<<9)-1;
	cmdtbl->prdt_entry[cmd_header->prdtl-1].i = 1;

	fis_reg_h2d_t *cmdfis = (fis_reg_h2d_t *)(&cmdtbl->cfis);
	cmdfis->fis_type = FIS_TYPE_REG_H2D;
	cmdfis->c = 1;
	cmdfis->command = ATA_CMD_READ_DMA_EXT;
	cmdfis->lba0 = (uint8_t)startl;
	cmdfis->lba1 = (uint8_t)(startl>>8);
	cmdfis->lba2 = (uint8_t)(startl>>16);
	cmdfis->device = 1<<6;

	cmdfis->lba3 = (uint8_t)(startl >> 24);
	cmdfis->lba4 = (uint8_t)(starth);
	cmdfis->lba5 = (uint8_t)(starth>>8);
	cmdfis->countl = count & 0xFF;
	cmdfis->counth = (count >> 8) & 0xFF;

	while ((port->tfd & (0x80 | 0x08)) && spin < 1000000) {
		spin++;
	}
	if (spin == 1000000) {
		printf("[AHCI] Port is hung\n");
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
	return true;
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
	uint8_t *buf = kzalloc(1024);
}