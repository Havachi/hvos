#include "klibc/string.h"
#include "klibc/printf.h"
#include <stddef.h>
#include <stdint.h>
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
	const char *end = str;
	if (str == NULL)
		return 0;
	while (*end != '\0') {
		++end;
	}
	return end - str;
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

char *kstrcat(char *dest, const char *src) {
	char *end = (char *)dest + kstrlen(dest);
	kstrncpy(end, src, kstrlen((char *)src));
	end = end + kstrlen((char *)src);
	*end = '\0';
	return dest;
}

char *kstrrchr(const char *s, int c) {
	char *last = NULL;
	char target = (char)c;
	do {
		if (*s == target){
			last = (char *)s;
		}
	} while(*s++);
	return last;
}


void *kmemset(void *s, int c, size_t n) {
	uint8_t *sb = (uint8_t *)s;
	uint8_t bc = (uint8_t )c;

	while (n > 0 && ((uintptr_t)sb & 7) != 0) {
		*sb++ = bc;
		n--;
	}

	uint64_t wv = bc;
	wv |= (wv << 8);
	wv |= (wv << 16);
	wv |= (wv << 32);
	uint64_t* sw = (uint64_t *)sb;
	while(n >= 32) {
		sw[0] = wv;
		sw[1] = wv;
		sw[2] = wv;
		sw[3] = wv;
		sw += 4;
		n -= 32;
	}

	while (n >= 8) {
		*sw++ = wv;
		n -= 8;
	}

	sb = (uint8_t *)sw;
	while (n > 0) {
		*sb++ = bc;
		n--;
	}

	return s;
}

