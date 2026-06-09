#include "kernel/file.h"
#include "kernel/vfs.h"

struct fd fdget_pos(unsigned int fd) {

	struct fd f = fdget(fd);
	file_t *file = fd_file(f);
	return f;
}