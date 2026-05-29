#ifndef HVOS_CPU_PORTS_H
#define HVOS_CPU_PORTS_H

#define PORT_DMA1_BASE          0x0000
#define PORT_DMA2_BASE          0x00C0

#define PORT_PIC_MASTER_CMD     0x0020
#define PORT_PIC_MASTER_DATA    0x0021
#define PORT_PIC_SLAVE_CMD      0x00A0
#define PORT_PIC_SLAVE_DATA     0x00A1

#define PORT_PIT_CHANNEL0       0x0040
#define PORT_PIT_CHANNEL1       0x0041
#define PORT_PIT_CHANNEL2       0x0042
#define PORT_PIT_CMD            0x0043

#define PORT_PS2_DATA           0x0060
#define PORT_PS2_STATUS         0x0064
#define PORT_PS2_CMD            0x0064

#define PORT_CMOS_INDEX         0x0070
#define PORT_CMOS_DATA          0x0071

#define PORT_POST_CHECK         0x0080
#define PORT_QEMU_DEBUG         0x00E9

#define PORT_FPU_CLEAR          0x00F0

#define PORT_IDE1_BASE          0x01F0
#define PORT_IDE2_BASE          0x0170

#define PORT_LPT3_BASE          0x03BC
#define PORT_LPT1_BASE          0x0378
#define PORT_LPT2_BASE          0x0278

#define PORT_COM1_BASE          0x03F8
#define PORT_COM2_BASE          0x02F8
#define PORT_COM3_BASE          0x03E8
#define PORT_COM4_BASE          0x02E8

#define PORT_VGA_ATTRIB_INDEX   0x03C0
#define PORT_VGA_ATTRIB_WRITE   0x03C0
#define PORT_VGA_ATTRIB_READ    0x03C1
#define PORT_VGA_MISC_READ      0x03CC
#define PORT_VGA_MISC_WRITE     0x03C2
#define PORT_VGA_CRTC_INDEX     0x03D4
#define PORT_VGA_CRTC_DATA      0x03D5

#define PORT_PCI_CONFIG_ADDR    0x0CF8
#define PORT_PCI_CONFIG_DATA    0x0CFC

#define PORT_ACPI_SMI_CMD       0x00B2
#endif