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
