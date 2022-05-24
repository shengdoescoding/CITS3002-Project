#ifndef verbose_h
#define verbose_h

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

void set_verbose(bool setting);
int verbose(const char * restrict, ...);

#endif