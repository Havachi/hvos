#include "hvos.h"

static char input_buffer[128];
static char history_buffer[10][128];
static int index = 0;
static int history_index = 0;

static int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static char *strcpy(const char* s1, char* s2) {
    while (*s1) {
		*s2++ = *s1++;
	}
	return s2;
}

char prompt[7] = "hvos> ";

void clear_buffer() {
	while (index > 0) {
		input_buffer[index--] = '\0';
	}
	input_buffer[0] = '\0';
}


void _start(void) {
	user_print("\nWelcome to HVOS 0.1\n");
	while (1) {
		user_print(prompt);
		index = 0;
		while (1) {
			char c;
			read(0, &c, 1);
			if (c == '\n') {
				user_print("\n");
				strcpy(history_buffer[history_index++], input_buffer); 
				break;
			} else if (c == '\b') {
				if (index > 0) {
					index--;
					input_buffer[index] = '\0';
					user_print("\b");
				}
			} else if(index < 127) {
				input_buffer[index++] = c;
				write(1, &c, 1);
			}
		}
		if (index == 0) continue;
		if (strcmp(input_buffer, "help") == 0) {
			user_print("Help menu !\n");
		} else {
			user_printerr("hvsh: command not found: ");
			user_printerr(input_buffer);
			user_printerr("\n");
		}
		clear_buffer();
	}

	exit(7);
}