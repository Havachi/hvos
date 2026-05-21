#include "kernel/intr.h"
#include "kernel/acpi.h"
#include "kernel/idt.h"
#include "kernel/io_apic.h"
#include "kernel/local_apic.h"
#include "kernel/pic.h"

extern void pit_interrupt();
extern void spurious_interrupt();
extern void scheduler_isr_asm();
extern void keyboard_interrupt(void);
extern void pagefault_interrupt(void);
extern void gpf_exc(void);

void intr_init() {
	idt_init();
	idt_set_handler(INT_TIMER, INTERRUPT_GATE, pit_interrupt);
	idt_set_handler(INT_KBD, INTERRUPT_GATE, keyboard_interrupt);
	idt_set_handler(INT_SPURIOUS, INTERRUPT_GATE, spurious_interrupt);
	idt_set_handler(INT_SCHEDULER, INTERRUPT_GATE, scheduler_isr_asm);
	//idt_set_handler(INT_PAGE_FAULT, INTERRUPT_GATE, pagefault_interrupt);
	idt_set_handler(INT_GP,INTERRUPT_GATE , gpf_exc);

	pic_init();
	local_apic_init();
	io_apic_init();
	io_write_8(0x22, 0x70);
	io_write_8(0x23, 0x01);
	local_apic_timer_init();

	uint64_t kbd_low_bits = (uint64_t)INT_KBD;

	uint64_t kbd_high_bits = ((uint64_t)local_apic_get_id()) << 56;

	io_apic_set_entry(g_io_apic_addr, acpi_remap_irq(IRQ_TIMER), INT_TIMER);
	io_apic_set_entry(g_io_apic_addr, acpi_remap_irq(IRQ_KEYBOARD), kbd_low_bits | kbd_high_bits);

}