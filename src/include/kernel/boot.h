#ifndef HVOS_BOOT_H
#define HVOS_BOOT_H

#include "limine.h"
extern const uint64_t limine_base_revision[];
extern volatile struct limine_hhdm_request hhdm_request;
extern volatile struct limine_memmap_request memmap_request;
extern volatile struct limine_framebuffer_request framebuffer_request;
extern volatile struct limine_module_request module_request;
extern volatile struct limine_rsdp_request rsdp_request;
extern volatile struct limine_smp_request smp_request;

void hcf(void);
void delay(int count);

#endif