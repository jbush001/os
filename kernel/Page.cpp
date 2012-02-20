#include "BootParams.h"
#include "cpu_asm.h"
#include "KernelDebug.h"
#include "Page.h"
#include "PageCache.h"
#include "PhysicalMap.h"
#include "Semaphore.h"
#include "syscall.h"
#include "stdio.h"
#include "string.h"
#include "Thread.h"

Semaphore Page::fFreePagesAvailable("Free Pages Available", 0);
Page* Page::fPages = 0;
Queue Page::fFreeQueue;
Queue Page::fActiveQueue;
Queue Page::fClearQueue;
int Page::fPageCount = 0;
int Page::fFreeCount = 0;
int Page::fTransitionCount = 0;
int Page::fActiveCount = 0;
int Page::fWiredCount = 0;
int Page::fClearCount = 0;

int64 Page::fPagesCleared = 0;
int64 Page::fPagesRequested = 0;
int64 Page::fClearPagesRequested = 0;
int64 Page::fClearPageHits = 0;
int64 Page::fClearPagesUsedAsFree = 0;

Page* Page::LockPage(unsigned int pa)
{
	Page *page;
	cpu_flags fl = DisableInterrupts();
	for (;;) {
		page = &fPages[pa / PAGE_SIZE];
		if (page->fState != kPageTransition)
			break;

		RestoreInterrupts(fl);
		sleep(20000);
		fl = DisableInterrupts();
	}

	page->MoveToQueue(kPageTransition);
	RestoreInterrupts(fl);
	return page;
}

unsigned int Page::GetPhysicalAddress() const
{
	return (this - fPages) * PAGE_SIZE;
}

Page* Page::Alloc(bool clear)
{
	Page *page = 0;

	fFreePagesAvailable.Wait();

	// Note that we grab pages from the tail of these queues.  This helps
	// processor cache utilization, but also improves performance of the
	// physical map page locking area.
	cpu_flags fl = DisableInterrupts();
	fPagesRequested++;
	if (clear && fClearCount > 0) {
		// There is already a pre-cleared page, use that.
		fClearPagesRequested++;
		fClearPageHits++;
		page = static_cast<Page*>(fClearQueue.GetTail());
	} else if (clear) {
		// There aren't pre-cleared pages available, clear one now.
		fClearPagesRequested++;
		page = static_cast<Page*>(fFreeQueue.GetTail());
		if (clear) {
			char *va = PhysicalMap::LockPhysicalPage(page->GetPhysicalAddress());
			ClearPage(va);
			PhysicalMap::UnlockPhysicalPage(va);
		}
	} else if (fFreeCount > 0) {
		// This page should not be cleared, so just grab it off the
		// free queue.
		page = static_cast<Page*>(fFreeQueue.GetTail());
	} else {
		// This page should not be cleared, but there aren't any free
		// pages.  Use a cleared page instead.
		fClearPagesUsedAsFree++;
		page = static_cast<Page*>(fClearQueue.GetTail());
	}
	
	page->MoveToQueue(kPageTransition);
	RestoreInterrupts(fl);
	return page;
}

void Page::Free()
{
	ASSERT(fState != kPageFree);
	ASSERT(fCache == 0);
	MoveToQueue(kPageFree);
}

void Page::SetBusy()
{
	MoveToQueue(kPageTransition);
}

void Page::SetNotBusy()
{
	ASSERT(fCache != 0);
	MoveToQueue(kPageActive);
}

void Page::Wire()
{
	MoveToQueue(kPageWired);
}

void Page::Unwire()
{
	ASSERT(fCache != 0);
	MoveToQueue(kPageActive);
}

int Page::CountFreePages()
{
	return fFreeCount;
}

void Page::Bootstrap()
{
	fPageCount = bootParams.memsize / PAGE_SIZE;
	fPages = new Page[fPageCount];
	fFreeCount = fPageCount;
	fFreePagesAvailable.Release(fPageCount, false);
	for (int pageIndex = 0; pageIndex < fPageCount; pageIndex++) {
		fPages[pageIndex].fCache = 0;
		fPages[pageIndex].fState = kPageFree;
		fFreeQueue.Enqueue(&fPages[pageIndex]);
	}

	AddDebugCommand("pgstat", "Page statistics", PrintStats);
}

void Page::StartPageEraser()
{
	new Thread("Page Eraser", Thread::GetRunningThread()->GetTeam(), PageEraser,
		0, 1);
}

