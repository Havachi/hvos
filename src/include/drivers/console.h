#ifndef _HVOS_CONSOLE_DRIVER_H
#define _HVOS_CONSOLE_DRIVER_H 1

#include "kernel/video.h"
#include "mem/mem.h"
#include <kernel/vfs.h>
#include <stdint.h>

file_t *create_kernel_console_file(int flags);
long console_read(file_t *__file, char *buf, size_t __size, uint64_t *__offset);
#endif