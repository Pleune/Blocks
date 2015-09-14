#ifndef DEBUG_H
#define DEBUG_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static inline void fail(const char *s, ... )
{
	fprintf(stderr, "FAIL: ");
	va_list argptr;
	va_start(argptr, s);
	vfprintf(stderr, s, argptr);
	va_end(argptr);
	exit(-1);
	fputc('\n', stderr);
}

static inline void info(const char *s, ... )
{
	fprintf(stderr, "INFO: ");
	va_list argptr;
	va_start(argptr, s);
	vfprintf(stderr, s, argptr);
	va_end(argptr);
	fputc('\n', stderr);
}

#endif