// This is a bit of a kludge to work around the chicken & egg problem
// of allocating pages from the first stage loader for things like
// the initial stack, the kernel image, and page management data structures.
// It is called after Page::Bootstrap to specify which pages are already in use.
void Page::MarkUsed(unsigned int pa)
{
	Page *page = &fPages[pa / PAGE_SIZE];
	if (page->fState == kPageFree) {
		// This never blocks (which would be fatal right now), as there
		// are enough pages available at this stage of the bootstrap; it
		// is just acquired to update the count.
		fFreePagesAvailable.Wait();
		page->MoveToQueue(kPageWired);
	} else
		printf("Page %08x is in state %d\n", pa, page->fState);
}

void Page::MoveToQueue(PageState newState)
{
	cpu_flags fl = DisableInterrupts();
	switch (fState) {
		case kPageFree:
			fFreeCount--;
			RemoveFromList();
			break;

		case kPageTransition:
			fTransitionCount--;
			break;

		case kPageActive:
			fActiveCount--;
			RemoveFromList();
			break;

		case kPageWired:
			fWiredCount--;
			break;

		case kPageClear:
			fClearCount--;
			RemoveFromList();
			break;

		default:
			panic("Page::MoveToQueue: bad page state 1");
	}

	fState = newState;

	switch (fState) {
		case kPageFree:
			ASSERT(fCache == 0);
			fFreeCount++;
			fFreeQueue.Enqueue(this);
			fFreePagesAvailable.Release(1, false);
			break;

		case kPageTransition:
			fTransitionCount++;
			break;

		case kPageActive:
			ASSERT(fCache != 0);
			fActiveCount++;
			fActiveQueue.Enqueue(this);
			break;

		case kPageWired:
			fWiredCount++;
			break;

		case kPageClear:
			fClearCount++;
			fClearQueue.Enqueue(this);
			fFreePagesAvailable.Release(1, false);
			break;

		default:
			panic("Page::MoveToQueue bad page state 2");
	}

	RestoreInterrupts(fl);
}

int Page::PageEraser(void*)
{
	for (;;) {
		DisableInterrupts();
		fFreePagesAvailable.Wait();	// Shouldn't block, just update count

		Page *page = static_cast<Page*>(fFreeQueue.GetTail());
		if (!page) {
			EnableInterrupts();
			fFreePagesAvailable.Release(1, false);
			sleep(100000);
			continue;
		}

		page->MoveToQueue(kPageTransition);
		EnableInterrupts();

		char *va = PhysicalMap::LockPhysicalPage(page->GetPhysicalAddress());
		ClearPage(va);
		PhysicalMap::UnlockPhysicalPage(va);
		
		DisableInterrupts();
		page->MoveToQueue(kPageClear);
		fPagesCleared++;
		EnableInterrupts();
	}

	return 0;
}

void Page::PrintStats(int, const char**)
{
	printf("Page Statistics\n");
	printf("  Free:        %5u (%2u%%)  %uk\n", fFreeCount, fFreeCount * 100 / fPageCount, fFreeCount * PAGE_SIZE / 1024);
	printf("  Active:      %5u (%2u%%)  %uk\n", fActiveCount, fActiveCount * 100 / fPageCount, fActiveCount * PAGE_SIZE / 1024);
	printf("  Wired:       %5u (%2u%%)  %uk\n", fWiredCount, fWiredCount * 100 / fPageCount, fWiredCount * PAGE_SIZE / 1024);
	printf("  Transition:  %5u (%2u%%)  %uk\n", fTransitionCount, fTransitionCount * 100 / fPageCount, fTransitionCount * PAGE_SIZE / 1024);
	printf("  Clear:       %5u (%2u%%)  %uk\n", fClearCount, fClearCount * 100 / fPageCount, fClearCount * PAGE_SIZE / 1024);
	printf("  Total:       %5u        %2uk\n", fPageCount, fPageCount * PAGE_SIZE / 1024);
	printf("\n");
	printf("Pages requested:          %Ld\n", fPagesRequested);
	printf("Clear pages requested:    %Ld (%Ld%%)\n", fClearPagesRequested,
		fClearPagesRequested * 100 / fPagesRequested);
	printf("Pages cleared:            %Ld\n", fPagesCleared);
	printf("Clear page hits:          %Ld/%Ld (%Ld%%)\n", fClearPageHits, fClearPagesRequested,
		fClearPageHits * 100 / fClearPagesRequested);
	printf("Clear pages used as free: %Ld/%Ld (%Ld%%)\n", fClearPagesUsedAsFree,
		fPagesRequested - fClearPagesRequested, fClearPagesUsedAsFree * 100
		/ (fPagesRequested - fClearPagesRequested));
}
