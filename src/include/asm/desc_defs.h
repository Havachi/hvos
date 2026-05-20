#ifndef HVOS_DESC_DEFS_H
#define HVOS_DESC_DEFS_H
#include <stdint.h>
enum {
	GATE_INTERRUPT = 0xE,
	GATE_TRAP = 0xF,
	GATE_CALL = 0xC,
	GATE_TASK = 0x5,
};

typedef struct seg_descr {
	uint16_t	limit0;
	uint16_t	base0;
	uint16_t	base1, type: 4, s: 1, dpl: 2, p: 1;
	uint16_t	limit1: 4, avl: 1, l: 1, d: 1, g: 1, base2: 8;
}__attribute__((packed)) seg_descr_t;

struct desc_ptr {
	uint16_t size;
	uint64_t address;
} __attribute__((packed));

#define GDT_ENTRY_INIT(flags, base, limit)			\
	{												\
		.limit0	= ((limit)	>>  0) & 0xFFFF,			\
		.limit1	= ((limit)	>> 16) & 0x000F,			\
		.base0	= ((base)	>>  0) & 0xFFFF,			\
		.base1	= ((base)	>> 16) & 0x00FF,			\
		.base2	= ((base)	>> 24) & 0x00FF,			\
		.type	= ((flags)	>>  0) & 0x000F,			\
		.s		= ((flags)	>>  4) & 0x0001,			\
		.dpl	= ((flags)	>>  5) & 0x0003,			\
		.p		= ((flags)	>>  7) & 0x0001,			\
		.avl	= ((flags)	>> 12) & 0x0001,			\
		.l		= ((flags)	>> 13) & 0x0001,			\
		.d		= ((flags)	>> 14) & 0x0001,			\
		.g		= ((flags)	>> 15) & 0x0001,			\
	}


#define _DESC_ACCESSED			0x0001
#define _DESC_DATA_WRITABLE		0x0002
#define _DESC_CODE_READABLE		0x0002
#define _DESC_DATA_EXPAND_DOWN	0x0004
#define _DESC_CODE_CONFORMING	0x0004
#define _DESC_CODE_EXECUTABLE	0x0008

#define _DESC_S					0x0010
#define _DESC_DPL(dpl)			((dpl) << 5)
#define _DESC_PRESENT			0x0080

#define _DESC_LONG_CODE 		0x2000
#define _DESC_DB				0x4000
#define _DESC_GRAN_4K			0x8000

#define _DESC_SYSTEM(code)		(code)

#define _DESC_DATA 				(_DESC_S | _DESC_PRESENT | _DESC_ACCESSED | \
								 _DESC_DATA_WRITABLE)
#define _DESC_CODE				(_DESC_S | _DESC_PRESENT | _DESC_ACCESSED | \
								_DESC_CODE_READABLE | _DESC_CODE_EXECUTABLE )

#define DESC_CODE64				(_DESC_CODE | _DESC_GRAN_4K | _DESC_LONG_CODE)
#define DESC_DATA64				(_DESC_DATA | _DESC_GRAN_4K | _DESC_DB)
#define DESC_USER				(_DESC_DPL(3))
#endif