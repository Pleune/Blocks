#include "debug.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void
fail(const char *s, ... )
{
	fprintf(stderr, "[FATAL]: ");
	va_list argptr;
	va_start(argptr, s);
	vfprintf(stderr, s, argptr);
	va_end(argptr);
	exit(-1);
	fputc('\n', stderr);
}

void
error(const char *s, ... )
{
	fprintf(stderr, "[ERROR]: ");
	va_list argptr;
	va_start(argptr, s);
	vfprintf(stderr, s, argptr);
	va_end(argptr);
	fputc('\n', stderr);
}

void
warn(const char *s, ... )
{
	fprintf(stderr, "[WARNING]: ");
	va_list argptr;
	va_start(argptr, s);
	vfprintf(stderr, s, argptr);
	va_end(argptr);
	fputc('\n', stderr);
}

void
info(const char *s, ... )
{
	fprintf(stderr, "[INFO]: ");
	va_list argptr;
	va_start(argptr, s);
	vfprintf(stderr, s, argptr);
	va_end(argptr);
	fputc('\n', stderr);
}
