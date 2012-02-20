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
