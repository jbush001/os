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
//	Stage 2 Loader.  Load the kernel into its proper spot and execute it.
//

#include "BootParams.h"
#include "elf.h"
#include "netboot.h"

#define roundup(x, y) (((((unsigned) x) + (y) - 1) / (y)) * (y))

enum PageFlags {
	kPresent = 1,
	kWritable = 2,
	kGlobal = 256,
};

const char *kKernelImageName = "kernel-x86";
const int kPageSize = 0x1000;
const int kBootDirBase = 0x100000;
const boot_entry *kBootDir = (boot_entry*) kBootDirBase;
ushort * const kScreenBase = (ushort*) 0xb8000;
const int kScreenWidth = 80;
const int kScreenHeight = 24;

unsigned AllocatePage();
void MapPage(unsigned va, unsigned pa, bool writable);
void LoadElfImage(void *data, unsigned *outStartAddress);
void ClearScreen();
void PrintMessage(const char *msg);
void memzero(void *ptr, int count);
int strcmp(const char *str1, const char *str2);

static unsigned nextFreePage = 0;
unsigned int *pagedir;

extern "C" void _start(unsigned int mem, char*)
{
	// Basic processor setup.
	asm("movl %%eax, %%cr0" : : "a" ((1 << 31) | (1 << 16) | (1 << 5) | 1));
	asm("cld");			// Ain't nothing but a GCC thang.
	asm("fninit");
	asm("movl %%cr3, %%eax" : "=a" (pagedir));

	// Clean out kernel portion of pagedir as it usually has crap in it.
	for (int pdindex = 768; pdindex < 1024; pdindex++)
		pagedir[pdindex] = 0;

	// Account for used pages and find the kernel image.
	void *kernelImage = 0;
	nextFreePage = kBootDirBase;
	for (int entry = 0; entry < 64; entry++) {
		if (kBootDir[entry].be_type == BE_TYPE_NONE)
			break;

		if (strcmp(kBootDir[entry].be_name, kKernelImageName) == 0)
			kernelImage = (void*) nextFreePage;

		nextFreePage += kBootDir[entry].be_size  * PAGE_SIZE;
	}

	if (kernelImage == 0) {
		ClearScreen();
		PrintMessage("Couldn't find kernel");
		for (;;)
			;
	}

	unsigned kernelEntry;
	LoadElfImage(kernelImage, &kernelEntry);

	// Map in an initial kernel stack & heap
	for (unsigned va = 0xc0100000; va < 0xc0200000; va += kPageSize)
		MapPage(va, AllocatePage(), true);

	// Set up boot parameters
	BootParams *params = (BootParams*)(0xc0104000 - sizeof(BootParams));
	params->memsize = mem;

	// Set up in-use ranges for the kernel.
	params->SetAllocated((unsigned) pagedir, (unsigned) pagedir + PAGE_SIZE - 1); // page dir
	params->SetAllocated(0xa0000, nextFreePage - 1); // BIOS, Boot dir and allocated memory.

	int temp = 0;
	asm( 	"		movl	%1, %2;	"	// Get param location
			"		subl	$4, %2; "	// Subtract 4 to find top of stack
			"		andl	$~(3), %2; "// mask off low bits so the stack is aligned
			"		pushl	%2; "		// Location for stack
			"		popl 	%%esp; "	// Restore stack in new location
			"		pushl 	%1;	"		// Push location of parameters
			"		pushl 	$0;	"		// dummy retval for call to main
			"		pushl 	%0;"		// this is the start address
			"		ret;		"		// jump.
		: : "m" (kernelEntry), "m" (params), "r" (temp));
}

void LoadElfImage(void *data, unsigned *outStartAddress)
{
	Elf32_Ehdr *imageHeader = (Elf32_Ehdr*) data;
	Elf32_Phdr *segments = (Elf32_Phdr*)(imageHeader->e_phoff + (unsigned) imageHeader);
	for (int segmentIndex = 0; segmentIndex < imageHeader->e_phnum; segmentIndex++) {
		Elf32_Phdr *segment = &segments[segmentIndex];
		unsigned segmentOffset;

		/* Map initialized portion */
		for (segmentOffset = 0; segmentOffset < roundup(segment->p_filesz, kPageSize);
			segmentOffset += kPageSize) {
			MapPage(segment->p_vaddr + segmentOffset, (unsigned) data + segment->p_offset
				+ segmentOffset, segment->p_flags & PF_W);
		}

		/* Clean out the leftover part of the last page */
		memzero((void*)((unsigned) data + segment->p_offset + segment->p_filesz), kPageSize
			- (segment->p_filesz % kPageSize));

		/* Map uninitialized portion */
		for (; segmentOffset < roundup(segment->p_memsz, kPageSize);
			segmentOffset += kPageSize) {
			MapPage(segment->p_vaddr + segmentOffset, AllocatePage(),
				segment->p_flags & PF_W);
		}
	}

	*outStartAddress = imageHeader->e_entry;
}

unsigned AllocatePage()
{
	unsigned frame = nextFreePage;
	nextFreePage += kPageSize;
	memzero((void*) frame, kPageSize);
	return frame;
}

void MapPage(unsigned va, unsigned pa, bool writable)
{
	int pdindex = va / kPageSize / 1024;
	if (!(pagedir[pdindex] & kPresent))
		pagedir[pdindex] = AllocatePage() | kPresent | kGlobal | kWritable;

	unsigned *pagetable = (unsigned*)(pagedir[pdindex] & 0xfffff000);
	pagetable[(va / kPageSize) % 1024] = pa | kPresent | kGlobal | (writable ? kWritable : 0);
}

void ClearScreen()
{
	for (int offset = 0; offset < kScreenWidth * kScreenHeight; offset++)
		kScreenBase[offset] = 0x1f20;
}

void PrintMessage(const char *msg)
{
	int offset = 0;
	while (*msg)
		kScreenBase[offset++] = 0x1f00 | *msg++;
}

void memzero(void *data, int count)
{
	while (count && ((unsigned) data & 3) != 0) {
		*(char*) data = 0;
		data = (void*)((unsigned) data + 1);
		count--;
	}

	while (count >= 4) {
		*(unsigned*) data = 0;
		data = (void*)((unsigned) data + 4);
		count -= 4;
	}

	while (count) {
		*(char*) data = 0;
		data = (void*)((unsigned) data + 1);
		count--;
	}
}

int strcmp(const char *str1, const char *str2)
{
	while (*str1) {
		if (*str2 == 0)
			return -1;

		if (*str1 != *str2)
			return *str1 - *str2;

		str1++;
		str2++;
	}

	if (*str2)
		return 1;

	return 0;
}
