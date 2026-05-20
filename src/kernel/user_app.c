#include <stdint.h>
#include "kernel/syscall_id.h"

void exit(int status) {
	asm volatile("syscall" :: "a"(SC_EXIT), "D"(status) : "rcx", "r11", "memory");
	for(;;);
}

void yield(void) {
	asm volatile("syscall" :: "a"(SC_YIELD) : "rcx", "r11", "memory");
}

int open(const char* path){
	int fd;
	asm volatile(
		"syscall"
		: "=a"(fd)
		: "a"(SC_OPEN), "D"(path)
		: "rcx", "r11", "memory"
	);
	return fd;
}

int read(int fd, void *buffer, uint32_t size) {
	int read_bytes;
	asm volatile (
		"syscall"
		: "=a"(read_bytes)
		: "a"(SC_READ), "D"(fd), "S"(buffer), "d"(size)
		: "rcx", "r11", "memory"
	);
	return read_bytes;
}

int write(int fd, const void* buf, uint32_t size) {
	int res; 
    asm volatile("syscall" : "=a"(res) : "a"(SC_WRITE), "D"(fd), "S"(buf), "d"(size) : "rcx", "r11", "memory");
	return res;
}

void user_print(const char *str) {
	int len = 0;
	while (str[len]) len++;
	write(1, str, len);
}

int exec(const char* path) {
	int result;
	asm volatile (
		"syscall"
		: "=a" (result)
		: "a"(SC_EXECVE), "D"(path)
		: "rcx", "r11", "memory"
	);
	return result;
}



void first_user_app(void) {

	user_print("Started !\n");
	int fd = open("hello.txt");
	if (fd < 0) {
		user_print("Failed to open hello.txt\n");
		while (1);
	}

	char file_buffer[128];
	for(int i = 0; i < 128; i++) {
		file_buffer[i] = '\0';
	}

	int read_bytes = read(fd, file_buffer, 127);
	if (read_bytes > 0) {
		user_print("Successfully read hello.txt\nContent:\n");
		user_print(file_buffer);
		user_print("\n");
	} else {
		user_print("File opened, but return 0 bytes\n");
	}

	exit(42);
}