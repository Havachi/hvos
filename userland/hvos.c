#include "hvos.h"
#include "kernel/syscall_id.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void clear_screen() {
	int res; 
	char buf[2] = {CLEAR, 0x00 };
}

void user_print(const char *str) {
	printf(str);
}

void user_printerr(const char *str) {
	fprintf(stderr, str);
}