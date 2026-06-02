#include "hvos.h"
#include <stdint.h>

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

void clear_screen() {
	int res; 
	char buf[2] = {CLEAR, 0x00 };
    asm volatile("syscall" : "=a"(res) : "a"(SC_WRITE), "D"(1), "S"(buf), "d"(2) : "rcx", "r11", "memory");
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

void *user_alloc_page(uint32_t pages) {
	uint64_t addr;
	asm volatile("syscall" : "=a"(addr) : "a"(SC_MMAP), "S"(pages): "memory");
}

void user_free_pages(void *ptr, uint32_t pages) {
	int res;
	asm volatile("syscall" : "=a"(res) : "a"(SC_MUNMAP), "D"(ptr), "S"(pages): "memory");
}


int liballoc_lock() {
	return 0;
}

int liballoc_unlock() {
	return 0;
}

void* liballoc_alloc(int size) {
	return (void *)user_alloc_page(size);
}

int liballoc_free(void* ptr ,int size) {
	user_free_pages(ptr, size);
	return 0;
}