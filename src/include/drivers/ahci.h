#ifndef HVOS_DRIVERS_AHCI_H
#define HVOS_DRIVERS_AHCI_H
#include "kernel/sync.h"
#include <stdint.h>
/*Advanced Host Controller Interface*/


#define SATA_SIG_ATA			0x00000101
#define	SATA_SIG_ATAPI			0xEB140101
#define	SATA_SIG_SEMB			0xC33C0101
#define	SATA_SIG_PM				0x96690101
#define AHCI_DEV_NULL			0
#define AHCI_DEV_SATA			1
#define AHCI_DEV_SEMB			2
#define AHCI_DEV_PM				3
#define AHCI_DEV_SATAPI			4
#define HBA_PORT_IPM_ACTIVE		1
#define HBA_PORT_DET_PRESENT	3

#define HBA_PxCMD_ST			0x0001
#define HBA_PxCMD_FRE			0x0010
#define HBA_PxCMD_FR			0x4000
#define HBA_PxCMD_CR			0x8000

#define SECTOR_SIZE				512

/*FIS type*/
typedef enum {
	FIS_TYPE_REG_H2D	= 0x27,	// Register FIS - host to device
	FIS_TYPE_REG_D2H	= 0x34,	// Register FIS - device to host
	FIS_TYPE_DMA_ACT	= 0x39,	// DMA activate FIS - device to host
	FIS_TYPE_DMA_SETUP	= 0x41,	// DMA setup FIS - bidirectional
	FIS_TYPE_DATA		= 0x46,	// Data FIS - bidirectional
	FIS_TYPE_BIST		= 0x58,	// BIST activate FIS - bidirectional
	FIS_TYPE_PIO_SETUP	= 0x5F,	// PIO setup FIS - device to host
	FIS_TYPE_DEV_BITS	= 0xA1,	// Set device bits FIS - device to host
} fis_type_t;


/*Register FIS - Host to Device*/
typedef struct {
	uint8_t fis_type;		//fis_type_t -> FIS_TYPE_REG_H2D
	uint8_t pmport		:4;	//Port Multiplier
	uint8_t rsv0		:3; //Reserved
	uint8_t c			:1; //1: Command, 0: Control
	uint8_t command;		//Command Register
	uint8_t feature_lo;		//Feature Register, 7:0

	uint8_t lba0;			//LBA low register, 7:0
	uint8_t lba1;			//LBA mid register, 15:8
	uint8_t lba2;			//LBA hi register, 23:16
	uint8_t device;			//Device register
	
	uint8_t lba3;			//LBA register, 31:24
	uint8_t lba4;			//LBA register, 39:32
	uint8_t lba5;			//LBA register, 47:40
	uint8_t feature_hi;		//Feature Register, 15:8

	uint8_t countl;			//Count register, 7:0
	uint8_t counth;			//Count register, 15:8
	uint8_t icc;			//Isochronous command completion
	uint8_t control;		//Control register

	uint8_t rsv1[4];		//Reserved
} fis_reg_h2d_t;

/*Register FIS – Device to Host*/
typedef struct {
	uint8_t  fis_type;    // FIS_TYPE_REG_D2H

	uint8_t  pmport:4;    // Port multiplier
	uint8_t  rsv0:2;      // Reserved
	uint8_t  i:1;         // Interrupt bit
	uint8_t  rsv1:1;      // Reserved

	uint8_t  status;      // Status register
	uint8_t  error;       // Error register
	
	uint8_t  lba0;        // LBA low register, 7:0
	uint8_t  lba1;        // LBA mid register, 15:8
	uint8_t  lba2;        // LBA high register, 23:16
	uint8_t  device;      // Device register

	uint8_t  lba3;        // LBA register, 31:24
	uint8_t  lba4;        // LBA register, 39:32
	uint8_t  lba5;        // LBA register, 47:40
	uint8_t  rsv2;        // Reserved

	uint8_t  countl;      // Count register, 7:0
	uint8_t  counth;      // Count register, 15:8
	uint8_t  rsv3[2];     // Reserved

	uint8_t  rsv4[4];     // Reserved
} fis_reg_d2h_t;
/*Data FIS – Bidirectional*/
typedef struct {
	uint8_t  fis_type;	// FIS_TYPE_DATA
	uint8_t  pmport:4;	// Port multiplier
	uint8_t  rsv0:4;		// Reserved
	uint8_t  rsv1[2];	// Reserved
	uint32_t data[1];	// Payload
} fis_data_t;

/*PIO Setup – Device to Host*/
typedef struct {
	// DWORD 0
	uint8_t  fis_type;	// FIS_TYPE_PIO_SETUP

	uint8_t  pmport:4;	// Port multiplier
	uint8_t  rsv0:1;		// Reserved
	uint8_t  d:1;		// Data transfer direction, 1 - device to host
	uint8_t  i:1;		// Interrupt bit
	uint8_t  rsv1:1;

	uint8_t  status;		// Status register
	uint8_t  error;		// Error register

	// DWORD 1
	uint8_t  lba0;		// LBA low register, 7:0
	uint8_t  lba1;		// LBA mid register, 15:8
	uint8_t  lba2;		// LBA high register, 23:16
	uint8_t  device;		// Device register

	// DWORD 2
	uint8_t  lba3;		// LBA register, 31:24
	uint8_t  lba4;		// LBA register, 39:32
	uint8_t  lba5;		// LBA register, 47:40
	uint8_t  rsv2;		// Reserved

	// DWORD 3
	uint8_t  countl;		// Count register, 7:0
	uint8_t  counth;		// Count register, 15:8
	uint8_t  rsv3;		// Reserved
	uint8_t  e_status;	// New value of status register

	// DWORD 4
	uint16_t tc;		// Transfer count
	uint8_t  rsv4[2];	// Reserved
} fis_pio_setup_t;

