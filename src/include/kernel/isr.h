#ifndef HVOS_ISR_H
#define HVOS_ISR_H
#include "kernel/idt.h"

#define PASTER(x, y) x##y
#define EVALUATOR(x, y) PASTER(x, y)
// This first macro makes functions like "void isr0(void)"
// But the number is summed up on each call
#define ONE EVALUATOR(void isr, __COUNTER__)(void)
// Different granularities
#define TWO \
	ONE;    \
	ONE
#define TEN \
	TWO;    \
	TWO;    \
	TWO;    \
	TWO;    \
	TWO
#define TWENTY \
	TEN;       \
	TEN
#define HUNDRED \
	TWENTY;     \
	TWENTY;     \
	TWENTY;     \
	TWENTY;     \
	TWENTY
// Define 255 ISRs based on previous granularities
#define DEFISR \
	HUNDRED;   \
	HUNDRED;   \
	TWENTY;    \
	TWENTY;    \
	TEN;       \
	TWO;       \
	TWO;       \
	TWO

DEFISR;

typedef void (*event_handlers_t)(register_t *);

void isr_install(void);
void isr_common_handler(register_t *frame);
void isr_register_handler(int n, void *handler);
#endif