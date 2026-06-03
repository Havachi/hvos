

#ifndef HVOS_USER_H
#define HVOS_USER_H

#include <stdint.h>
#include "liballoc.h"

#define SC_READ			0
#define SC_WRITE		1
#define SC_OPEN			2
#define SC_CLOSE		3
#define SC_STAT			4
#define SC_MMAP			9
#define SC_MUNMAP		11
#define SC_YIELD		24
#define SC_FORK			57
#define SC_EXECVE		59
#define SC_EXIT			60
#define SC_UPRINT		99

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