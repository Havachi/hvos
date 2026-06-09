#include "hvos.h"
#include <stdint.h>
#include <syscall-ids.h>

int read(int fd, void *buffer, uint32_t size) {
	int read_bytes;
	__asm__ volatile (
		"syscall"
		: "=a"(read_bytes)
		: "a"(SC_READ), "D"(fd), "S"(buffer), "d"(size)
		: "rcx", "r11", "memory"
	);
	return read_bytes;
}

int write(int fd, const void* buf, uint32_t size) {
	int res; 
    __asm__ volatile("syscall" : "=a"(res) : "a"(SC_WRITE), "D"(fd), "S"(buf), "d"(size) : "rcx", "r11", "memory");
	return res;
}

int open(const char* path){
	int fd;
	__asm__ volatile(
		"syscall"
		: "=a"(fd)
		: "a"(SC_OPEN), "D"(path)
		: "rcx", "r11", "memory"
	);
	return fd;
}

void exit(int status) {
	__asm__ volatile("syscall" :: "a"(SC_EXIT), "D"(status) : "rcx", "r11", "memory");
	for(;;);
}

int exec(const char* path) {
	int result;
	__asm__ volatile (
		"syscall"
		: "=a" (result)
		: "a"(SC_EXECVE), "D"(path)
		: "rcx", "r11", "memory"
	);
	return result;
}

void clear_screen() {
	int res; 
	char buf[2] = {CLEAR, 0x00 };
    __asm__ volatile("syscall" : "=a"(res) : "a"(SC_WRITE), "D"(1), "S"(buf), "d"(2) : "rcx", "r11", "memory");
}

void user_print(const char *str) {
	int len = 0;
	while (str[len]) len++;
	write(1, str, len);
}

void user_printerr(const char *str) {
	int len = 0;
	while (str[len]) len++;
	write(2, str, len);
}