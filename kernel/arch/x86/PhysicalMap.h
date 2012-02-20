#ifndef _PHYSICAL_MAP_H
#define _PHYSICAL_MAP_H

#include "List.h"
#include "Lock.h"
#include "types.h"

const int kLockedPageHashSize = 2048;

const int kUncacheablePage = 64; // private PageProtection flag

class PhysicalMap : private ListNode {
public:
	PhysicalMap();
	virtual ~PhysicalMap();
	void Map(unsigned int va, unsigned int pa, PageProtection);
	void Unmap(unsigned int base, unsigned int size);
	unsigned int GetPhysicalAddress(unsigned int va);
	int CountMappedPages() const;
	unsigned int GetPageDir() const;
	static char* LockPhysicalPage(unsigned int pa);
	static void UnlockPhysicalPage(const void *va);
	static void CopyPage(unsigned int destpa, unsigned int srcpa);
	static void Bootstrap();
	static PhysicalMap* GetKernelPhysicalMap();

private:
	PhysicalMap(unsigned int pageDirAddress);
	static void PrintStats(int, const char**);

	unsigned int fPageDirectory;
	int fMappedPageCount;
	RecursiveLock fLock;
	static List fPhysicalMaps;
	static struct LockedPage *fLockedPageHash[kLockedPageHashSize];
	static Queue fInactiveLockedPages;
	static struct LockedPage *fLockedPages;
	static PhysicalMap *fKernelPhysicalMap;
	static int fLockPageHits;
	static int fLockPageRequests;
};

#endif
