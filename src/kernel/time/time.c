#include "kernel/time.h"
#include <stdint.h>
#include <stdlib.h>

time_t time( time_t *arg ) {
	datetime_t *t = now();
	time_t ti = dttots(t);
	if (arg != NULL) {
		*arg = ti;
	}
	return ti;
}
