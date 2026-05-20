#ifndef HVOS_ASM_H
#define HVOS_ASM_H

#include <stdint.h>

void cli();
void sti();
void outb(uint16_t port, uint8_t data);
uint8_t inb(uint16_t port);
extern void reload_segments();

#endif