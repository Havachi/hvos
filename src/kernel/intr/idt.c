#include "kernel/idt.h"

idt_entry_t idt[256] = {0};

extern void default_exception_handler();
extern void default_interrupt_handler();
extern void (*exception_handlers[20])();


void idt_init() {
	for (uint32_t i = 0; i < 20; ++i) {
		idt_set_handler(i, INTERRUPT_GATE, exception_handlers[i]);
	}

	for (uint32_t i = 20; i < 32; ++i) {
		idt_set_handler(i, INTERRUPT_GATE, default_exception_handler);
	}

	for (uint32_t i = 32; i < 256; ++i) {
		idt_set_handler(i, TRAP_GATE, default_interrupt_handler);
	}

	idt_ptr_t idt_ptr = {
		.limit = (256 * sizeof(idt_entry_t)) - 1,
		.base = (uint64_t)&idt
	};
	__asm__ volatile("lidt %0" : : "m"(idt_ptr) : "memory");
}

static void idt_set_entry(uint8_t index, uint64_t base, uint16_t selector, uint16_t type) {
	idt_entry_t *entry = &idt[index];

	entry->offset_1 = (uint16_t)base;
	entry->selector = selector;
	entry->type_attributes = type;
	entry->offset_2 = (uint16_t)(base >> 16);
	entry->offset_3 = (uint32_t)(base >> 32);
	entry->zero = 0;
}

void idt_set_handler(uint8_t index, uint16_t type, void (*handler)()) {
	if (handler) {
		uint16_t selector = 0x8;
		idt_set_entry(index, (uint64_t)handler, selector, type);
	} else {
		idt_set_entry(index, 0, 0, 0);
	}
}

void idt_set_gate(size_t vector, void *handler, uint8_t ist) {
	uint64_t addr = (uint64_t)handler;

	idt[vector].offset_1 = (uint16_t)(addr & 0xFFFF);
	idt[vector].selector = 0x08;
	idt[vector].ist = ist;
	idt[vector].type_attributes = 0x8E;
	idt[vector].offset_2 = (uint16_t)((addr >> 16) & 0xFFFF);
	idt[vector].offset_3 = (uint32_t)((addr >> 32) & 0xFFFFFFFF);
	idt[vector].zero = 0 ;

}
