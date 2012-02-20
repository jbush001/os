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