/*DMA Setup – Device to Host*/
typedef struct {
	uint8_t  fis_type;	// FIS_TYPE_DMA_SETUP

	uint8_t  pmport:4;	// Port multiplier
	uint8_t  rsv0:1;		// Reserved
	uint8_t  d:1;		// Data transfer direction, 1 - device to host
	uint8_t  i:1;		// Interrupt bit
	uint8_t  a:1;            // Auto-activate. Specifies if DMA Activate FIS is needed

        uint8_t  rsved[2];       // Reserved

	//DWORD 1&2

        uint64_t DMAbufferID;    // DMA Buffer Identifier. Used to Identify DMA buffer in host memory.
                                 // SATA Spec says host specific and not in Spec. Trying AHCI spec might work.

        //DWORD 3
        uint32_t rsvd;           //More reserved

        //DWORD 4
        uint32_t DMAbufOffset;   //Byte offset into buffer. First 2 bits must be 0

        //DWORD 5
        uint32_t TransferCount;  //Number of bytes to transfer. Bit 0 must be 0

        //DWORD 6
        uint32_t resvd;          //Reserved
        
} fis_dma_setup_t;

typedef union {
	struct {
		uint32_t np:5;
		uint32_t sxs:1;
		uint32_t ems:1;
		uint32_t cccs:1;
		uint32_t ncs:5;
		uint32_t psc:1;
		uint32_t ssc:1;
		uint32_t pmd:1;
		uint32_t fbss:1;
		uint32_t spm:1;
		uint32_t sam:1;
		uint32_t _res:1;
		uint32_t iss:4;
		uint32_t sclo:1;
		uint32_t sal:1;
		uint32_t salp:1;
		uint32_t sss:1;
		uint32_t smps:1;
		uint32_t ssntf:1;
		uint32_t sncq:1;
		uint32_t s64a:1;
	}__attribute__ ((__packed__));
	uint32_t _raw;
}__attribute__ ((__packed__))  hba_cap_t;

typedef union {
	struct {
		uint32_t hr:1;
		uint32_t ie:1;
		uint32_t mrsm:1;
		uint32_t _res0:28;
		uint32_t ae:1;
	}__attribute__ ((__packed__));
	uint32_t _raw;
}__attribute__ ((__packed__))  hba_ghc_t;

typedef union {
	struct {
		uint32_t bos:1;
		uint32_t oos:1;
		uint32_t sooe:1;
		uint32_t ooc:1;
		uint32_t bb:1;
		uint32_t _res:27;
	}__attribute__ ((__packed__));
	uint32_t _raw;
}__attribute__ ((__packed__))  hba_bohc_t;

typedef struct {
    volatile uint32_t clb, clbu, fb, fbu, is, ie, cmd, rsv0, tfd, sig, ssts, sctl, serr, sact, ci, sntf, fbs;
    volatile uint32_t rsv1[11], vendor[4];
} __attribute__((__packed__)) hba_port_t;

typedef struct {
    uint32_t cap, ghc;
    volatile uint32_t is, pi;
    uint32_t vs, ccc_ctl, ccc_pts, em_loc, em_ctl, cap2, bohc;
    uint8_t  rsv[0xA0 - 0x2C];
    uint8_t  vendor[0x100 - 0xA0];
    hba_port_t ports[32];
} __attribute__ ((__packed__)) hba_mem_t;

typedef struct {
    uint8_t  cfl:5, a:1, w:1, p:1, r:1, b:1, c:1, rsv0:1, pmp:4;
    uint16_t prdtl;
    volatile uint32_t prdbc;
    uint32_t ctba, ctbau, reserved[4];
} __attribute__ ((__packed__)) hba_cmd_header_t;

typedef struct {
    uint32_t dba, dbau, rsv0;
    uint32_t dbc:22, rsv1:9, i:1;
} __attribute__ ((__packed__)) hba_prdt_entry_t;

typedef struct {
    uint8_t  cfis[64], acmd[16], rsv[48];
    hba_prdt_entry_t prdt_entry[]; // Dynamically sized
} __attribute__ ((__packed__)) hba_cmd_table_t;

typedef struct {
    hba_cmd_header_t clb[32]; // 1024 bytes
    uint8_t fb[256];          // 256 bytes
    // Explicitly allocate space for command tables to avoid under-allocation
    uint8_t cmd_tables[32 * 1024]; // 32 slots * 1KB max table size
} __attribute__ ((__packed__, aligned(4096))) ahci_port_mem_t;

void init_ahci();
bool ahci_read(hba_port_t *port, uint32_t startl, uint32_t starth, uint32_t count, uint16_t *buf);
bool ahci_write(hba_port_t *port, uint32_t startl, uint32_t starth, uint32_t count, uint16_t *buffer);
extern void storage_register_drive(hba_port_t *port);
uint64_t ahci_get_sector_count(hba_port_t *port);

#endif
