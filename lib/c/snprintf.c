#include <stdio.h>

void snprintf(char *out, int size, const char *fmt, ...)
{
	va_list arglist;

	VA_START(arglist, fmt);
	vsnprintf(out, size, fmt, arglist);
}
