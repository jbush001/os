#include "AddressSpace.h"
#include "Area.h"
#include "cpu_asm.h"
#include "memory_layout.h"
#include "Page.h"
#include "PageCache.h"
#include "PhysicalMap.h"
#include "stdio.h"
#include "string.h"
#include "syscall.h"
#include "Team.h"
#include "Thread.h"

const int kDefaultWorkingSet = 0x40000;
const int kDefaultMinWorkingSet = 0x10000;
const int kDefaultMaxWorkingSet = 0x200000;
const int kWorkingSetAdjustInterval = 2000000;
const int kMinFaultsPerSecond = 20;
const int kMaxFaultsPerSecond = 100;
const int kMinFreePages = 40;
const int kWorkingSetIncrement = PAGE_SIZE * 10;
const bigtime_t kTrimInterval = 500000;

AddressSpace* AddressSpace::fKernelAddressSpace = 0;

AddressSpace::AddressSpace()
	:	fPhysicalMap(new PhysicalMap),
		fChangeCount(0),
		fWorkingSetSize(kDefaultWorkingSet),
		fMinWorkingSet(kDefaultMinWorkingSet),
		fMaxWorkingSet(kDefaultMaxWorkingSet),
		fFaultCount(0),
		fLastWorkingSetAdjust(SystemTime()),
		fNextTrimAddress(0)
{
	// This is a new user address space.  Add some dummy areas for
	// the lower 4k (which is reserved to detect null pointer references),
	// and for kernel space, so they can't be allocated.
	fAreas.Add(new Area("(null area)"), 0, kUserBase - 1);
	fAreas.Add(new Area("(kernel)"), kKernelBase, kKernelTop);
}

AddressSpace::~AddressSpace()
{
	for (;;) {
		// This is a very clunky way to step through the area tree while
		// removing items from it.  The iterator must be re-created because
		// it will become invalid after each area is removed from the tree.
		AVLTreeIterator iterator(fAreas, true);
		Area *area = static_cast<Area*>(iterator.GetCurrent());
		if (area == 0)
			break;

		// Don't unmap the pages because:
		// 1. The address space is going away anyway
		// 2. This would unmap the kernel. :)
		fAreas.Remove(area);
		area->ReleaseRef();
	}

	delete fPhysicalMap;
}

Area* AddressSpace::CreateArea(const char name[], unsigned int size, AreaWiring wiring,
	PageProtection protection, PageCache *cache, off_t offset, unsigned int va,
	int flags)
{
	if (size % PAGE_SIZE != 0 || size == 0)
		return 0;

	if (cache != 0 && cache->Commit(offset + size) != offset + size)
		return 0;

	fAreaLock.LockWrite();
	if (va == INVALID_PAGE)
		va = FindFreeRange(size, flags);
	else if (va % PAGE_SIZE != 0 || !fAreas.IsRangeFree(va, va + size - 1))
		va = INVALID_PAGE;

	Area *area = 0;
	if (va != INVALID_PAGE) {
		area = new Area(name, protection, cache, offset, wiring);
		fAreas.Add(area, va, va + size - 1);
		if (wiring & AREA_WIRED) {
			for (unsigned int aroffs = 0; aroffs < size; aroffs += PAGE_SIZE) {
				Page *page = area->GetPageCache()->GetPage(area->GetCacheOffset()
					+ aroffs, false);
				page->Wire();
				fPhysicalMap->Map(va + aroffs, page->GetPhysicalAddress(), protection);
			}
		}
	}

	fChangeCount++;
	fAreaLock.UnlockWrite();
	return area;
}

Area* AddressSpace::MapPhysicalMemory(const char name[], unsigned int pa,
	unsigned int size, PageProtection protection)
{
	fAreaLock.LockWrite();
	unsigned int va = FindFreeRange(size);
	if (va == INVALID_PAGE) {
		fAreaLock.UnlockWrite();
		return 0;
	}

	Area *area = new Area(name, protection, 0, 0, AREA_WIRED);
	fAreas.Add(area, va, va + size - 1);
	for (unsigned int aroffs = 0; aroffs < size; aroffs += PAGE_SIZE)
		fPhysicalMap->Map(va + aroffs, pa + aroffs, protection);

	fChangeCount++;
	fAreaLock.UnlockWrite();
	return area;
}

