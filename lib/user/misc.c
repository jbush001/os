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


