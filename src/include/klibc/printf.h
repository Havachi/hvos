#ifndef HVOS_PRINTF_H
#define HVOS_PRINTF_H
#include <stdarg.h>
#include "klibc/string.h"
#include "kernel/video.h"
#include "kernel/sync.h"
void kprintf(const char * format, ...);
void kvprintf(const char *format, va_list args);
#endif