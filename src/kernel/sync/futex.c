#include "kernel/sync/futex.h"
#include "hvos/compiler_attributes.h"
#include <stdatomic.h>
#include <stdint.h>

int64_t futex_wait(uint32_t *addr, int op, uint32_t val) {

}

int64_t futex_wake(uint32_t *addr, int op, uint32_t val) {
	
}

static uint32_t futex_to_flags(uint32_t op) {
	
}

int64_t do_futex(uint32_t *addr, int op, uint32_t val) {
	int cmd = op & FUTEX_CMD_MASK;
	uint32_t flags = futex_to_flags(op);
	switch (cmd) {
		case FUTEX_WAIT:
			fallthrough;
		case FUTEX_WAIT_BITSET:
			return futex_wait(addr, flags, val);

		case FUTEX_WAKE:
			fallthrough;
		case FUTEX_WAKE_BITSET:
			return futex_wake(addr, flags, val);
		/*unimplemented*/
		case FUTEX_FD:
			break;
		case FUTEX_REQUEUE:
			break;
		case FUTEX_CMP_REQUEUE:
			break;
		case FUTEX_WAKE_OP:
			break;
		case FUTEX_LOCK_PI:
			break;
		case FUTEX_UNLOCK_PI:
			break;
		case FUTEX_TRYLOCK_PI:
			break;
		case FUTEX_WAIT_REQUEUE_PI:
			break;
		case FUTEX_CMP_REQUEUE_PI:
			break;
		case FUTEX_LOCK_PI2:
			break;
	}
	return -1;
}