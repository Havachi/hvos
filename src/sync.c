#include "kernel/sync.h"


void spin_lock(spinlock_t *lock) {
	while (atomic_exchange(&lock->locked, 1) != 0) {
		asm volatile("pause");
	}
}

void spin_unlock(spinlock_t *lock) {
	lock->locked = 0;
}

uint64_t safe_lock(safe_lock_t *lock) {
	uint64_t rflags = save_and_disable_interrupt();

	while (atomic_exchange(&lock->locked, 1) != 0) {
		asm volatile("pause");
	}

	return rflags;
}

void safe_unlock(safe_lock_t *lock, uint64_t rflags) {
	lock->locked = 0;
	restore_interrupts(rflags);
}