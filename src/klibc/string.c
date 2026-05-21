#include "klibc/string.h"
#include "klibc/printf.h"
#include <stddef.h>
#include <string.h>

char buffer[33] = {0};

char *itoa(int num, char* str, int radix) {
	char base[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int si = 0;
	int n = num;
	
	// Handle negative numbers for base 10
	if (n < 0 && radix == 10) {
		str[si++] = '-';
		n = -n;
	}
	// Convert number to string
	char tmp[33]; // Sufficient for 32-bit int + null
	int rdi = -1;
	while (n) {
		tmp[++rdi] = base[n % radix];
		n /= radix;
	}
	// Special case for 0
	if (rdi == -1) {
		tmp[++rdi] = '0';
	}
	// Reverse and add to buffer
	while (rdi >= 0) {
		str[si++] = tmp[rdi--];
	}
	str[si] = '\0'; // Null-terminate
	return str;
}

char *litoa(uint64_t num, char* str, int radix) {
	char base[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int si = 0;
	uint64_t n = num;
	
	// Handle negative numbers for base 10
	if (n < 0 && radix == 10) {
		str[si++] = '-';
		n = -n;
	}
	// Convert number to string
	char tmp[33]; // Sufficient for 32-bit int + null
	int rdi = -1;
	while (n) {
		tmp[++rdi] = base[n % radix];
		n /= radix;
	}
	// Special case for 0
	if (rdi == -1) {
		tmp[++rdi] = '0';
	}
	// Reverse and add to buffer
	while (rdi >= 0) {
		str[si++] = tmp[rdi--];
	}
	str[si] = '\0'; // Null-terminate
	return str;
}

void	*kmemcpy(void *restrict dest, const void *restrict src, size_t n) {
	unsigned char *d = (unsigned char *)dest;
	const unsigned char *s = (const unsigned char *)src;

	size_t chunks = n / 8;
	uint64_t *d64 = (uint64_t *)d;
	const uint64_t *s64 = (const uint64_t *)s;

	while (chunks--) {
		*d64++ = *s64++;
	}

	d = (unsigned char *)d64;
	s = (unsigned char *)s64;

	size_t remainder = n % 8;
	while (remainder--) {
		*d++ = *s++;
	}

	return dest;
}

size_t kstrlen(const char *str){
	if ((uint64_t)str == 0x0A || (uint64_t)str < 0x1000) {
        uint64_t *rbp_ptr;
        asm volatile("mov %%rbp, %0" : "=r"(rbp_ptr));
        for(;;);
    }
	size_t len = 0;
	while (str[len++])
		;
	return len;
}

size_t kstrnlen(const char *str, const size_t n){
	size_t len = 0;
	while (str[len++] && len < n)
		;
	return len;
}

int kstrcmp(const char *s1, const char *s2) {
	const unsigned char *p1 = (const unsigned char *)s1;
	const unsigned char *p2 = (const unsigned char *)s2;

	while (*p1 && (*p1 == *p2)) {
		p1++;
		p2++;
	}

	return *p1 - *p2;
}

int kstrncmp(const char *s1, const char *s2, register size_t n) {
	register unsigned char u1, u2;

	while (n-- > 0)
	{
		u1 = (unsigned char) *s1++;
		u2 = (unsigned char) *s2++;
		if (u1 != u2)
	return u1 - u2;
		if (u1 == '\0')
	return 0;
	}
	return 0;
}

char *kstrncpy(char *s1, const char *s2, register size_t n) {
	size_t len = kstrnlen(s2, n);
	kmemcpy(s1, s2, len);
	if (len < n)
		kmemset(s1 +len, '\0', n - len);
	return s1;
}