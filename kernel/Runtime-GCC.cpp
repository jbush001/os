// 
// Copyright 1998-2012 Jeff Bush
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// 

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
