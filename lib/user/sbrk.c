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

static int heap_area = -1;
static char *heap_base = 0;
static int heap_size = 0;

void __sbrk_init(void)
{
	heap_area = create_area("Heap", (void**) &heap_base, SEARCH_FROM_BOTTOM,
		PAGE_SIZE, AREA_NOT_WIRED, USER_READ | USER_WRITE | SYSTEM_READ
		| SYSTEM_WRITE);
}

void *sbrk(int diff)
{
	char *retval;
	
	if (diff == 0)
		return heap_base + heap_size;
			
	if (resize_area(heap_area, heap_size + diff) < 0)
		return 0;

	retval = heap_base + heap_size;
	heap_size += diff;
	return retval;
}
