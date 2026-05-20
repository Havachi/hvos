#ifndef HVOS_IO_APIC_H
#define HVOS_IO_APIC_H
#include <stdint.h>
#include "cpu/io.h"
// Memory mapped registers for IO APIC register access
#define IOREGSEL                        0x00
#define IOWIN                           0x10

// ------------------------------------------------------------------------------------------------
// IO APIC Registers
#define IOAPICID                        0x00
#define IOAPICVER                       0x01
#define IOAPICARB                       0x02
#define IOREDTBL                        0x10

extern uint8_t *g_io_apic_addr;

void io_apic_init();
void io_apic_set_entry(uint8_t *base, uint8_t index, uint64_t data);

#endif