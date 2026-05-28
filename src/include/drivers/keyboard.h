#ifndef HVOS_DRIVERS_KDB_H
#define HVOS_DRIVERS_KDB_H
#include <stdint.h>
#include <stdbool.h>


#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define BACKSPACE 0x0E
#define LSHIFT 0x2A
#define RSHIFT 0x36
#define LCTRL 0x1D
#define KDB_BUFFER_SIZE 256

#define CANCEL 0x18
#define CLEAR 0x02

#endif