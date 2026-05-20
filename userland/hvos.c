#include "hvos.h"

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

void exit(int status) {
	asm volatile("syscall" :: "a"(SC_EXIT), "D"(status) : "rcx", "r11", "memory");
	for(;;);
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

void user_print(const char *str) {
	int len = 0;
	while (str[len]) len++;
	write(1, str, len);
}
