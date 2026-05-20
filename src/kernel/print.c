#include "kernel/print.h"
#include <stdarg.h>

void printverbose(int verbosity, char *str, ...) {
	if (VERBOSITY >= verbosity){
		va_list args;
		va_start(args, str);
		kvprintf(str,args);
		va_end(args);
	}
}