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

#include "BackingStore.h"
#include "cpu_asm.h"
#include "KernelDebug.h"
#include "Lock.h"
#include "Page.h"
#include "PageCache.h"
#include "PhysicalMap.h"
#include "stdio.h"
#include "string.h"
#include "SwapSpace.h"
#include "syscall.h"

Page** PageCache::fPageHash = 0;
int PageCache::fPageHashSize = 0;
Mutex PageCache::fCacheLock;

PageCache::PageCache(BackingStore *backingStore, PageCache *copyCache)
	:	fSourceCache(copyCache),
		fResidentPages(0),
		fRefCount(0)
{
	if (copyCache)
		copyCache->AcquireRef();

	if (backingStore)
		fBackingStore = backingStore;
	else
		fBackingStore = new SwapSpace;
}

PageCache::~PageCache()
{
	ASSERT(fRefCount == 0);
	
	fCacheLock.Lock();
	while (fResidentPages) {
		Page *page = fResidentPages;
		RemovePage(page);
		page->Free();
	}

	if (fSourceCache)
		fSourceCache->ReleaseRef();

	fCacheLock.Unlock();

	delete fBackingStore;
}

Page* PageCache::GetPage(off_t offset, bool privateCopy)
{
	Page *page = 0;
	fCacheLock.Lock();

	// Check to see if this page is in memory.
	for (;;) {
		page = LookupPage(offset);
		if (page == 0 || !page->IsBusy())
			break;

		// This page is busy, loop and try again.
		fCacheLock.Unlock();
		sleep(2000);
		fCacheLock.Lock();
	}

	if (page == 0 && fBackingStore && fBackingStore->HasPage(offset)) {
		// Check to see if the backing store has a copy.
		page = Page::Alloc();
		page->SetBusy();
		InsertPage(offset, page);

		fCacheLock.Unlock();
		char *va = PhysicalMap::LockPhysicalPage(page->GetPhysicalAddress());
		status_t err = fBackingStore->Read(offset, va);
		PhysicalMap::UnlockPhysicalPage(va);
		fCacheLock.Lock();
		if (err < E_NO_ERROR) {
			RemovePage(page);
			page->Free();
			page = 0;
		} else
			page->SetNotBusy();
	}

	if (page == 0 && privateCopy) {
		// Check to see if this is a private copy.
		page = Page::Alloc();
		page->SetBusy();
		InsertPage(offset, page);

		fCacheLock.Unlock();
		Page *sourcePage = fSourceCache->GetPage(offset, false);
		fCacheLock.Lock();

		if (sourcePage) {
			// Copy this page.  Note that the source page will never be busy.
			PhysicalMap::CopyPage(page->GetPhysicalAddress(), sourcePage->GetPhysicalAddress());
			page->SetNotBusy();
		} else {
			RemovePage(page);
			page->Free();
			page = 0;
		}
	}
	
	if (page == 0 && fSourceCache) {
		// Get the page and use it as a shared (read-only) page. 
		// Insert a dummy page at this virtual address to handle
		// collided page faults.  This will be removed and replaced
		// with the actual page.  Other threads will wake and find
		// the new page.
		Page *dummy = Page::Alloc();
		dummy->SetBusy();
		InsertPage(offset, dummy);
		fCacheLock.Unlock();
		page = fSourceCache->GetPage(offset, false);
		fCacheLock.Lock();
		RemovePage(dummy);
		dummy->Free();
	}
	
	if (page == 0) {
		// Anonymous page.  Zero it out.  Insert a dummy page as above.
		page = Page::Alloc(true);
		InsertPage(offset, page);
		page->SetNotBusy();
	}

	fCacheLock.Unlock();
	return page;
}

// This assumes that the cache lock and queue lock is already taken.
void PageCache::StealPage(Page *page, bool modified)
{
	RemovePage(page);
	if (modified) {
		// Page has been modified, write back
		fCacheLock.Unlock();
		const char *va = PhysicalMap::LockPhysicalPage(page->GetPhysicalAddress());
		status_t err = fBackingStore->Write(page->fCacheOffset, va);
		if (err < 0)
			panic("error writing back page");
			
		PhysicalMap::UnlockPhysicalPage(va);
		fCacheLock.Lock();
	}
}

