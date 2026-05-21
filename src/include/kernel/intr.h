#ifndef HVOS_INTR_H
#define HVOS_INTR_H

#define IRQ_BASE                        0x20

#define IRQ_TIMER                       0x00
#define IRQ_KEYBOARD                    0x01
#define IRQ_COM2                        0x03
#define IRQ_COM1                        0x04
#define IRQ_FLOPPY                      0x06
#define IRQ_ATA0                        0x0E
#define IRQ_ATA1                        0x0F


#define INT_DIVIDE						0x0
#define INT_DBG_EX						0x1
#define INT_NMI							0x2
#define INT_BREAKPOINT					0x3
#define INT_OVERFLOW					0x4
#define INT_BOUND						0x5
#define INT_INVALID_OPCODE				0x6
#define INT_DEVICE_NOT_AVAILABLE		0x7
#define INT_DOUBLE_FAULT				0x8
#define INT_COPR_SEG_OVRN				0x9
#define INT_INV_TSS						0xA
#define INT_SEGMENT_NOT_PRESENT			0xB
#define INT_STACK_SEGMENT_FAULT			0xC
#define INT_GP							0xD
#define INT_PAGE_FAULT					0xE
#define INT_RESERVED					0xF

#define INT_TIMER                       0x20
#define INT_KBD                   		0x21
#define INT_SPURIOUS                    0xFF
#define INT_SCHEDULER					0x30
void intr_init();

#endif