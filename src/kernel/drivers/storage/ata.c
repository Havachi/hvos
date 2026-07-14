#include "drivers/ata.h"
#include "cpu/io.h"
#include "mem/mem.h"
#include <stdint.h>
#include <stdio.h>

ata_drive_t *drive_list = NULL;
uint32_t selected_bus = ATA_BUS_PRIMARY_IO;
uint32_t selected_drive = ATA_DISK_MASTER;

static void ata_wait(uint16_t bus){
	io_read_8(bus + 7);
	io_read_8(bus + 7);
	io_read_8(bus + 7);
	io_read_8(bus + 7);
}

static void select_drive(uint8_t d) {
	if (d != ATA_DISK_MASTER && d != ATA_DISK_SLAVE)
		return;
	io_write_8(ATA_DRV_SEL_REG(selected_bus), d);
	selected_drive = d;
}

static void select_bus(uint16_t b) {
	if (b != ATA_BUS_PRIMARY_IO && b != ATA_BUS_SECONDARY_IO)
		return;
	selected_bus = b;
}

ata_status_reg_t get_drive_status(uint16_t b, uint8_t d) {
	if (io_read_8(ATA_DRV_SEL_REG(b)) != d) {
		io_write_8(ATA_DRV_SEL_REG(b), d);
	}
	uint8_t raw = io_read_8(ATA_STATUS_REG(b));
	return (ata_status_reg_t)raw;
}

void poll_drive(uint16_t b, uint8_t d) {
	ata_status_reg_t status = get_drive_status(b,d);
	while (status.bsy != 0) {
		status = get_drive_status(b,d);
	}
	return;
}

int32_t send_cmd(uint32_t cmd, uint16_t b, uint8_t d){
	ata_status_reg_t status = {0};
	io_write_8(ATA_COMMAND_REG(selected_bus),cmd);
	status = (ata_status_reg_t)io_read_8(ATA_STATUS_REG(selected_bus));
	if (status._raw == 0) {
		return -1;
	}
	poll_drive(b,d);
	return 0;
}

char *read_master(size_t nbw, char *buf) {

	if (buf == NULL || nbw == 0){
		return NULL;
	}

	uint16_t *wbuf = (uint16_t *)buf;

	for (uint16_t i = 0; i < nbw; i++) {
		wbuf[i] = io_read_16(ATA_DATA_REG(selected_bus));
	}
	return buf;
}

static void ata_fix_string(char *str, int len) {
	for (int i = 0; i < len; i += 2){
		char tmp = str[i];
		str[i] = str[i+1];
		str[i+1] = tmp;
	}
}

void print_drive_info(char *drvi) {
		char model[41];
		char serial[21];
		printf("\n\n====================\n");
		printf("Valid ATA drive Found!\n");
		ata_identify_device_data_t *drv_ident = (ata_identify_device_data_t *)drvi;

		for (int i = 0; i < 40; i++){
			model[i] = drvi[54+i];
		}
		model[40] = '\0';
		ata_fix_string(model, 40);

		printf("Model: %s\n", model);

		for (int i = 0; i < 20; i++){
			serial[i] = drvi[20+i];
		}
		serial[20] = '\0';

		ata_fix_string(serial, 20);
		printf("Serial Number: %s\n", serial);
		if (drv_ident->capabilities.lba_supported) {
			printf("LBA ");
		}
		if (drv_ident->capabilities.dma_supported) {
			printf("DMA ");
		}
		if (drv_ident->capabilities.iordy_supported) {
			printf("IORDY ");
		}
		if (drv_ident->command_set_support.smart_commands) {
			printf("SMART ");
		}
		printf("\n");
		if (drv_ident->nominal_media_rotation_rate == 1) {
			printf("SSD");
		} else {
			printf("HDD@%d", drv_ident->nominal_media_rotation_rate);
		}
		printf("\n");
		printf("====================\n\n");
}


int32_t identify(uint16_t b, uint8_t d) {
	ata_status_reg_t status = {0};
	io_write_8(ATA_DRV_SEL_REG(b), d);
	ata_wait(b);

	io_write_8(ATA_SECT_CNT_REG(b), 0);
	io_write_8(ATA_LBA_LO(b), 0);
	io_write_8(ATA_LBA_MID(b), 0);
	io_write_8(ATA_LBA_HI(b), 0);
	ata_wait(b);
	uint8_t lba_mid = io_read_8(ATA_LBA_MID(b));
	uint8_t lba_hi = io_read_8(ATA_LBA_HI(b));

	int32_t res = send_cmd(ATA_CMD_IDENTIFY, b, d);
	if (res == -1) {
		return ATA_ERR_IDNF;
	}
	ata_wait(b);

	status = get_drive_status(b, d);
	if (status._raw == 0x00 || status._raw == 0xFF) {
		return ATA_ERR_IDNF;
	}

	uint32_t timeout = 100000;
	while (timeout--) {
		status = get_drive_status(b, d);
		if (!(status.bsy) && (status.drq || status.err)) {
			break;
		}
		ata_wait(b);
	}
	if (timeout == 0) {
		return -2;
	}

	if (status.drq && !status.err) {
		uint16_t buffer[256];
		read_master(256, (char*)buffer);
		print_drive_info((char *)buffer);
		return 0;
	}

	if (status.err) {


		if (lba_mid == 0x14 && lba_hi == 0xEB){
			(void)io_read_8(b+7);
			return -3;
		}
	}

	return ATA_ERR_ABRT;
}

void list_drives(){
	int32_t dsks_res[4] = {0}; 

	dsks_res[0] = identify(ATA_BUS_PRIMARY_IO, ATA_DISK_MASTER);
	dsks_res[1] = identify(ATA_BUS_PRIMARY_IO, ATA_DISK_SLAVE);
	dsks_res[2] = identify(ATA_BUS_SECONDARY_IO, ATA_DISK_MASTER);
	dsks_res[3] = identify(ATA_BUS_SECONDARY_IO, ATA_DISK_SLAVE);

	for (int i = 0; i < 4; i++) {
		if (dsks_res[i] == ATA_ERR_IDNF){
			printf("[%d] Not present\n",i);
		} else if (dsks_res[i] == ATA_ERR_UNC){
			printf("[%d] Present, not ATA\n", i);
		} else if (dsks_res[i] == 0) {
			printf("[%d] Present, ATA\n", i);
		} else if (dsks_res[i] == -2) {
			printf("[%d] Timeout\n", i);
		} else if (dsks_res[i] == -3) {
			printf("[%d] Present, ATAPI\n", i);
		}   else {
			printf("[%d] %d\n", i,  dsks_res[i]);
		}
	}
}

void init_ata() {
	//drive_list = kzalloc(sizeof(ata_drive_t) * 4);

	list_drives();
}



