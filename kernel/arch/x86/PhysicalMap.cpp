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

#include "BootParams.h"
#include "cpu_asm.h"
#include "memory_layout.h"
#include "Page.h"
#include "PhysicalMap.h"
#include "Queue.h"
#include "stdio.h"
#include "string.h"
#include "x86.h"

class LockedPage : public QueueNode {
public:
	unsigned int pa;
	LockedPage *hashNext;
	int mapCount;
	LockedPage **hashPrev;
};

const unsigned int kPageMask = ~(PAGE_SIZE - 1);
List PhysicalMap::fPhysicalMaps;
LockedPage *PhysicalMap::fLockedPageHash[kLockedPageHashSize];
Queue PhysicalMap::fInactiveLockedPages;
LockedPage *PhysicalMap::fLockedPages = 0;
PhysicalMap *PhysicalMap::fKernelPhysicalMap = 0;
int PhysicalMap::fLockPageHits = 0;
int PhysicalMap::fLockPageRequests = 0;

PhysicalMap::PhysicalMap()
	:	fMappedPageCount(0),
		fLock("Physical Map Lock")
{
	Page *pageDirectory = Page::Alloc();
	pageDirectory->Wire();
	fPageDirectory = pageDirectory->GetPhysicalAddress();

	// Set up kernel space page directory entries.
	int *srcPageDir = reinterpret_cast<int*>(LockPhysicalPage(fKernelPhysicalMap->fPageDirectory));
	int *destPageDir = reinterpret_cast<int*>(LockPhysicalPage(fPageDirectory));
	memset(destPageDir, 0, 768 * 4);
	memcpy(destPageDir + 768, srcPageDir + 768, 256 * 4);
	UnlockPhysicalPage(srcPageDir);
	UnlockPhysicalPage(destPageDir);

	cpu_flags fl = DisableInterrupts();
	fPhysicalMaps.AddToTail(this);
	RestoreInterrupts(fl);
}

PhysicalMap::~PhysicalMap()
{
	cpu_flags fl = DisableInterrupts();
	fPhysicalMaps.Remove(this);
	RestoreInterrupts(fl);

	// Since the context switching code can lazily skip changing address
	// spaces (when switching to a kernel thread), its possible the
	// current kernel thread is running in the address space that is about
	// to go away.  Politely switch to the default kernel address space if
	// is about to happen.
	if (fPageDirectory == GetCurrentPageDir())
		SetCurrentPageDir(fKernelPhysicalMap->fPageDirectory);

	// Free up page tables (Deleting areas does not currently do this).
	unsigned int *pageDirVA = reinterpret_cast<unsigned int*>(LockPhysicalPage(fPageDirectory));
	for (int i = 0; i < 768; i++) {
		unsigned int pdent = pageDirVA[i]; 
		if (pdent & kPagePresent)
			Page::LockPage(pdent & kPageMask)->Free();
	}		

	UnlockPhysicalPage(pageDirVA);
	Page::LockPage(fPageDirectory)->Free();
}

void PhysicalMap::Map(unsigned int va, unsigned int pa, PageProtection protection)
{
	ASSERT(va < kKernelBase || fKernelPhysicalMap == this);
	fLock.Lock();

	unsigned int *pgdir = reinterpret_cast<unsigned int*>(LockPhysicalPage(fPageDirectory));
	unsigned int *pgtbl = 0;
	if ((pgdir[va / PAGE_SIZE / 1024] & kPagePresent) == 0) {
		// Allocate a new page table
		Page *page = Page::Alloc();
		page->Wire();
		unsigned int newTablePA = page->GetPhysicalAddress();
		pgtbl = reinterpret_cast<unsigned int*>(LockPhysicalPage(newTablePA));
		ClearPage(pgtbl);
		if (va >= kKernelBase) {
			// If this is in kernel space, map the page table into all
			// address spaces.
			for (ListNode *node = fPhysicalMaps.GetHead(); node;
				node = fPhysicalMaps.GetNext(node)) {
				PhysicalMap *map = static_cast<PhysicalMap*>(node);
				unsigned int *mpgdir = reinterpret_cast<unsigned int*>(LockPhysicalPage(map->fPageDirectory));
				mpgdir[va / PAGE_SIZE / 1024] = newTablePA | kPagePresent |
					kPageWritable;
				UnlockPhysicalPage(mpgdir);
			}
		} else
			pgdir[va / PAGE_SIZE / 1024] = newTablePA | kPagePresent | kPageWritable
				| kPageUser;
	} else
		pgtbl = reinterpret_cast<unsigned int*>(LockPhysicalPage(pgdir[va / PAGE_SIZE / 1024] & kPageMask));

	unsigned int pageFlags = kPagePresent;
	if (va >= kKernelBase)
		pageFlags |= kPageGlobal;

	if (protection & (USER_WRITE | SYSTEM_WRITE))
		pageFlags |= kPageWritable;

	if (protection & (USER_WRITE | USER_READ))
		pageFlags |= kPageUser;

	if (protection & kUncacheablePage)
		pageFlags |= kPageCacheDisable;

	if ((pgtbl[(va / PAGE_SIZE) % 1024] & kPagePresent) == 0)
		fMappedPageCount++;

	pgtbl[(va / PAGE_SIZE) % 1024] = pa | pageFlags;
	UnlockPhysicalPage(pgtbl);
	UnlockPhysicalPage(pgdir);

	InvalidateTLB(va);
	fLock.Unlock();
}

