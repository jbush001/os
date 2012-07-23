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

/// @file Page.h
#ifndef _PAGE_H
#define _PAGE_H

#include "Lock.h"
#include "Queue.h"

/// Architecture dependent abstraction for a physical page frame.
class Page : public QueueNode {
public:
	/// Get a page from a specific physical address and mark it so
	/// it can't be paged out by other threads.
	/// @note this doesn't acquire a blocking lock, it just sets
	/// the pages state so other threads will leave it alone
	static Page* LockPage(unsigned int pa);

	/// Get the physical address for this page, in bytes
	unsigned int GetPhysicalAddress() const;

	/// Allocate an unused page.  This may block if pages need to be swapped out.
	/// @param clear If this is true, the page will be zeroed out
	static Page* Alloc(bool clear = false);

	/// Move this page to the free list
	void Free();

	/// Mark this page as busy (IO is currently being performed)
	void SetBusy();

	/// Mark this page as not busy (IO is finished)
	void SetNotBusy();

	/// @returns true if this page is busy
	inline bool IsBusy() const;

	/// Lock this page into place so it can't be swapped
	void Wire();

	/// Unlock this page so it can be swapped if needed
	void Unwire();

	/// Get the total number of free pages
	static int CountFreePages();

	/// Get the total size of memory in bytes
	static inline unsigned int GetMemSize();

	/// Called at boot time to initialize strutures
	static void Bootstrap();

	/// Start the low priority thread that erases pages in the background
	static void StartPageEraser();

	/// Mark a specific physical address used (used during initialization to mark
	/// pages that have been preallocated by the bootloader)
	static void MarkUsed(unsigned int pa);

private:
	enum PageState {
		kPageFree,
		kPageTransition,
		kPageActive,
		kPageWired,
		kPageClear
	};

	void MoveToQueue(PageState);
	static int PageEraser(void*);
	static void PrintStats(int, const char**);

	Page *fHashNext;
	class PageCache *fCache;
	off_t fCacheOffset;
	Page *fCacheNext;
	Page **fCachePrev;
	volatile PageState fState;

	static class Semaphore fFreePagesAvailable;
	static Page *fPages;
	static Queue fFreeQueue;
	static Queue fActiveQueue;
	static Queue fClearQueue;
	static int fPageCount;
	static int fFreeCount;
	static int fTransitionCount;
	static int fActiveCount;
	static int fWiredCount;
	static int fClearCount;

	// Page erase statistics
	static int64 fPagesCleared;
	static int64 fPagesRequested;
	static int64 fClearPagesRequested;
	static int64 fClearPageHits;
	static int64 fClearPagesUsedAsFree;


	friend class PageCache;
};

inline unsigned int Page::GetMemSize()
{
	return fPageCount * PAGE_SIZE;
}

inline bool Page::IsBusy() const
{
	return fState == kPageTransition;
}

#endif
