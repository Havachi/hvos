

#ifndef HVOS_USER_H
#define HVOS_USER_H

#include <stdint.h>
#include <stdlib.h>


#ifndef HVOS_VERSION_MAJOR
#define HVOS_VERSION_MAJOR 0
#endif
#ifndef HVOS_VERSION_MINOR
#define HVOS_VERSION_MINOR 1
#endif

#define CANCEL 0x18
#define CLEAR  0x02

typedef struct cmd_s {
	char **tokens;
	uint32_t nb_tokens;
} cmd_t;

void yield(void);

// Basic userland utility
void user_print(const char* message);
void user_printerr(const char *str);
void clear_screen();


#endif