status_t AddressSpace::ResizeArea(Area *area, unsigned int newSize)
{
	if (newSize == 0)
		return E_INVALID_OPERATION;

	fAreaLock.LockWrite();
	if (newSize > area->GetSize()) {
		// Grow the area.
		if (area->GetPageCache()->Commit(newSize) != newSize) {
			fAreaLock.UnlockWrite();
			return E_NO_MEMORY;
		}

		if (area->GetBaseAddress() + newSize < area->GetBaseAddress()	// wrap
			|| !fAreas.IsRangeFree(area->GetBaseAddress() + area->GetSize(),
			area->GetBaseAddress() + newSize))	{
			fAreaLock.UnlockWrite();
			return E_NO_MEMORY;
		}

		if (area->GetWiring() == AREA_WIRED) {
			for (unsigned int aroffs = area->GetSize(); aroffs < newSize;
				aroffs += PAGE_SIZE) {
				Page *page = area->GetPageCache()->GetPage(area->GetCacheOffset()
					+ aroffs, false);
				page->Wire();
				fPhysicalMap->Map(area->GetBaseAddress() + aroffs, page->GetPhysicalAddress(),
					area->GetProtection());
			}
		}
	} else if (newSize < area->GetSize()) {
		// Shrink the area.
		fPhysicalMap->Unmap(area->GetBaseAddress() + newSize, area->GetSize() - newSize);
		area->GetPageCache()->Commit(newSize);
	}

	fAreas.Resize(area, newSize);
	fChangeCount++;
	fAreaLock.UnlockWrite();
	return E_NO_ERROR;
}

void AddressSpace::DeleteArea(Area *area)
{
	fAreaLock.LockWrite();
	fAreas.Remove(area);
	fPhysicalMap->Unmap(area->GetBaseAddress(), area->GetSize());
	area->ReleaseRef();
	fChangeCount++;
	fAreaLock.UnlockWrite();
}

status_t AddressSpace::HandleFault(unsigned int va, bool write, bool user)
{
	va &= ~(PAGE_SIZE - 1); // Round down to a page boundry.

	fAreaLock.LockRead();
	int lastChangeCount = fChangeCount;
	Area *area = static_cast<Area*>(fAreas.Find(va));
	if (area == 0) {
		fAreaLock.UnlockRead();
		return E_BAD_ADDRESS;
	}

	PageProtection protection = area->GetProtection();
	if ((user && write && !(protection & USER_WRITE))
		|| (user && !write && !(protection & USER_READ))
		|| (!user && write && !(protection & SYSTEM_WRITE))
		|| (!user && !write && !(protection & SYSTEM_READ))) {
		fAreaLock.UnlockRead();
		return E_NOT_ALLOWED;
	}

	PageCache *cache = area->GetPageCache();
	if (cache == 0) {
		fAreaLock.UnlockRead();
		return E_NOT_ALLOWED;
	}

	bool copy = cache->IsCopy();
	cache->AcquireRef();
	fAreaLock.UnlockRead();
	off_t offset = va - area->GetBaseAddress() + area->GetCacheOffset();
	Page *page = cache->GetPage(offset, write && cache->IsCopy());
	cache->ReleaseRef();
	if (page == 0)
		return E_IO;

	fAreaLock.LockRead();
	if (lastChangeCount != fChangeCount) {
		// Changes have occured to this address.  Make sure that
		// the area hasn't changed underneath the fault handler.
		Area *newArea = static_cast<Area*>(fAreas.Find(va));
		if (newArea != area || newArea->GetPageCache() != cache ||
			newArea->GetCacheOffset() != offset)
			fAreaLock.UnlockRead();
			return E_BAD_ADDRESS;
	}

	// If this is a read from copy-on-write page, it is shared with the
	// original cache.  Mark it read only.
	if (copy && !write)
		protection &= ~(USER_WRITE | SYSTEM_WRITE);

	fPhysicalMap->Map(va, page->GetPhysicalAddress(), protection);
	fAreaLock.UnlockRead();
	AtomicAdd(&fFaultCount, 1);
	return E_NO_ERROR;
}

void AddressSpace::TrimWorkingSet()
{
	int mappedMemory = fPhysicalMap->CountMappedPages() * PAGE_SIZE;
	bigtime_t now = SystemTime();

	// Adjust the working set size based on fault rate.
	if (now - fLastWorkingSetAdjust > kWorkingSetAdjustInterval) {
		cpu_flags fl = DisableInterrupts();
		int faultsPerSecond = static_cast<int64>(fFaultCount)
			* 1000000 / (now - fLastWorkingSetAdjust);
		if (faultsPerSecond > kMaxFaultsPerSecond
			&& mappedMemory >= fWorkingSetSize
			&& fWorkingSetSize < fMaxWorkingSet) {
			fWorkingSetSize = MIN(fWorkingSetSize + kWorkingSetIncrement, fMaxWorkingSet);
		} else if (faultsPerSecond < kMinFaultsPerSecond
			&& mappedMemory <= fWorkingSetSize
			&& fWorkingSetSize > fMaxWorkingSet
			&& Page::CountFreePages() < kMinFreePages) {
			fWorkingSetSize = MAX(fWorkingSetSize - kWorkingSetIncrement, fMaxWorkingSet);
		}

		fLastWorkingSetAdjust = now;
		fFaultCount = 0;
		RestoreInterrupts(fl);
	}

	// Trim some pages if needed
	while (mappedMemory > fWorkingSetSize) {

        break;

	}
}

