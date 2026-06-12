#ifndef HVOS_SMP_H
#define HVOS_SMP_H

#include <stdint.h>
#include <stdio.h>
#include "kernel/acpi.h"
#include "kernel/local_apic.h"
#include "kernel/pit.h"
#include "kernel/boot.h"

extern volatile uint8_t g_active_cpu_count;
void smp_init();
#endif