void PageCache::AcquireRef()
{
	AtomicAdd(&fRefCount, 1);
}

void PageCache::ReleaseRef()
{
	ASSERT(fRefCount > 0);
	if (AtomicAdd(&fRefCount, -1) == 1)
		delete this;
}

off_t PageCache::Commit(off_t size)
{
	return fBackingStore->Commit(size);
}

void PageCache::Lock()
{
	fCacheLock.Lock();
}

void PageCache::Unlock()
{
	fCacheLock.Unlock();
}

void PageCache::Bootstrap()
{
	const int kHashSizes[] = {
		563, 2063, 4079, 8423, 16387, 32843, 65543, 131267, 262127, 0
	};

	int pageCount = Page::GetMemSize() / PAGE_SIZE;
	for (const int *sz = kHashSizes; *sz != 0; sz++) {
		fPageHashSize = *sz;
		if (pageCount < *sz)
			break;
	}
	
	fPageHash = new Page*[fPageHashSize];
	memset(fPageHash, 0, sizeof(Page*) * fPageHashSize);
	AddDebugCommand("cachestat", "Page Cache Statistics", PageCache::HashStats);
}

void PageCache::Print() const
{
	for (Page *page = fResidentPages; page; page = page->fCacheNext)
		printf(" phys=%08x offset=%08x\n", page->GetPhysicalAddress(),
			static_cast<int>(page->fCacheOffset));

	if (fSourceCache) {
		printf("\nCopy cache %p refcnt=%d\n", fSourceCache, fSourceCache->fRefCount);
		fSourceCache->Print();
	}
}

int PageCache::GenerateHash(off_t offset) const
{
	return (((reinterpret_cast<int>(this) / sizeof(PageCache)) << 7)
		| (static_cast<int>(offset) / PAGE_SIZE)) & 0x7fffffff;
}

void PageCache::InsertPage(off_t offset, Page *page)
{
	page->fCache = this;
	page->fCacheOffset = offset;

	// Insert the page into the hash table.
	Page **bucket = &fPageHash[GenerateHash(offset) % fPageHashSize];
	page->fHashNext = *bucket;
	*bucket = page;

	// Insert the page into this caches page list.
	page->fCacheNext = fResidentPages;
	page->fCachePrev = &fResidentPages;
	if (page->fCacheNext)
		page->fCacheNext->fCachePrev = &page->fCacheNext;
	
	fResidentPages = page;
}

void PageCache::RemovePage(Page *page)
{
	ASSERT(page->fCache == this);
	Page **link = &fPageHash[GenerateHash(page->fCacheOffset) % fPageHashSize];
	while (*link != page)
		link = &((*link)->fHashNext);

	*link = (*link)->fHashNext;	

	page->fCache = 0;
	*page->fCachePrev = page->fCacheNext;
	if (page->fCacheNext)
		page->fCacheNext->fCachePrev = page->fCachePrev;
}

Page* PageCache::LookupPage(off_t offset) const
{
	for (Page *page = fPageHash[GenerateHash(offset) % fPageHashSize]; page;
		page = page->fHashNext) {
		if (page->fCache == this && page->fCacheOffset == offset)
			return page;
	}

	return 0;
}

void PageCache::HashStats(int, const char*[])
{
	const int kMaxCount = 11;
	int counts[kMaxCount];
	memset(counts, 0, kMaxCount * sizeof(int));
	int totalCached = 0;

	printf("Tabulating...\n");
	for (int bucket = 0; bucket < fPageHashSize; bucket++) {
		int chainLength = 0;
		for (const Page *page = fPageHash[bucket]; page; page = page->fHashNext) {
			chainLength++;
			totalCached++;
		}

		if (chainLength >= kMaxCount)
			counts[kMaxCount - 1]++;
		else
			counts[chainLength]++;
	}

	printf("Cached pages: %d\n", totalCached);
	printf("Num buckets: %d\n", fPageHashSize);
	printf("Load Factor %d.%3d%%\n", totalCached * 100 / fPageHashSize,
		(totalCached * 100000 / fPageHashSize) % 1000);
	printf("Chain Lengths:\n");
	for (int length = 0; length < kMaxCount - 1; length++)
		printf("%d: %d\n", length, counts[length]);

	printf("%d+: %d\n", kMaxCount - 1, counts[kMaxCount - 1]);
}
