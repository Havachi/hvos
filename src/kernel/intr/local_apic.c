#include "kernel/local_apic.h"

uint8_t *g_local_apic_address;
uint32_t local_apic_id = 255;

static uint32_t local_apic_in(uint32_t reg) {
	return mmio_read_32(g_local_apic_address + reg);
}

static void local_apic_out(uint32_t reg, uint32_t data) {
	mmio_write_32(g_local_apic_address + reg, data);
}

void local_apic_init() {
	local_apic_out(LAPIC_TPR, 0);
	local_apic_out(LAPIC_DFR, 0xFFFFFFFF);
	local_apic_out(LAPIC_LDR, 0x01000000);
	local_apic_out(LAPIC_SVR, 0x100 | 0xFF);
}

void local_apic_timer_init() {
	local_apic_out(0x3E0, 0x03);
	local_apic_out(0x320, 0x20 | 0x20000);
	local_apic_out(0x380, 0x100000);
}

uint32_t local_apic_get_id() {
	if (local_apic_id != 255 ){
		return local_apic_id;
	} else {
		local_apic_id = local_apic_in(LAPIC_ID) >> 24;
	}
	return local_apic_id;
}

void local_apic_send_init(uint32_t apic_id) {
	local_apic_out(LAPIC_ICRHI, apic_id << ICR_DESTINATION_SHIFT);
	local_apic_out(LAPIC_ICRLO, ICR_INIT | ICR_PHYSICAL | ICR_ASSERT | ICR_EDGE | ICR_NO_SHORTHAND);
	while (local_apic_in(LAPIC_ICRLO) & ICR_SEND_PENDING)
		;
}

void local_apic_send_startup(uint32_t apic_id, uint32_t vector){
	local_apic_out(LAPIC_ICRHI, apic_id << ICR_DESTINATION_SHIFT);
	local_apic_out(LAPIC_ICRLO, vector | ICR_STARTUP | ICR_PHYSICAL | ICR_ASSERT | ICR_EDGE | ICR_NO_SHORTHAND);
	while (local_apic_in(LAPIC_ICRLO) & ICR_SEND_PENDING)
		;	
}
