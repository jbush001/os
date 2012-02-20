
#include <string.h>
#include <types.h>
#include <syscall.h>
#include <stdio.h>

#define MINIMUM_BLOCK_SIZE 32
#define BRK_SIZE 0x2000
#define MALLOCED_HEADER_SIZE 8
#define FREE_MAGIC 0xdeadbeef
#define MALLOCED_MAGIC 0xfee1dea1
#define ALLOC_FILL 0xd7
#define UNALLOCED_FILL 0xd9
#define FREE_FILL 0xd2

#define assert(x)

#define MERGE_BLOCKS(block1, block2)	{				\
	assert(block1->next = block2);						\
	assert(block2->prev = block1);						\
	block2->next->prev = block2->prev;					\
	block2->prev->next = block2->next;					\
	block1->size += block2->size; }

struct block_header {
	size_t size;		/* including headers */
	unsigned magic;
	struct block_header *next;
	struct block_header *prev;
};

static void* find_free_block(size_t);
static void insert_free_block(struct block_header *header);
static void grow_heap(size_t min_size);
void dump_heap(void);
extern void* sbrk(int diff);

static struct block_header free_list = {0, 0, &free_list, &free_list};	/* This is a dummy node */
static int allocated_blocks = 0;
static int total_allocations = 0;
static int total_space_used = 0;
static int total_allocated = 0;

void* __malloc_internal(size_t size)
{
	size_t rounded_size;
	size_t shifted_size;
	void *address;

	/* Round size to a power of two */
	rounded_size = MINIMUM_BLOCK_SIZE;
	for (shifted_size = (size + MALLOCED_HEADER_SIZE) / MINIMUM_BLOCK_SIZE;
		shifted_size; shifted_size >>= 1)
		rounded_size <<= 1;

	assert(rounded_size >= size + MALLOCED_HEADER_SIZE);

	address = find_free_block(rounded_size);
	if (address == 0) {
		grow_heap(rounded_size);
		address = find_free_block(rounded_size);
		if (address == 0)
			return 0;
	}

	memset(address, ALLOC_FILL, rounded_size - MALLOCED_HEADER_SIZE);

	allocated_blocks++;
	total_allocations++;
	total_allocated += rounded_size;

	return address;
}

void __free_internal(void *address)
{
	struct block_header *header;

	if (address == 0)
		return;

	assert(allocated_blocks > 0);
	header = (struct block_header*) ((unsigned) address - MALLOCED_HEADER_SIZE);
	assert(header->magic == MALLOCED_MAGIC)
	memset((void*)((unsigned) header + sizeof(struct block_header)), FREE_FILL,
		header->size - sizeof(struct block_header));
	header->magic = FREE_MAGIC;

	allocated_blocks--;
	total_allocated -= header->size;
	insert_free_block(header);
}

static void* find_free_block(size_t size)
{
	void *address = 0;
	struct block_header *block, *new_block;

	for (block = free_list.next; block != &free_list; block = block->next) {
		if (block->size == size) {
			assert(block->magic == FREE_MAGIC)
			block->magic = MALLOCED_MAGIC;
			block->next->prev = block->prev;
			block->prev->next = block->next;
			address = (void*)((unsigned) block + MALLOCED_HEADER_SIZE);
			break;
		} else if (block->size > size) {
			assert(block->magic == FREE_MAGIC)

			/* carve a chunk from the end of block */
			block->size -= size;
			new_block = (struct block_header*)((unsigned) block + block->size);
			new_block->size = size;
			new_block->magic = MALLOCED_MAGIC;
			address = (void*)((unsigned) new_block + MALLOCED_HEADER_SIZE);
			break;
		}
	}

	return address;
}

static void insert_free_block(struct block_header *new_block)
{
	struct block_header *block, *next_header, *previous_header;

	if (free_list.next == &free_list || new_block < free_list.next) {
		/* Insert as first element in heap */
		new_block->next = free_list.next;
		new_block->prev = &free_list;
		new_block->next->prev = new_block;
		new_block->prev->next = new_block;
	} else {
		/* Walk heap and insert in proper place */
		for (block = free_list.next;; block = block->next) {
			assert(new_block != block);
			if (block->next == &free_list || (new_block > block
				&& new_block < block->next)) {
				new_block->next = block->next;
				new_block->prev = block;
				new_block->next->prev = new_block;
				new_block->prev->next = new_block;
				break;
			}
		}
	}

	/* Coalesce with next block */
	next_header = new_block->next;
	if (next_header != &free_list
		&& ((unsigned) new_block + new_block->size == (unsigned) next_header)) {
		assert(next_header->magic == FREE_MAGIC);
		MERGE_BLOCKS(new_block, next_header);
	}

	assert(new_block->magic == FREE_MAGIC);

	/* Coalesce with previous block */
	previous_header = new_block->prev;
	if (previous_header != &free_list
		&& (unsigned) new_block->prev + new_block->prev->size ==
		(unsigned) new_block) {
		assert(previous_header->magic == FREE_MAGIC);
		MERGE_BLOCKS(previous_header, new_block);
	}
}

void grow_heap(size_t min_size)
{
	struct block_header *block;
	size_t grow_size;
	void *new_space;

	grow_size = ((min_size + BRK_SIZE - 1) / BRK_SIZE) * BRK_SIZE;

	new_space = sbrk(grow_size);
	memset((void*) new_space, UNALLOCED_FILL, grow_size);
	block = (struct block_header*) new_space;
	block->size = grow_size;
	block->magic = FREE_MAGIC;
	insert_free_block(block);

	total_space_used += grow_size;
}

void dump_heap(void)
{
	struct block_header *header;

	printf("Heap\n");
	printf("Blocks currently allocated: %d\n", allocated_blocks);
	printf("Total allocations: %d\n", total_allocations);
	printf("Total memory allocated: %d\n", total_allocated);
	printf("Total memory used: %d\n", total_space_used);
	printf("Free Block List:\n");
	printf("    Address  Size\n");
	for (header = free_list.next; header != &free_list; header = header->next)
		printf("    %08x %08x\n", (unsigned) header, (unsigned) header->size);

	printf("\n");
}