void PhysicalMap::Unmap(unsigned int base, unsigned int size)
{
	ASSERT(base < kKernelBase || fKernelPhysicalMap == this);

	fLock.Lock();
	unsigned int *pgdir = reinterpret_cast<unsigned int*>(LockPhysicalPage(fPageDirectory));
	int pdindex = base / PAGE_SIZE / 1024;
	int ptindex = (base / PAGE_SIZE) % 1024;
	int count = size / PAGE_SIZE;
	while (count > 0) {
		if ((pgdir[pdindex] & kPagePresent) == 0) {
			// No page table mapped, skip.

			if (ptindex + count < 1024)
				break;		// No more entries, done
		
			count -= 1024 - ptindex;
			pdindex++;
			ptindex = 0;
			continue;
		}

		unsigned int *pgtbl = reinterpret_cast<unsigned int*>(LockPhysicalPage(pgdir[pdindex] & kPageMask));		
		while (count > 0 && ptindex < 1024) {
			if (pgtbl[ptindex] & kPagePresent) {
				unsigned int va = (pdindex * 1024 + ptindex) * PAGE_SIZE;
				pgtbl[ptindex] = 0;
				fMappedPageCount--;
				InvalidateTLB(va);
			}

			count--;
			ptindex++;
		}	
		
		UnlockPhysicalPage(pgtbl);
		ptindex = 0;
		pdindex++;
	}

	UnlockPhysicalPage(pgdir);
	fLock.Unlock();
}

unsigned int PhysicalMap::GetPhysicalAddress(unsigned int va)
{
	fLock.Lock();
	unsigned int *pgdir = reinterpret_cast<unsigned int*>(LockPhysicalPage(fPageDirectory));
	unsigned int pdent = pgdir[va / PAGE_SIZE / 1024];
	UnlockPhysicalPage(pgdir);
	if ((pdent & kPagePresent) == 0) {
		fLock.Unlock();
		return INVALID_PAGE;
	}

	unsigned int *pt = reinterpret_cast<unsigned int*>(LockPhysicalPage(pgdir[va / PAGE_SIZE / 1024] & kPageMask));
	unsigned int ptent = pt[(va / PAGE_SIZE) % 1024];
	UnlockPhysicalPage(pt);

	if ((ptent & kPagePresent) == 0) {
		fLock.Unlock();
		return INVALID_PAGE;
	}

	unsigned int pa = ptent & kPageMask;
	fLock.Unlock();
	return pa;
}

int PhysicalMap::CountMappedPages() const
{
	return fMappedPageCount;
}

unsigned int PhysicalMap::GetPageDir() const
{
	return fPageDirectory;
}

char* PhysicalMap::LockPhysicalPage(unsigned int pa)
{
	ASSERT((pa & (PAGE_SIZE - 1)) == 0);

	unsigned int va = INVALID_PAGE;

	cpu_flags fl = DisableInterrupts();

	fLockPageRequests++;

	// Check to see if this is already mapped.
	LockedPage *lockedPage;
	for (lockedPage = fLockedPageHash[(pa / PAGE_SIZE) & kLockedPageHashSize - 1];
		lockedPage; lockedPage = lockedPage->hashNext) {
		if (lockedPage->pa == pa)
			break;
	}

	if (lockedPage == 0) {
		// This is not mapped, so find a new slot to map it in.
		lockedPage = static_cast<LockedPage*>(fInactiveLockedPages.Dequeue());
		if (lockedPage == 0)
			panic("out of IO page slots");

		if (lockedPage->pa != INVALID_PAGE) {
			*lockedPage->hashPrev = lockedPage->hashNext;
			if (lockedPage->hashNext)
				lockedPage->hashNext->hashPrev = lockedPage->hashPrev;		
		}
		
		LockedPage **bucket = &fLockedPageHash[(pa / PAGE_SIZE) % kLockedPageHashSize];
		lockedPage->hashPrev = bucket;
		*bucket = lockedPage;
		lockedPage->pa = pa;
		lockedPage->mapCount++;
		va = (lockedPage - fLockedPages) * PAGE_SIZE + kIOAreaBase;
		reinterpret_cast<unsigned int*>(kIOAreaBase)[lockedPage - fLockedPages] = pa | kPagePresent
			| kPageWritable;
		InvalidateTLB(va);
	} else {
		fLockPageHits++;
		va = (lockedPage - fLockedPages) * PAGE_SIZE + kIOAreaBase;
		if (lockedPage->mapCount++ == 0)
			lockedPage->RemoveFromList();
	}
	
	RestoreInterrupts(fl);	
	return reinterpret_cast<char*>(va);
}

