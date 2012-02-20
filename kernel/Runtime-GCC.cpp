//
//	C++ Runtime Support
//

#include "BootParams.h"
#include "KernelDebug.h"
#include "stdlib.h"
#include "types.h"

struct nothrow_t {} nothrow;
typedef void (*Initializer)();

extern "C" void _start(const BootParams*) NORETURN;
extern "C" int main() NORETURN;

extern const Initializer __CTOR_LIST__[];
BootParams bootParams;

//
//	This is the main entry point for the kernel
//
void _start(const BootParams *params)
{
	// Call global constructors
	for (const Initializer *initializer = __CTOR_LIST__; *initializer; initializer++)
		 (*initializer)();

	bootParams = *params;
	main();
}

void* operator new(size_t size)
{
	return malloc(size);
}

void* operator new[] (size_t size)
{
	return malloc(size);
}

void operator delete(void *ptr)
{
	free(ptr);
}

void operator delete[] (void *ptr)
{
	free(ptr);
}

extern "C" void __pure_virtual()
{
	panic("pure virtual function call");
}

extern "C" void abort()
{
	panic("abort()");
}
