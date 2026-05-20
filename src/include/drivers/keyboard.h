#ifndef HVOS_DRIVERS_KDB_H
#define HVOS_DRIVERS_KDB_H
#include <stdint.h>
#include <stdbool.h>
#include "drivers/scancode.h"
#include "kernel/sync.h"
#include "kernel/asm.h"
#include "kernel/mem.h"

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define BACKSPACE 0x0E
#define LSHIFT 0x2A
#define RSHIFT 0x36
#define KDB_BUFFER_SIZE 256

#endif