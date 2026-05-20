#include "kernel/io_apic.h"

uint8_t *g_io_apic_addr;

static void io_apic_out(uint8_t *base, uint8_t reg, uint32_t val) {
	mmio_write_32(base + IOREGSEL, reg);
	mmio_write_32(base + IOWIN, val);
}

static uint32_t io_apic_in(uint8_t *base, uint8_t reg) {
	mmio_write_32(base + IOREGSEL, reg);
	mmio_read_32(base + IOWIN);
}

void io_apic_set_entry(uint8_t *base, uint8_t index, uint64_t data) {
	io_apic_out(base, IOREDTBL + index * 2, (uint32_t)data);
	io_apic_out(base, IOREDTBL + index * 2 + 1, (uint32_t)(data >> 32));
}

void io_apic_init() {
	uint32_t x = io_apic_in(g_io_apic_addr, IOAPICVER);
	uint32_t count = ((x >> 16) & 0xFF) + 1;

	for (uint32_t i = 0; i < count; ++i) {
		io_apic_set_entry(g_io_apic_addr, i, 1 << 16);
	}
}