#include "cpu_asm.h"
#include "Lock.h"
#include "memory_layout.h"
#include "stdio.h"
#include "stdlib.h"

struct HeapBin {
	Mutex lock;
	size_t elementSize;
	int growSize;
	int allocCount;
	void *freeList;
	int freeCount;
	char *rawList;
	int rawCount;
};

struct HeapPage {
	unsigned short binIndex : 5;
	unsigned short freeCount : 9;
	unsigned short cleaning : 1;
	unsigned short inUse : 1;
} PACKED;

static char* RawAlloc(int size, int binindex);

static volatile unsigned int brk = kHeapBase;
static HeapPage pageDescriptors[(kHeapTop - kHeapBase) / PAGE_SIZE];
static HeapBin bins[] = {
	{0, 16, PAGE_SIZE, 0, 0, 0, 0, 0},
	{0, 32, PAGE_SIZE, 0, 0, 0, 0, 0},
	{0, 44, PAGE_SIZE, 0, 0, 0, 0, 0},
	{0, 64, PAGE_SIZE, 0, 0, 0, 0, 0},
	{0, 89, PAGE_SIZE, 0, 0, 0, 0, 0},
	{0, 128, PAGE_SIZE, 0, 0, 0, 0, 0},
	{0, 256, PAGE_SIZE, 0, 0, 0, 0, 0},
	{0, 315, PAGE_SIZE, 0, 0, 0, 0, 0},
	{0, 512, PAGE_SIZE, 0, 0, 0, 0, 0},
	{0, 1024, PAGE_SIZE, 0, 0, 0, 0, 0},
	{0, 2048, PAGE_SIZE, 0, 0, 0, 0, 0},
	{0, 0x1000, 0x1000, 0, 0, 0, 0, 0},
	{0, 0x2000, 0x2000, 0, 0, 0, 0, 0},
	{0, 0x3000, 0x3000, 0, 0, 0, 0, 0},
	{0, 0x4000, 0x4000, 0, 0, 0, 0, 0},
	{0, 0x5000, 0x5000, 0, 0, 0, 0, 0},
	{0, 0x6000, 0x6000, 0, 0, 0, 0, 0},
	{0, 0x7000, 0x7000, 0, 0, 0, 0, 0},
	{0, 0x8000, 0x8000, 0, 0, 0, 0, 0},
	{0, 0x9000, 0x9000, 0, 0, 0, 0, 0},
	{0, 0xa000, 0xa000, 0, 0, 0, 0, 0}
};

const int kBinCount = sizeof(bins) / sizeof(HeapBin);

void* malloc(size_t size)
{
	void *address = 0;
	int binIndex;
	for (binIndex = 0; binIndex < kBinCount; binIndex++)
		if (size <= bins[binIndex].elementSize)
			break;

	if (binIndex == kBinCount)
		address = RawAlloc(size, binIndex);
	else {
		HeapBin &bin = bins[binIndex];
		bin.lock.Lock();
		if (bin.freeList) {
			address = bin.freeList;
			bin.freeList = *reinterpret_cast<void**>(bin.freeList);
			bin.freeCount--;
		} else {
			if (bin.rawCount == 0) {
				bin.rawList = RawAlloc(bin.growSize, binIndex);
				bin.rawCount = bin.growSize / bin.elementSize;
			}

			bin.rawCount--;
			address = bin.rawList;
			bin.rawList += bin.elementSize;
		}
		
		bin.allocCount++;
		HeapPage *pages = &pageDescriptors[(reinterpret_cast<unsigned int>(address) - kHeapBase) / PAGE_SIZE];
		for (unsigned int index = 0; index < bin.elementSize / PAGE_SIZE; index++)
			pages[index].freeCount--;

		bin.lock.Unlock();
	}

	return address;
}

void free(void *address)
{
	if (address == 0)
		return;

	HeapPage *pages = &pageDescriptors[((unsigned int) address - kHeapBase) / PAGE_SIZE];
	HeapBin &bin = bins[pages[0].binIndex];
	bin.lock.Lock();
	for (unsigned int index = 0; index < bin.elementSize / PAGE_SIZE; index++)
		pages[index].freeCount++;

	*reinterpret_cast<void**>(address) = bin.freeList;
	bin.freeList = address;
	bin.allocCount--;
	bin.freeCount++;
	bin.lock.Unlock();
}

void PrintHeapBins(int, const char*[])
{
	printf("    Size   In Use     Free      Raw\n");
	for (int binIndex = 0; binIndex < kBinCount; binIndex++)
		printf("%8lu %8d %8d %8d\n", bins[binIndex].elementSize, bins[binIndex].allocCount,
			bins[binIndex].freeCount, bins[binIndex].rawCount);
}

void PrintHeapPages(int, const char*[])
{
	printf("Address       Element Size   Free\n");      
	for (unsigned int i = 0; i < (kHeapTop - kHeapBase) / PAGE_SIZE; i++) {
		if (!pageDescriptors[i].inUse)
			continue;

		if (pageDescriptors[i].binIndex < kBinCount)
			printf("%08x         %5d      %5d\n", i * PAGE_SIZE + kHeapBase,
				static_cast<int>(bins[pageDescriptors[i].binIndex].elementSize),
				pageDescriptors[i].freeCount);
		else
			printf("%08x            xx         xx\n", i * PAGE_SIZE + kHeapBase);
	}
}

char* RawAlloc(int size, int binIndex)
{
	unsigned int val = static_cast<unsigned int>(AtomicAdd(reinterpret_cast<volatile int*>(&brk), (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1)));
	if (val > kHeapTop)
		panic("Out of heap space");

	for (unsigned int addr = val; addr < val + size; addr += PAGE_SIZE) {
		HeapPage &page = pageDescriptors[(addr - kHeapBase) / PAGE_SIZE];
		page.inUse = true;
		page.cleaning = false;
		page.binIndex = binIndex;
		if (binIndex < kBinCount && bins[binIndex].elementSize < PAGE_SIZE)
			page.freeCount = PAGE_SIZE / bins[binIndex].elementSize;
		else
			page.freeCount = 1;
	}
			
	return reinterpret_cast<char*>(val);
}

void GarbageCollectBin(HeapBin &bin)
{
	for (void **link = &bin.freeList; *link != 0;) {
		HeapPage &page = pageDescriptors[(reinterpret_cast<unsigned int>(*link) - kHeapBase)
			/ PAGE_SIZE];
		if (page.cleaning) {
			*link = **reinterpret_cast<void***>(link);
			if (--page.freeCount == 0) {
				// Free this page...
			}
		} else if (page.freeCount == PAGE_SIZE / bin.elementSize) {
			*link = **reinterpret_cast<void***>(link);
			page.cleaning = true;
		} else
			link = *reinterpret_cast<void***>(link);
	}
}
