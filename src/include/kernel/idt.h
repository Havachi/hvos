#ifndef HVOS_IDT_H
#define HVOS_IDT_H
#include <stdint.h>
#include "asm/asm.h"
#include <stdio.h>
#include "kernel/reg.h"
#include "mem/lowmem.h"

#define INTERRUPT_GATE		0x8E
#define TRAP_GATE			0x8F

typedef struct idt_entry_s {
	uint16_t offset_1;
	uint16_t selector;
	uint8_t ist;
	uint8_t type_attributes;
	uint16_t offset_2;
	uint32_t offset_3;
	uint32_t zero;
} __attribute__((packed)) idt_entry_t;

typedef struct idt_ptr_s {
	uint16_t limit;
	uint64_t base;
} __attribute__((packed)) idt_ptr_t;

typedef idt_ptr_t idtr_t;

typedef void (*handler_t)();

void isr_common_handler(registers_t *frame);
void idt_set_gate(size_t vector, void *handler, uint8_t attr);
void idt_reload(void);

void idt_init();
void idt_set_handler(uint8_t index, uint16_t type, void (*handler)());

#endif