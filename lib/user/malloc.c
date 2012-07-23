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

#include <types.h>
#include <syscall.h>
#include <stdio.h>

static void lock_heap(void);
static void unlock_heap(void);
extern void __sbrk_init(void);
extern void* sbrk(int diff);
extern void* __malloc_internal(size_t);
extern void __free_internal(void*);

static int lock_count;
static int heap_lock = -1;

void __heap_init(void)
{
	__sbrk_init();
	heap_lock = create_sem("heap lock", 1);
}

void* malloc(size_t size)
{
	void *address = 0;
	
	lock_heap();
	address = __malloc_internal(size);
	unlock_heap();

	return address;
}

void free(void *address)
{
	lock_heap();	
	__free_internal(address);	
	unlock_heap();
}

static void lock_heap(void)
{
	if (atomic_add(&lock_count, 1) >= 1)
		acquire_sem(heap_lock, INFINITE_TIMEOUT);
}

static void unlock_heap(void)
{
	if (atomic_add(&lock_count, -1) > 1)
		release_sem(heap_lock, INFINITE_TIMEOUT);
}
