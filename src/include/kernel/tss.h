#ifndef HVOS_TSS_H
#define HVOS_TSS_H
#include <stdint.h>
#include "kernel/mem.h"
#include "klibc/string.h"

typedef struct tss {
	uint32_t reserved0;
	uint64_t rsp0;
	uint64_t rsp1;
	uint64_t rsp2;
	uint64_t reserved1;
	uint64_t ist[7];
	uint64_t reserved2;
	uint16_t reserved3;
	uint16_t iomap_base;
} __attribute__((packed)) tss_t;

void tss_init(void);
void tss_set_stack(uint64_t kernel_stack);

#endif