#ifndef HVOS_APIC_H
#define HVOS_APIC_H

#include <stdint.h>


#define PACKED __attribute__((__packed__))

#define IA32_APIC_BASE_MSR	0x1B

#define IOAPIC_PHYS_BASE	0xFEC00000
#define IOAPIC_REG_INDEX	0x00
#define IOAPIC_REG_DATA		0x10

#define IOAPIC_REDTBL_KEYBOARD_LOW  0x12
#define IOAPIC_REDTBL_KEYBOARD_HIGH 0x13

#define APIC_TYPE_LOCAL_APIC            0
#define APIC_TYPE_IO_APIC               1
#define APIC_TYPE_INTERRUPT_OVERRIDE    2


#define MAX_CPU_COUNT 16
extern uint32_t g_acpi_cpu_count;
extern uint8_t g_acpi_cpu_ids[MAX_CPU_COUNT];

/* APIC STRUCTS DEFS */

typedef struct {
	uint8_t address_space;
	uint8_t bit_width;
	uint8_t bit_offset;
	uint8_t access_size;
	uint64_t address;
} acpi_generic_address_t;

typedef struct {
	uint32_t signature;
	uint32_t length;
	uint8_t revision;
	uint8_t checksum;
	uint8_t oem[6];
	uint8_t oem_table_id[8];
	uint32_t oem_revision;
	uint32_t creator_id;
	uint32_t creator_revision;
} PACKED acpi_header_t;

typedef struct  {
	acpi_header_t acpi_header;

	uint32_t 	firmware_ctrl;
	uint32_t 	dsdt;
	uint8_t 	reserver;
	uint8_t 	preferred_pm_profile;
	uint16_t 	sci_interrupt;
	uint32_t 	smi_command_port;
	uint8_t 	acpi_enable;
	uint8_t 	acpi_disable;
	
	uint8_t 	s4bios_req;
	uint8_t 	p_pstate_control;
	uint32_t	pm1_a_event_block;
	uint32_t	pm1_b_event_block;
	uint32_t	pm1_a_control_block;
	uint32_t	pm1_b_control_block;
	uint32_t	pm2_control_block;
	uint32_t	pm_timer_block;
	uint32_t	gpe0_block;
	uint32_t	gpe1_block;
	uint8_t 	pm1_event_length;
	uint8_t 	pm1_control_length;
	uint8_t 	pm2_control_length;
	uint8_t 	pm_timer_length;
	uint8_t 	gpe0_length;
	uint8_t 	gpe1_length;
	uint8_t 	gpe1_base;
	uint8_t 	c_state_control;
	uint16_t 	worst_c2_latency;
	uint16_t 	worst_c3_latency;
	uint16_t 	flush_size;
	uint16_t 	flush_stride;
	uint8_t 	duty_offset;
	uint8_t 	duty_width;
	uint8_t 	day_alarm;
	uint8_t 	month_alarm;
	uint8_t 	century;
	uint16_t 	boot_arch_flags;
	uint8_t 	reserved2;
	uint32_t 	flags;
	acpi_generic_address_t reset_reg;
	uint8_t		reset_value;
	uint8_t		reserved3[3];
	uint64_t	x_frimware_control;
	uint64_t	x_dsdt;

    acpi_generic_address_t x_pm1_a_event_block;
    acpi_generic_address_t x_pm1_b_event_block;
    acpi_generic_address_t x_pm1_a_control_block;
    acpi_generic_address_t x_pm1_b_control_block;
    acpi_generic_address_t x_pm2_control_block;
    acpi_generic_address_t x_pm_timer_block;
    acpi_generic_address_t x_gpe_0_block;
    acpi_generic_address_t x_gpe_1_block;
} PACKED acpi_fadt;



typedef struct {
	char signature[8];
	uint8_t checksum;
	char oem_id[6];
	uint8_t revision;
	uint32_t rsdt_address; // deprecated since 2.0

	uint32_t length;
	uint64_t xsdt_address;
	uint8_t	extended_checksum;
	uint8_t reserved [3];
} PACKED acpi_rsdp_t;

typedef struct {
	char signature[4];
	uint32_t length;
	uint8_t revision;
	uint8_t checksum;
	char oem_id[6];
	char oem_table_id[8];
	uint32_t oem_revision;
	uint32_t creator_id;
	uint32_t creator_revision;
} PACKED acpi_sdt_header_t;

typedef struct {
	acpi_sdt_header_t header;
	uint32_t entries[];
} PACKED acpi_rsdt_t;
/*APIC (MADT)*/
typedef struct {
	acpi_header_t header;
	uint32_t local_apic_address;
	uint32_t flags;
	uint8_t entries[];
} PACKED acpi_madt_t;

typedef struct {
	uint8_t type;
	uint8_t length;
} PACKED apic_header_t;

typedef struct  {
	apic_header_t header;
	uint8_t acpi_processor_id;
	uint8_t apic_id;
	uint32_t flags;
} PACKED madt_lapic_t;

typedef struct {
	apic_header_t header;
	uint8_t io_apic_id;
	uint8_t reserved;
	uint32_t io_apic_address;
	uint32_t global_system_interrupt_base;
} PACKED madt_iopic_t;

typedef struct {
	apic_header_t header;
	uint8_t bus;
	uint8_t source;
	uint32_t interrupt;
	uint16_t flags;
} PACKED madt_iso_t;


void acpi_init();
uint32_t acpi_remap_irq(uint32_t irq);

#endif