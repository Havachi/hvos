

#ifndef HVOS_USER_H
#define HVOS_USER_H

#include <stdint.h>
#include <stdlib.h>

#define CANCEL 0x18
#define CLEAR  0x02

typedef struct cmd_s {
	char **tokens;
	uint32_t nb_tokens;
} cmd_t;

int open(const char* path);
int read(int fd, void* buffer, uint32_t size);
int write(int fd, const void* buffer, uint32_t size);
int exec(const char* path);
void exit(int status);
void yield(void);

// Basic userland utility
void user_print(const char* message);
void user_printerr(const char *str);
void clear_screen();


#endif