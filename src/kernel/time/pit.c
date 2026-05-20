#include "kernel/pit.h"

#include "klibc/printf.h"

volatile uint64_t g_pit_ticks;
extern uint8_t *g_local_apic_address;

void pit_init(){

	uint32_t hz = 1000;
	uint32_t divisor = PIT_FREQUENCY / hz;
	io_write_8(PIT_CMD, CMD_BINARY | CMD_MODE3 | CMD_RW_BOTH | CMD_COUNTER0);
	io_write_8(PIT_COUNTER0, divisor);
	io_write_8(PIT_COUNTER0, divisor >> 8);
}

void pit_wait(uint32_t ms){
	uint64_t now = g_pit_ticks;
	while(g_pit_ticks - now < ms) {
		asm volatile("pause");
	}
}