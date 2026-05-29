#ifndef HVOS_CPU_IO_H
#define HVOS_CPU_IO_H
#include <stddef.h>
#include <stdint.h>
#include "cpu/ports.h"

static inline void io_write_8(uint32_t port, uint8_t data) {
	__asm__ volatile("outb %b0, %w1" : : "a" (data), "Nd" (port));
}

static inline uint8_t io_read_8(uint32_t port) {
	uint8_t data;
	__asm__ volatile("inb %w1, %b0" : "=a" (data) : "Nd" (port));
	return data;
}

static inline void io_write_16(uint32_t port, uint16_t data) {
	__asm__ volatile("outw %w0, %w1" : : "a" (data), "Nd" (port));
}

static inline uint16_t io_read_16(uint32_t port)
{
    uint16_t data;
    __asm__ volatile("inw %w1, %w0" : "=a" (data) : "Nd" (port));
    return data;
}

static inline void io_write_32(uint32_t port, uint32_t data)
{
    __asm__ volatile("outl %0, %w1" : : "a" (data), "Nd" (port));
}

static inline uint32_t io_read_32(uint32_t port)
{
    uint32_t data;
    __asm__ volatile("inl %w1, %0" : "=a" (data) : "Nd" (port));
    return data;
}

static inline void io_wait() {
	io_write_8(PORT_POST_CHECK, 0x00);
}

static inline void mmio_write_8(void *p, uint8_t data)
{
    *(volatile uint8_t *)(p) = data;
}

static inline uint8_t mmio_read_8(void *p)
{
    return *(volatile uint8_t *)(p);
}

static inline void mmio_write_16(void *p, uint16_t data)
{
    *(volatile uint16_t *)(p) = data;
}

static inline uint16_t mmio_read_16(void *p)
{
    return *(volatile uint16_t *)(p);
}

static inline void mmio_write_32(void *p, uint32_t data)
{
    *(volatile uint32_t *)(p) = data;
}

static inline uint32_t mmio_read_32(void *p)
{
    return *(volatile uint32_t *)(p);
}

static inline void mmio_write_64(void *p, uint64_t data)
{
    *(volatile uint64_t *)(p) = data;
}

static inline uint64_t mmio_read_64(void *p)
{
    return *(volatile uint64_t *)(p);
}

static inline void mmio_read_n(void *dst, const volatile void *src, size_t bytes) {
	volatile uint8_t *s = (volatile uint8_t *)src;
	uint8_t *d = (uint8_t *) dst;
	while (bytes > 0)
	{
		*d = *s;
		++s;
		++d;
		--bytes;
	}
}

#endif