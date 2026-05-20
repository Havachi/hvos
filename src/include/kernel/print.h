#ifndef HVOS_PRINT_H
#define HVOS_PRINT_H
#include "klibc/printf.h"

#ifndef VERBOSITY
#define VERBOSITY 10
#endif


void printverbose(int verbosity, char *str, ...);

#endif