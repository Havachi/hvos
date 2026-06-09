#ifndef _HVOS_COMPILER_H
#define _HVOS_COMPILER_H

#ifndef __same_type
# define __same_type(a, b) __builtin_types_compatible_p(typeof(a), typeof(b))
#endif

#ifdef __CHECKER__
#define __BUILD_BUG_ON_ZERO_MSG(e, msg, ...) (0)
#else
#define __BUILD_BUG_ON_ZERO_MSG(e, msg, ...) ((int)sizeof(struct {_Static_assert(!(e), msg) }))
#endif

#define BUILD_BUG_ON_ZERO(e, ...) \
	__BUILD_BUG_ON_ZERO_MSG(e, ##__VA_ARGS__, #e " is true")

#endif