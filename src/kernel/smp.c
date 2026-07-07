#include <stdint.h>
#include <stdio.h>
#include "kernel/smp.h"
#include "mem/mem.h"

volatile uint8_t g_active_cpu_count;

cpu_data_t **cpu_data_list;


void ap_entry(struct limine_smp_info *smp_info) {

	printf("Hello from CPU core ID: %d!\n", smp_info->processor_id);
	__atomic_fetch_add(&g_active_cpu_count, 1, __ATOMIC_SEQ_CST);
	__asm__ __volatile("sti");
	for (;;) {
		__asm__ __volatile("hlt");
	}
}

cpu_data_t *get_current_cpu_data() {
	return cpu_data_list[local_apic_get_id()];
}

void init_smp_data() {
	struct limine_smp_response *smp_response = smp_request.response;
	uint64_t nb_cpu = smp_response->cpu_count;

	cpu_data_list = (cpu_data_t **)kzalloc(sizeof(cpu_data_t) * nb_cpu);

	for(uint64_t i = 0; i < nb_cpu; i++) {
		cpu_data_list[i] = kzalloc(sizeof(cpu_data_t));
		cpu_data_list[i]->stack_top = 0;
		cpu_data_list[i]->tss_sp2 = 0;
	}
}

void smp_init() {
	struct limine_smp_response *smp_response = smp_request.response;

	if (smp_response == NULL || smp_response->cpu_count <= 1) {
		printf("SMP not supported\n");
		return;
	}

	g_acpi_cpu_count = smp_response->cpu_count;
	g_active_cpu_count = 1;
	printf("Detected %d cpu cores\n", g_acpi_cpu_count);
	for (uint64_t i = 0; i < smp_response->cpu_count; i++) {
		struct limine_smp_info *cpu = smp_response->cpus[i];
		if (cpu->lapic_id == smp_response->bsp_lapic_id) {
			continue;
		}
		printf ("Waking up AP core %d\n", cpu->processor_id);
		__atomic_store_n(&cpu->goto_address, ap_entry, __ATOMIC_SEQ_CST);
	}
	while (g_active_cpu_count != g_acpi_cpu_count)
	{
		printf("Waiting for cores... %d/%d\n", g_active_cpu_count, g_acpi_cpu_count);
		pit_wait(1);
	}
	printf("All CPUs activated\n");
}