const PhysicalMap* AddressSpace::GetPhysicalMap() const
{
	return fPhysicalMap;
}

AddressSpace* AddressSpace::GetCurrentAddressSpace()
{
	return Thread::GetRunningThread()->GetTeam()->GetAddressSpace();
}

AddressSpace* AddressSpace::GetKernelAddressSpace()
{
	return fKernelAddressSpace;
}

void AddressSpace::Bootstrap()
{
	fKernelAddressSpace = new AddressSpace(PhysicalMap::GetKernelPhysicalMap());
}

void AddressSpace::Print() const
{
	printf("Name                 Start    End      Cache    Protection\n");
	for (AVLTreeIterator iterator(fAreas, true); iterator.GetCurrent();
		iterator.GoToNext()) {
		const Area *area = static_cast<const Area*>(iterator.GetCurrent());
		PageCache *cache = area->GetPageCache();
		PageProtection prot = area->GetProtection();
		printf("%20s %08x %08x %p %c%c%c%c%c%c\n", area->GetName(),
			area->GetBaseAddress(),
			area->GetBaseAddress() + area->GetSize() - 1, cache,
			(prot & USER_READ) ? 'r' : '-',
			(prot & USER_WRITE) ? 'w' : '-',
			(prot & USER_EXEC) ? 'x' : '-',
			(prot & SYSTEM_READ) ? 'r' : '-',
			(prot & SYSTEM_WRITE) ? 'w' : '-',
			(prot & SYSTEM_EXEC) ? 'x' : '-');
	}

	printf("Resident: %dk Working Set: %dk Min: %dk Max: %dk Faults/Sec: %Ld\n",
		fPhysicalMap->CountMappedPages() * PAGE_SIZE, fWorkingSetSize / 1024, fMinWorkingSet / 1024,
		fMaxWorkingSet / 1024, static_cast<int64>(fFaultCount) * 1000000
		/ kWorkingSetAdjustInterval);
	printf("\n");
}

// Constructor for kernel address space
AddressSpace::AddressSpace(PhysicalMap *map)
	:	fPhysicalMap(map),
		fWorkingSetSize(kDefaultWorkingSet),
		fMinWorkingSet(kDefaultMinWorkingSet),
		fMaxWorkingSet(kDefaultMaxWorkingSet),
		fFaultCount(0),
		fLastWorkingSetAdjust(SystemTime()),
		fNextTrimAddress(0)
{
	fAreas.Add(new Area("(user space)"), 0, kUserTop);	// dummy area->
	fAreas.Add(new Area("Kernel Text", SYSTEM_READ | SYSTEM_EXEC), kKernelBase, kKernelDataBase - 1);
	fAreas.Add(new Area("Kernel Data", SYSTEM_READ | SYSTEM_WRITE), kKernelDataBase, kKernelDataTop);
	fAreas.Add(new Area("Kernel Heap", SYSTEM_READ | SYSTEM_WRITE), kHeapBase, kHeapTop);
	fAreas.Add(new Area("Hyperspace", SYSTEM_READ | SYSTEM_WRITE), kIOAreaBase, kIOAreaTop);
	Area *kstack = new Area("Init Stack", SYSTEM_READ | SYSTEM_WRITE);
	fAreas.Add(kstack, kBootStackBase, kBootStackTop);
	Thread::GetRunningThread()->SetKernelStack(kstack);
}

unsigned int AddressSpace::FindFreeRange(unsigned int size, int flags) const
{
	unsigned int base = INVALID_PAGE;
	if (flags & SEARCH_FROM_BOTTOM) {
		unsigned int low = 0;
		for (AVLTreeIterator iterator(fAreas, true); iterator.GetCurrent();
			iterator.GoToNext()) {
			const Area *area = static_cast<const Area*>(iterator.GetCurrent());
			if (area->GetBaseAddress() - low >= size) {
				base = low;
				break;
			}

			low = area->GetHighKey() + 1;
		}
	} else {
		unsigned int high = kAddressSpaceTop;
		for (AVLTreeIterator iterator(fAreas, false); iterator.GetCurrent();
			iterator.GoToNext()) {
			const Area *area = static_cast<const Area*>(iterator.GetCurrent());
			if (high - area->GetHighKey() >= size) {
				base = high + 1 - size;
				break;
			}

			high = area->GetBaseAddress() - 1;
		}
	}

	return base;
}

void AddressSpace::PageDaemonLoop()
{
	for (;;) {
		sleep(kTrimInterval);
		Team::DoForEach(TrimTeamWorkingSet, 0);
	}
}

void AddressSpace::TrimTeamWorkingSet(void *, Team *team)
{
	team->GetAddressSpace()->TrimWorkingSet();
}
