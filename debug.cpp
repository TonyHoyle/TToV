#include <stdio.h>
#include <stdarg.h>
#include "debug.h"

void debug(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
}