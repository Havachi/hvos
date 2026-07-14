#ifndef HVOS_DRIVERS_PCI_H
#define HVOS_DRIVERS_PCI_H

#include "cpu/io.h"
#include <stdint.h>

#define PCI_CONFIG_REG	0xCF8
#define PCI_DATA_REG	0xCFC
#define PCI_DEV_LIST_BASE_SIZE 4

typedef union {
	struct {
		//I/O Space
		uint8_t is:1;
		//Memory Space
		uint8_t ms:1;
		//Bus Master
		uint8_t bm:1;
		//Special Cycles
		uint8_t sc:1;
		//Memory Write and Invalidate Enable
		uint8_t mwie:1;
		//VGA Palette Snoop
		uint8_t vps:1;
		//Parity Error Response
		uint8_t per:1;
		//Reserved
		uint8_t _res0:1;
		//SERR# Enable
		uint8_t se:1;
		//Fast Back-to-Back Enable
		uint8_t fbtbe:1;
		//Interrupt Disable
		uint8_t intd:1;
		//Reserved
		uint8_t _res1:5;
	}__attribute__ ((__packed__));
	uint16_t _raw;
} __attribute__ ((__packed__)) pci_cmd_reg_t;

typedef union {
	struct {
		uint8_t res0:3;
		//Interrupt status
		uint8_t intr_status:1;
		//Capabilities list
		uint8_t cp_list:1;
		//66 Mhz Capable
		uint8_t __66mhz_cp:1;
		uint8_t res1:1;
		//Fast back-to-back capable
		uint8_t fbtbc:1;
		//Master Data Parity Error
		uint8_t mdpe:1;
		//DEVSEL Timing
		uint8_t DEVSELt:2;
		//Signaled Target Abort
		uint8_t sta:1;
		//Received Target Abort
		uint8_t rta:1;
		//Received Master Abort
		uint8_t rma:1;
		//Signaled System Error
		uint8_t sse:1;
		//Detected Parity Error
		uint8_t dpe:1;
	}__attribute__ ((__packed__));
	uint16_t _raw;
} __attribute__ ((__packed__)) pci_status_reg_t;


//Header 0x1 - PCI-to-PCI Bridge
typedef struct {
	uint32_t base_addr0;
	uint32_t base_addr1;
	uint32_t base_addr2;
	uint32_t base_addr3;
	uint32_t base_addr4;
	uint32_t base_addr5;
	uint32_t cardbus_cis_ptr;
	uint16_t subsys_vendor_id;
	uint16_t subsys_id;
	uint32_t exp_rom_base_addr;
	uint8_t	cp_pointer;
	uint32_t _res0:24;
	uint32_t _res1;
	uint8_t intr_line;
	uint8_t intr_pin;
	uint8_t min_grant;
	uint8_t max_latency;
} __attribute__ ((__packed__)) gendev_header_t;

//Header Type 0x1 - PCI-to-PCI Bridge
typedef struct {
	uint32_t base_addr0;
	uint32_t base_addr1;
	//Primary Bus Number
	uint8_t pbn;
	//Secondary Bus Number
	uint8_t sbn;
	//Subordinate Bus Number
	uint8_t sub_bn;
	//Secondary Latency Timer
	uint8_t slt;
	uint8_t io_base;
	uint8_t io_lim;
	//Secondary Status
	uint16_t ss;
	uint16_t mem_base;
	uint16_t mem_limit;
	uint16_t pre_mem_base;
	uint16_t pre_mem_limit;
	//Prefetchable Base Upper 32 Bits
	uint32_t pre_base_hi;
	//Prefetchable Limit Upper 32 Bits
	uint32_t pre_limit_hi;
	//I/O Base Upper 16 Bits
	uint16_t io_base_hi;
	//I/O Limit Upper 16 Bits
	uint16_t io_limit_hi;
	//Capability Pointer
	uint8_t cp_ptr;
	uint32_t _res0:24;
	//Expansion ROM base address
	uint32_t exp_rom_base_addr;
	uint8_t intr_line;
	uint8_t intr_pin;
	//Bridge Control
	uint16_t br_ctrl;
} __attribute__ ((__packed__)) p2pbr_header_t;

//Header Type 0x2 (PCI-to-CardBus bridge)
typedef struct {
	uint8_t _raw[48];
} __attribute__ ((__packed__)) p2cbr_header_t;

typedef union {
	//detailed PCI struct
	struct{
		//PCI common header fields
		uint16_t vendor_id;
		uint16_t device_id;
		uint16_t command;
		uint16_t status;
		uint8_t rev_id;
		uint8_t prog_if;
		uint8_t subclass;
		uint8_t class_code;
		uint8_t cache_line_size;
		uint8_t latency_timer;
		uint8_t header_type;
		uint8_t BIST;
		union {
			//Header 0x0 - General Device
			gendev_header_t gendev_header;
			//Header Type 0x1 - PCI-to-PCI Bridge
			p2pbr_header_t p2pbr_header;
			//Header Type 0x2 (PCI-to-CardBus bridge)
			p2cbr_header_t p2cbr_header;
		};

	}__attribute__ ((__packed__, aligned(256)));

	uint8_t buffer[256];
} __attribute__ ((__packed__)) pci_dev_t;

typedef struct {
	uint32_t bus_id;
	uint32_t dev_id;
	uint32_t fn_id;
	pci_dev_t *dev;
} pci_device_t;

//Helper struct for Base Address Registers
typedef union {
	//Memory Space BAR Layout
	struct {
		uint32_t b0:1;
		uint32_t type:2;
		uint32_t prefetchable:1;
		uint32_t mem_addr:28;
	}__attribute__ ((__packed__));

	//IO Space BAR Layout
	struct {
		uint32_t _b0:1;
		uint32_t reserved:1;
		uint32_t io_addr:30;
	}__attribute__ ((__packed__));
	//always0
	uint32_t _raw;
} __attribute__ ((__packed__)) pci_bar_t;


typedef union {
	struct {
		uint8_t reg_offset;
		uint8_t func_nb:3;
		uint8_t dev_nb:5;
		uint8_t bus_nb;
		uint8_t _reserved:7;
		uint8_t enable:1;		
	} __attribute__ ((__packed__)) ;
	uint32_t _raw;
} __attribute__ ((__packed__)) pci_config_addr_t;


enum {
	PCI_HEADER_TYPE_STD = 0x0,
	PCI_HEADER_TYPE_P2P = 0x1,
	PCI_HEADER_TYPE_CBB = 0x2,
};


static inline uint32_t read_pci(uint32_t bus, uint32_t dev, uint32_t func, uint32_t offset) {
	io_write_32(PCI_CONFIG_REG,
		(0x80000000 | (bus << 16) | (dev<<11) | (func<<8) | (offset & 0xFC))
	);
	return io_read_32(PCI_DATA_REG);
}

#define read_pci_vendorid(bus, dev, func)	(read_pci(bus, dev, func, 0x0) & 0xFFFF)
#define read_pci_deviceid(bus, dev, func)	(read_pci(bus, dev, func, 0x0) >> 16)
#define read_pci_class(bus, dev, func)		(read_pci(bus, dev, func, 0x8) >> 24)
#define read_pci_subclass(bus, dev, func)	((read_pci(bus, dev, func, 0x8) >> 16) & 0xFF)
#define read_pci_progif(bus, dev, func)		((read_pci(bus, dev, func, 0x8) >> 8) & 0xFF)

void init_pci();
void print_all_pci();
pci_device_t **pci_get_all_device();
uint32_t pci_get_nb_devices();
#endif