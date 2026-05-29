#ifndef HVOS_PRINTF_H
#define HVOS_PRINTF_H
#include <stdarg.h>
#include <stddef.h>

void kprintf(const char * format, ...);
void kprintf_err(const char * format, ...);
void kvprintf(const char *format, va_list args);
void ksprintf(char *str, const char *fmt, ...);
void kvsprintf(char *str, const char *fmt, va_list args);
#endif