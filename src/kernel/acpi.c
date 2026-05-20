#include "kernel/acpi.h"
#ifdef PRINTV
#undef 	PRINTV
#endif
#define PRINTV(...) printverbose(6, __VA_ARGS__)

//Globals

uint32_t g_acpi_cpu_count;
uint8_t	g_acpi_cpu_ids[MAX_CPU_COUNT];

static acpi_madt_t *s_madt;

static acpi_rsdp_t *s_rsdp = NULL;
static acpi_sdt_header_t *s_rsdt = NULL;

uintptr_t lapic_phys_base;
uintptr_t lapic_virt_base;

#define PIC1_DATA 0x21
#define PIC2_DATA 0xA1


static void acpi_parse_facp(acpi_fadt *facp) {
	if (facp->smi_command_port){

	} else {
		PRINTV("ACPI already enabled\n");
	}
}

static void register_cpu_core(uint8_t apic_id) {
	g_acpi_cpu_ids[g_acpi_cpu_count++] = apic_id;
}

static void acpi_parse_apic(acpi_madt_t *madt) {
	s_madt = madt;

	uint32_t local_apic_phys = madt->local_apic_address;
	uint32_t madt_len = madt->header.length;
	PRINTV("Local APIC Address = %016lx\n", local_apic_phys);
	g_local_apic_address = (uint8_t *) (uintptr_t)(madt->local_apic_address + hhdm_offset);
	uint8_t *current_entry = (uint8_t *)madt + 44;
	uint8_t *end = (uint8_t *)madt + madt_len;

	while (current_entry < end) {
		uint8_t entry_type = current_entry[0];
		uint8_t entry_len = current_entry[1];

		if (entry_len == 0) {
			kprintf("Error: Malformed MADT entry with length 0\n");
			break;
		}

		switch (entry_type) {
			case APIC_TYPE_LOCAL_APIC:
				madt_lapic_t *lapic = (madt_lapic_t *)current_entry;
				if ((lapic->flags & 1) || (lapic->flags & 2)) {
					kprintf("Found CPU Core - ACPI ID: %d, APIC ID: %d\n", lapic->acpi_processor_id, lapic->apic_id);
					register_cpu_core(lapic->apic_id);
				}
				break;
			case APIC_TYPE_IO_APIC:
				madt_iopic_t *ioapic = (madt_iopic_t *)current_entry;
				kprintf("Found I/O APIC - ID: %d, Address: %016lx, GSI Base: %d\n", ioapic->io_apic_id, ioapic->io_apic_address, ioapic->global_system_interrupt_base);
				g_io_apic_addr = (uint8_t *)(ioapic->io_apic_address + hhdm_offset);
				break;
			case APIC_TYPE_INTERRUPT_OVERRIDE:
				madt_iso_t *iso = (madt_iso_t *) current_entry;
				kprintf("Found Interrupt Override - ISA IRQ %d -> GSI Pin %d\n", iso->source, iso->bus);
				break;
			default:
				break;
		}
		current_entry += entry_len;
	}
}

static void acpi_parse_dt(acpi_sdt_header_t *header) {
	char *signature = header->signature;

	char sig[5] = {0};
	strcpy(sig, signature);
	sig[4] = '\0';
	PRINTV("%s \n", sig);
	if (!strncmp(sig, "FACP", 4)) {
		acpi_parse_facp((acpi_fadt *)header);
	} else if (!strncmp(sig, "MADT", 4) || !strncmp(sig, "APIC", 4)) {
		acpi_parse_apic((acpi_madt_t *)header);
	}
}


static void acpi_parse_rsdt(acpi_rsdt_t *rsdt) {
	s_rsdt = &rsdt->header;
	uint32_t nb_entries = ((rsdt->header.length - sizeof(acpi_sdt_header_t)) / 4);
	for (int i = 0; i < nb_entries; i++) {
		acpi_parse_dt((acpi_sdt_header_t *)(uintptr_t)(rsdt->entries[i] + hhdm_offset));
	}
}

static void acpi_parse_xsdt(acpi_header_t *xsdt) {
	uint64_t *p = (uint64_t *)(xsdt + 1);
	uint64_t *end = (uint64_t *)((uint8_t *)xsdt + xsdt->length);

	while(p < end) {
		uint64_t address = *p++;
		acpi_parse_dt((acpi_sdt_header_t *)(uintptr_t)address +hhdm_offset);
	}
}

static bool acpi_parse_rsdp() {
	PRINTV("RSDP found\n");
	if (s_rsdp->checksum == 0) {
		PRINTV("checksum failed\n");
		return false;
	}
	PRINTV("OEM = %s\n", s_rsdp->oem_id);
	if (s_rsdp->revision == 0) {
		PRINTV("Version 1\n");
		acpi_parse_rsdt((acpi_rsdt_t *)(uintptr_t)(s_rsdp->rsdt_address + hhdm_offset));
	} else if (s_rsdp->revision == 2) {
		PRINTV("Version 2\n");
		if (s_rsdp->xsdt_address) {
			acpi_parse_xsdt((acpi_header_t *)(uintptr_t)s_rsdp->xsdt_address);
		} else {
			acpi_parse_rsdt((acpi_rsdt_t *)(uintptr_t)s_rsdp->rsdt_address);
		}
	} else {
		PRINTV("Unsupported ACPI version %d\n", s_rsdp->revision);
	}
	return true;
}

void acpi_init() {
	s_rsdp = (acpi_rsdp_t *)(uintptr_t)rsdp_request.response->address;
	uint64_t signature = *(uint64_t *)(s_rsdp->signature);
	if (signature == 0x2052545020445352) {
		acpi_parse_rsdp();
	}
}

uint32_t acpi_remap_irq(uint32_t irq){
	acpi_madt_t *madt = s_madt;
	uint32_t madt_len = madt->header.length;
	uint8_t *current_entry = (uint8_t *)madt + 44;
	uint8_t *end = (uint8_t *)madt + madt_len;

	while (current_entry < end) {
		uint8_t entry_type = current_entry[0];
		uint8_t entry_len = current_entry[1];

		if (entry_type == APIC_TYPE_INTERRUPT_OVERRIDE){
			madt_iso_t *s = (madt_iso_t *)current_entry;

			if (s->source == irq) {
				return s->interrupt;
			}
		}

		current_entry += entry_len;
	}
	return irq;
}