#ifndef HVOS_DRIVERS_PCI_DEVICES_H
#define HVOS_DRIVERS_PCI_DEVICES_H

#include <stdint.h>
#include <stdlib.h>

#define NB_VALID_PCI_CLASSES 24

struct pci_sub_class {
	uint8_t code;
	const char* name;
};

struct pci_class {
	uint8_t code;
	const char* name;
	const struct pci_sub_class* subclasses;
	size_t subclasses_count;
};

const struct pci_sub_class unclassified_subclasses[] = {
	{0x0, "Non-VGA-Compatible Unclassified Device"},
	{0x1, "VGA-Compatible Unclassified Device"},
};

const struct pci_sub_class mass_storage_controller_subclasses[] = {
	{0x0, "SCSI Bus Controller"},
	{0x1, "IDE Controller"},
	{0x2, "Floppy Disk Controller"},
	{0x3, "IPI Bus Controller"},
	{0x4, "RAID Controller"},
	{0x5, "ATA Controller"},
	{0x6, "Serial ATA Controller"},
	{0x7, "Serial Attached SCSI Controller"},
	{0x8, "Non-Volatile Memory Controller"},
	{0x80, "Other"},
};

const struct pci_sub_class network_controller_subclasses[] = {
	{0x0, "Ethernet Controller"},
	{0x1, "Token Ring Controller"},
	{0x2, "FDDI Controller"},
	{0x3, "ATM Controller"},
	{0x4, "ISDN Controller"},
	{0x5, "WorldFip Controller"},
	{0x6, "PICMG 2.14 Multi Computing Controller"},
	{0x7, "Infiniband Controller"},
	{0x8, "Fabric Controller"},
	{0x80, "Other"},
};

const struct pci_sub_class display_controller_subclasses[] = {
	{0x0, "VGA Compatible Controller"},
	{0x1, "XGA Controller"},
	{0x2, "3D Controller (Not VGA-Compatible)"},
	{0x80, "Other"},
};

const struct pci_sub_class bridge_subclasses[] = {
	{0x0, "Host Bridge"},
	{0x1, "ISA Bridge"},
	{0x2, "EISA Bridge"},
	{0x3, "MCA Bridge"},
	{0x4, "PCI-to-PCI Bridge"},
	{0x5, "PCMCIA Bridge"},
	{0x6, "NuBus Bridge"},
	{0x7, "CardBus Bridge"},
	{0x8, "RACEway Bridge"},
	{0x9, "PCI-to-PCI Bridge"},
	{0x0A, "InfiniBand-to-PCI Host Bridge"},
	{0x80, "Other"},
};


const struct pci_class pci_classes[] = {
	{0x0, "Unclassified", unclassified_subclasses, 2},
	{0x1, "Mass Storage Controller", mass_storage_controller_subclasses, 10},
	{0x2, "Network Controller", network_controller_subclasses, 10},
	{0x3, "Display Controller", display_controller_subclasses, 4},
	{0x4, "Multimedia Controller", NULL, 0},
	{0x5, "Memory Controller", NULL, 0},
	{0x6, "Bridge", bridge_subclasses, 12},
	{0x7, "Simple Communication Controller", NULL, 0},
	{0x8, "Base System Peripheral", NULL, 0},
	{0x9, "Input Device Controller", NULL, 0},
	{0xA, "Docking Station", NULL, 0},
	{0xB, "Processor", NULL, 0},
	{0xC, "Serial Bus Controller", NULL, 0},
	{0xD, "Wireless Controller", NULL, 0},
	{0xE, "Intelligent Controller", NULL, 0},
	{0xF, "Satellite Communication Controller", NULL, 0},
	{0x10, "Encryption Controller", NULL, 0},
	{0x11, "Signal Processing Controller",NULL, 0},
	{0x12, "Processing Accelerator", NULL, 0},
	{0x13, "Non-Essential Instrumentation", NULL, 0},
	{0x14, "0x3F (Reserved)", NULL, 0},
	{0x40, "Co-Processor", NULL, 0},
	{0x41, "0xFE (Reserved)", NULL, 0},
	{0xFF, "Unassigned Class (Vendor specific)", NULL, 0},
};

#endif