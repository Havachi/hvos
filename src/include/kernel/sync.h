#ifndef HVOS_SYNC_H
#define HVOS_SYNC_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
	volatile int locked;
} spinlock_t;

typedef struct {
	volatile int locked;
} safe_lock_t;


extern int32_t atomic_exchange(volatile int *v, int32_t val);

void spin_lock(spinlock_t *lock);
void spin_unlock(spinlock_t *lock);
uint64_t safe_lock(safe_lock_t *lock);
void safe_unlock(safe_lock_t *lock, uint64_t rflags);

static inline uint64_t save_and_disable_interrupt(void) {
	uint64_t rflags;
	asm volatile (
		"pushfq\n\t"
		"cli\n\t"
		"pop %0"
		: "=r"(rflags)
		:
		: "memory"
	);
	return rflags;
}

static inline void restore_interrupts(uint64_t rflags) {
	asm volatile (
		"push %0\n\t"
		"popfq"
		:
		: "rm"(rflags)
		: "memory", "cc"
	);
}

#endif