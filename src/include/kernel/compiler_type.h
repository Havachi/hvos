
#ifndef __HVOS_COMPILER_TYPES_H
#define __HVOS_COMPILER_TYPES_H

#ifndef __has_builtin
#define __has_builtin(x) (0)
#endif

#define ___PASTE(a, b) a##b
#define __PASTE(a, b) ___PASTE(a, b)

#ifndef __ASSEMBLY__

#if __STDC_VERSION__ < 202311L
#define auto __auto_type
#endif

#define BFT_TYPE_TAG(value)

#ifdef __CHECKER__
# define __kernel __attribute__((address_space(0)))
# define __user __attribute__((noderef, address_space(__user)))
# define __iomem __attribute__((noderef, address_space(__iomem)))
# define __percpu __attribute__((noderef, address_space(__percpu)))
# define __rcu __attribute__((noderef, address_space(__rcu)))

# define __force	__attribute__((force))
# define __nocast	__attribute__((nocast))
# define __safe		__attribute__((safe))
# define __private	__attribute__((noderef))
#else /* __CHECKER__ */

# define __kernel
# ifdef STRUCTLEAK_PLUGIN
#  define __user	__attribute__((user))
# else
#  define __user	BTF_TYPE_TAG(user)
# endif
# define __iomem
# define __percpu	__percpu_qual BTF_TYPE_TAG(percpu)
# define __rcu		BTF_TYPE_TAG(rcu)
# define __chk_user_ptr(x)	(void)0
# define __chk_io_ptr(x)	(void)0
# define __force
# define __nocast
# define __safe
# define __private

#endif /* __CHECKER__ */


#endif /* __ASSEMBLY__ */
#endif /* __HVOS_COMPILER_TYPES_H */