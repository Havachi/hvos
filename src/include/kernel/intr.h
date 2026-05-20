#ifndef HVOS_INTR_H
#define HVOS_INTR_H

#include "kernel/idt.h"
#include "kernel/io_apic.h"
#include "kernel/local_apic.h"
#include "kernel/pic.h"
#include "kernel/acpi.h"
#include "kernel/pit.h"

#define IRQ_BASE                        0x20

#define IRQ_TIMER                       0x00
#define IRQ_KEYBOARD                    0x01
#define IRQ_COM2                        0x03
#define IRQ_COM1                        0x04
#define IRQ_FLOPPY                      0x06
#define IRQ_ATA0                        0x0E
#define IRQ_ATA1                        0x0F

#define INT_TIMER                       0x20
#define INT_KBD                   		0x21
#define INT_SPURIOUS                    0xFF
#define INT_SCHEDULER					0x30
void intr_init();

#endif