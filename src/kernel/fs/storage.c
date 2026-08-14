#include "kernel/fs/storage.h"
#include "data_structure/list.h"
#include "kernel/fs/block_dev.h"
#include "mem/mem.h"
#include <stdint.h>
#include <stdio.h>

static char next_drv_letter = 'a';

extern list_t *blkdevs_list;
extern block_device_t *selected_blkdev;
extern int ahci_read_blocks(block_device_t *dev, uint64_t lba, uint32_t count, void* buffer);
extern int ahci_write_blocks(block_device_t *dev, uint64_t lba, uint32_t count, const void* buffer);
extern int ahci_get_drv_stat(block_device_t *dev, void *buffer);
extern uint64_t ahci_get_sector_count(hba_port_t *port);

void storage_register_ahci_drive(hba_port_t *port){
	char *name = kzalloc(4);
	sprintf(name, "sd%c\0", next_drv_letter++);

	blkdev_ops_t *ops = kzalloc(sizeof(blkdev_ops_t));
	ops->read_blocks = ahci_read_blocks;
	ops->write_blocks = ahci_write_blocks;
	ops->stat = ahci_get_drv_stat;
	
	block_device_t *blkdev = new_block_dev(name, ops);
	//blkdev->block_size = ahci_get_sector_count(port);
	blkdev->total_bg = 0;
	blkdev->priv_data = port;

	if (!selected_blkdev){
		selected_blkdev = blkdev;
	}
}

