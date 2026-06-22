#ifndef HVOS_SMP_H
#define HVOS_SMP_H

#include <stdint.h>
#include <stdio.h>
#include "kernel/acpi.h"
#include "kernel/local_apic.h"
#include "kernel/pit.h"
#include "kernel/boot.h"


typedef struct {
	uint64_t tss_sp2;
	uint64_t stack_top;
} __attribute__((packed)) cpu_data_t;

extern volatile uint8_t g_active_cpu_count;
void smp_init();

void init_smp_data();
cpu_data_t *get_current_cpu_data();

#endif