#include "hvos.h"

static char input_buffer[128];
static int index = 0;

static int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

void _start(void) {
	user_print("\nWelcome to HVOS 0.1\n");
	while (1) {
		user_print("hvos>");
		index = 0;
		while (1) {
			char c;
			read(0, &c, 1);
			if (c == '\n') {
				input_buffer[index] = '\0';
				user_print("\n");
				break;
			} else if (c == '\b') {
				if (index > 0) {
					index--;
					user_print("\b \b");
				}
			} else if(index < 127) {
				input_buffer[index++] = c;
				write(1, &c, 1);
			}
		}

		if (strcmp(input_buffer, "help") == 0) {
			user_print("Help menu !\n");
		} else {
			user_print("hvos: command not found\n");
		}
	}

	exit(7);
}