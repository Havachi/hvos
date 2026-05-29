#ifndef HVOS_STRING_H
#define HVOS_STRING_H
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

extern char buffer[33];
char *itoa(int num, char* str, int radix);
char *litoa(uint64_t num, char* str, int radix);
int		kstrcmp(const char *s1, const char *s2);
int		kstrncmp(const char *s1, const char *s2, register size_t n);
void	*kmemcpy(void *restrict dest, const void *restrict src, size_t n);
size_t kstrlen(const char *str);
size_t kstrnlen(const char *str, const size_t n);
char *kstrncpy(char *s1, const char *s2, register size_t n);
char *kstrcat(char *dest, const char *src);
void		*kmemset(void *s, int c, size_t n);
/* kernel/arch/x86_64/string_.asm */
extern void		*kmemmove(void *dest, const void *src, size_t n);
extern int		kmemcmp(const void *s1, const void *s2, size_t n);
extern char		*kstrcpy(char* dest, const char* src);
#endif