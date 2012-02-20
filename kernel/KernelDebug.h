#ifndef _KERNEL_DEBUG_H
#define _KERNEL_DEBUG_H

#include "types.h"

#if DEBUG
	#ifdef __GNUC__
		#define ASSERT(x) \
			if (!(x)) { __assert_failed(#x, __PRETTY_FUNCTION__, __FILE__, __LINE__);  }
	#else
		#define ASSERT(x) \
			if (!(x)) { __assert_failed(#x, "", __FILE__, __LINE__);  }
	#endif	
#else
	#define ASSERT(x)
#endif

typedef void (*DebugHook)(int argc, const char **argv);

void KernelDebugBootstrap();
void panic(const char *fmt, ...) NORETURN;
void Debugger();
void AddDebugCommand(const char name[], const char description[], DebugHook fn);
void RemoveDebugCommand(int (*hook)(int, const char*[]));
void bindump(const char data[], int size);
void __assert_failed(const char[], const char[], const char[], int) NORETURN;

#endif
