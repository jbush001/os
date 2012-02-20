#include "types.h"
#include "syscall.h"

extern void __heap_init();
extern void __stdio_init();

void __syslib_init()
{
	__heap_init();
	__stdio_init();
}

int64 rdtsc()
{
	unsigned high, low;
	asm("rdtsc" : "=a" (low), "=d" (high));
	return (int64) high << 32 | low;
}

bigtime_t system_time()
{
	return rdtsc() / 150;
}


