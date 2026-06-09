#ifndef __HVOS_FDTABLE_H
#define __HVOS_FDTABLE_H

#include "compiler.h"
#include "vfs.h"

typedef struct fdtable_s {
	unsigned int		max_fds;
	file_t				**fd;
	unsigned long		*close_on_exec;
	unsigned long		*open_fds;
	struct fdtable_s	*next;
} fdtable_t;

#endif