void PhysicalMap::UnlockPhysicalPage(const void *va)
{
	cpu_flags fl = DisableInterrupts();
	unsigned int slot = (reinterpret_cast<unsigned int>(va) - kIOAreaBase) / PAGE_SIZE;	
	ASSERT(slot < 1024);
	ASSERT(fLockedPages[slot].mapCount > 0);
	if (--fLockedPages[slot].mapCount == 0)
		fInactiveLockedPages.Enqueue(&fLockedPages[slot]);
	
	RestoreInterrupts(fl);
}

void PhysicalMap::CopyPage(unsigned int destpa, unsigned int srcpa)
{
	const char *src = LockPhysicalPage(srcpa);
	char *dest = LockPhysicalPage(destpa);
	CopyPageInternal(dest, src);
	UnlockPhysicalPage(src);
	UnlockPhysicalPage(dest);
}

void PhysicalMap::Bootstrap()
{
	// Set up an area to temporarily map physical pages.
	fLockedPages = new LockedPage[1024];
	for (int i = 1; i < 1024; i++) {
		fInactiveLockedPages.Enqueue(&fLockedPages[i]);
		fLockedPages[i].pa = INVALID_PAGE;
		fLockedPages[i].hashNext = 0;
		fLockedPages[i].hashPrev = 0;
		fLockedPages[i].mapCount = 0;
	}

	fLockedPages[0].mapCount = 0x3fffffff;

	fKernelPhysicalMap =  new PhysicalMap(GetCurrentPageDir());

	// Grab information about physical memory allocation from the bootloader
	// and update the kernel tables.
	for (int rangeIndex = 0; rangeIndex < bootParams.rangeCount; rangeIndex++) {
		for (unsigned int pa = bootParams.allocatedRange[rangeIndex].begin; pa <
			bootParams.allocatedRange[rangeIndex].end; pa += PAGE_SIZE) {
			Page::MarkUsed(pa);
		}
	}

	// Set up a place to temporarily map physical memory when needed.  This is a tricky part,
	// because this might unwittingly allocate the page table that maps this page in low memory
	// so care is needed when modifying.  Since this only touches the allocated
	// page once, it is safe (although a little clunky).
	// Note: at this point, VA = PA in low memory.
	unsigned int *pagedir = reinterpret_cast<unsigned int*>(GetCurrentPageDir());
	unsigned int *pte = reinterpret_cast<unsigned int*>(Page::Alloc()->GetPhysicalAddress());
	pagedir[kIOAreaBase / PAGE_SIZE / 1024] = (reinterpret_cast<unsigned int>(pte) & kPageMask)
		| kPagePresent | kPageWritable | kPageGlobal;
	pte[0] = (reinterpret_cast<unsigned int>(pte) & kPageMask) | kPagePresent | kPageWritable
		| kPageGlobal;

	// Clear out the rest of this page table for cleanliness.
	void *va = LockPhysicalPage(reinterpret_cast<unsigned int>(pte));
	memset(reinterpret_cast<char*>(va) + 4, 0, PAGE_SIZE - 4);
	UnlockPhysicalPage(va);

	// Clear out the rest of user space area that was used by boot
	va = LockPhysicalPage(reinterpret_cast<unsigned int>(pagedir));
	memset(va, 0, 768 * sizeof(int));
	UnlockPhysicalPage(va);

	// Flush all of the TLBs
	SetCurrentPageDir(GetCurrentPageDir());

	AddDebugCommand("pmapstat", "Statistics about physical maps", PrintStats);
}

PhysicalMap* PhysicalMap::GetKernelPhysicalMap()
{
	return fKernelPhysicalMap;
}

PhysicalMap::PhysicalMap(unsigned int pageDirAddress)
	:	fPageDirectory(pageDirAddress),
		fMappedPageCount(0),
		fLock("Physical Map Lock")
{
	fPhysicalMaps.AddToTail(this);
}

void PhysicalMap::PrintStats(int, const char*[])
{
	printf("Locked page area:\n");
	printf("Hits %d Requests %d (%d%%)\n", fLockPageHits, fLockPageRequests, fLockPageHits * 100 / fLockPageRequests);
	const int kSampleSize = 12;
	int lengths[kSampleSize];
	memset(lengths, 0, kSampleSize * sizeof(int));
	for (int bucket = 0; bucket < kLockedPageHashSize; bucket++) {
		int count = 0;
		for (LockedPage *page = fLockedPageHash[bucket]; page; page = page->hashNext)
			count++;

		lengths[count >= kSampleSize ? kSampleSize : count]++;
	}

	for (int i = 0; i < kSampleSize; i++)
		printf("%d %d\n", i, lengths[i]);
}
