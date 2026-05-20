#include "kernel/asm.h"

void cli() {
	__asm__ __volatile__("cli": : :"memory");
}

void sti() {
	__asm__ __volatile__("sti": : :"memory");
}

void outb(uint16_t port, uint8_t data) {
	__asm__ volatile ( "outb %b0, %w1" : : "a"(data), "Nd"(port) : "memory");
}

uint8_t inb(uint16_t port) {
	uint8_t ret;
	__asm__ volatile ("inb %w1, %b0" : "=a"(ret) : "Nd"(port) : "memory");
	return ret;
}