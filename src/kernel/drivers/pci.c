#include "drivers/pci.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "cpu/io.h"
#include "drivers/pci-devices.h"
#include "mem/mem.h"

uint32_t nb_dev = 0;
pci_device_t **devices = NULL;

pci_dev_t* read_full_dev(uint8_t bus, uint8_t dev, uint8_t fn) {
	pci_dev_t *pci_dev = kzalloc(sizeof(pci_dev_t));
	for(int i = 0; i < 64; i++) {

		uint32_t addr = (0x80000000 | (bus << 16) | (dev<<11) | (fn<<8) | (i*4));
		io_write_32(PCI_CONFIG_REG, addr);
		*(uint32_t*)&pci_dev->buffer[i*4] = io_read_32(PCI_DATA_REG);
	}
	return pci_dev;
}

int32_t check_dev(uint8_t bus, uint8_t dev) {
	uint16_t vend = read_pci_vendorid(bus, dev, 0);
	if (vend == 0xFFFF)
		return -1;
	uint16_t devid = read_pci_deviceid(bus, dev, 0);
	if (devid == 0xFFFF)
		return -1;
	return 1;
}

void bf_check_all_buses(void) {
	uint16_t bus;
	uint8_t dev;
	uint8_t fn = 0;

	for(bus = 0; bus < 256; bus++) {
		for (dev = 0; dev < 32; dev++) {
			if(check_dev(bus, dev) == 1) {
				devices = krealloc(devices, nb_dev+1, sizeof(pci_device_t), (sizeof(pci_device_t) * nb_dev));
				devices[nb_dev] = kzalloc(sizeof(pci_device_t));
				devices[nb_dev]->dev = read_full_dev(bus, dev,fn);
				devices[nb_dev]->bus_id = bus;
				devices[nb_dev]->dev_id = dev;
				devices[nb_dev]->fn_id = fn;
				nb_dev++;
			}
		}
	}
}

static const char *get_pci_class_name(uint8_t class_id) {
	for (size_t i = 0; i < NB_VALID_PCI_CLASSES; i++) {
		if (pci_classes[i].code == class_id){
			return pci_classes[i].name;
		}
	}
	return "Unknown";
}

static const char *get_pci_subclass_name(uint8_t class_id, uint8_t subclass_id) {

	for (size_t i = 0; i < NB_VALID_PCI_CLASSES; i++) {
		if (pci_classes[i].code == class_id) {
			for (size_t j = 0; j < pci_classes[i].subclasses_count; j++) {
				if (pci_classes[i].subclasses[j].code == subclass_id){
					return pci_classes[i].subclasses[j].name;
				}
			}
		}
	}

	return NULL;
}

pci_device_t **pci_get_all_device() {
	return devices;
}

uint32_t pci_get_nb_devices() {
	return nb_dev;
}

void print_all_pci() {
	for (uint32_t i = 0; i < nb_dev; i++) {
		pci_dev_t *dev = devices[i]->dev;
		printf("[PCI] %x:%x.%x ", devices[i]->bus_id, devices[i]->dev_id, devices[i]->fn_id);

		printf("[%s] ", get_pci_class_name(dev->class_code));
		if (pci_classes[dev->class_code].subclasses_count >= 0) {
			printf("%s", get_pci_subclass_name(dev->class_code, dev->subclass));
		}
		printf("\n");
	}
}


void init_pci() {
	nb_dev = 0;
	devices = kcalloc(sizeof(pci_dev_t), 0);
	bf_check_all_buses();
}