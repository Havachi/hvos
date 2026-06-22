#ifndef HVOS_SYSCALL_H
#define HVOS_SYSCALL_H

#include "kernel/scheduler/mt.h"
#include "linkage.h"
#include "compiler.h"
#include <stdint.h>
#include "hvos/stringify.h"

#define MAX_FD          16
#define MSR_STAR		0xC0000081
#define MSR_LSTAR		0xC0000082
#define MSR_SFMASK		0xC0000084
#define MSR_EFER        0xC0000080
#define MSR_GS_BASE		0xC0000101
#define MSR_KERNEL_GS_BASE	0xC0000102


#define __MAP0(m,...)
#define __MAP1(m,t,a)		m(t,a)
#define __MAP2(m,t,a,...)	m(t,a), __MAP1(m,__VA_ARGS__)
#define __MAP3(m,t,a,...)	m(t,a), __MAP2(m,__VA_ARGS__)
#define __MAP4(m,t,a,...)	m(t,a), __MAP3(m,__VA_ARGS__)
#define __MAP5(m,t,a,...)	m(t,a), __MAP4(m,__VA_ARGS__)
#define __MAP6(m,t,a,...)	m(t,a), __MAP5(m,__VA_ARGS__)
#define __MAP(n,...) __MAP##n(__VA_ARGS__)

#define __SC_DECL(t, a) t a
#define __TYPE_IS_L(t) (__same_type((t)0, 0L))
#define __TYPE_IS_UL(t) (__same_type((t)0, 0UL))
#define __TYPE_IS_LL(t) (__same_type((t)0, 0LL) || __same_type((t)0, 0ULL))
#define __SC_LONG(t, a) __typeof(__builtin_choose_expr(__TYPE_IS_LL(t), 0LL, 0L)) a
#define __SC_CAST(t, a) (t) a
#define __SC_ARGS(t, a) a
#define __SC_TEST(t, a) (void)BUILD_BUG_ON_ZERO(!__TYPE_IS_LL(t) && sizeof(t) > sizeof(long))

#define SYSCALL_METADATA(sname, nb, ...)

#define SYSCALL_DEFINE0(sname) \
	SYSCALL_METADATA(_##sname, 0); \
	asmlinkage long sys_##sname(void)

#define SYSCALL_DEFINE1(name, ...) SYSCALL_DEFINEx(1, _##name, __VA_ARGS__)
#define SYSCALL_DEFINE2(name, ...) SYSCALL_DEFINEx(2, _##name, __VA_ARGS__)
#define SYSCALL_DEFINE3(name, ...) SYSCALL_DEFINEx(3, _##name, __VA_ARGS__)
#define SYSCALL_DEFINE4(name, ...) SYSCALL_DEFINEx(4, _##name, __VA_ARGS__)
#define SYSCALL_DEFINE5(name, ...) SYSCALL_DEFINEx(5, _##name, __VA_ARGS__)
#define SYSCALL_DEFINE6(name, ...) SYSCALL_DEFINEx(6, _##name, __VA_ARGS__)

#define SYSCALL_DEFINEx(x, sname, ...) \
	SYSCALL_METADATA(sname, x, __VA_ARGS__) \
	__SYSCALL_DEFINEx(x, sname, __VA_ARGS__)

#define __PROTECT(...) asmlinkage_protect(__VA_ARGS__)
#define __SYSCALL_DEFINEx(x, name, ...)					\
	asmlinkage long sys##name(__MAP(x,__SC_DECL,__VA_ARGS__))	\
		__attribute__((alias(__stringify(SyS##name))));		\
	static inline long SYSC##name(__MAP(x,__SC_DECL,__VA_ARGS__));	\
	asmlinkage long SyS##name(__MAP(x,__SC_LONG,__VA_ARGS__));	\
	asmlinkage long SyS##name(__MAP(x,__SC_LONG,__VA_ARGS__))	\
	{								\
		long ret = SYSC##name(__MAP(x,__SC_CAST,__VA_ARGS__));	\
		__MAP(x,__SC_TEST,__VA_ARGS__);				\
		__PROTECT(x, ret,__MAP(x,__SC_ARGS,__VA_ARGS__));	\
		return ret;						\
	}		\
	static inline long SYSC##name(__MAP(x,__SC_DECL,__VA_ARGS__))




asmlinkage long sys_read(unsigned int fd, char *buf, size_t count);
asmlinkage long sys_write(unsigned int fd, const char *buf, size_t count);


extern void wrmsr(uint32_t, uint64_t);
extern uint64_t rdmsr(uint32_t msr);
extern void syscall_entry_asm(void);

void init_syscall(void);
void sys_print(const char* str);
void syscall_handler(pt_regs_t *frame);

#endif
