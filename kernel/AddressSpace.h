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

/// @file AddressSpace.h
#ifndef _ADDRESS_SPACE_H
#define _ADDRESS_SPACE_H

#include "AVLTree.h"
#include "Lock.h"
#include "types.h"

class Area;
class PageCache;
class PhysicalMap;
class Team;

/// AddressSpace is the machine independent version of a virtual address space
class AddressSpace {
public:
	AddressSpace();
	~AddressSpace();

	/// Allocate a contiguous range of virtual addresses in this address space
	/// @param name NULL terminated ASCII name of this area, for debugging purposes only.
	///   The name will be copied into an allocated buffer so the caller may retain ownership of it.
	///   The name will be truncated if it is larger than OS_NAME_LENGTH characters.
	/// @param areaSize Number of bytes in this area.  Must be a multiple of the physical page size
	/// @param wiring Determines if pages can be swapped out if needed
	/// @param protection Determines if pages can be written and/or read and if so, by supervisor, user, or both
	/// @param PageCache place to get pages that will be mapped into this area
	/// @param cacheOffset Offset into page cache for first virtual address of this area
	/// @param va Lowest virtual address of this area (in bytes).  Must be a multiple of physical page size
	///    If this is set to INVALID_PAGE, this function will search the address space for an unused range
	///    of bytes.
	/// @param flags Hints to CreateArea where it should allocate the area (start from high address, start
	///    from low address, etc).  Valid constants for this are in types.h
	/// @returns
	///   - Pointer to new Area object on success
	///   - NULL if it failed to create an area for some reason
	Area* CreateArea(const char name[], unsigned int areaSize, AreaWiring wiring, PageProtection protection,
		PageCache *cache, off_t cacheOffset, unsigned int va = INVALID_PAGE, int flags = 0);

	/// Create an area that maps physical memory.  This is primarly for device drivers, where the device
	/// is mapped into physical memory.
	/// @returns
	///   - Pointer to new Area object on success
	///   - NULL if it failed to create an area for some reason
	Area* MapPhysicalMemory(const char name[], unsigned int pa, unsigned int size,
		PageProtection protection);

	/// Change the size of an existing area, potentially allocating or freeing backing store associated with it
	/// @param area Area to be resized
	/// @param newSize New size of area in bytes.  Must be a multiple of physical page size
	/// @returns
	///   - E_NO_ERROR if the area was sucessfully resized
	///   - E_NO_MEMORY if there wasn't enough backing store to hold the newly allocated data
	///   - E_NO_MEMORY if there was another area in the way
	status_t ResizeArea(Area *area, unsigned int newSize);

	/// Remove an area from this address space and unmap all of its pages
	void DeleteArea(Area*);

	/// Called when a thread attempts to access an area of memory that doesn't have a physical
	/// page mapped to it.  This will attempt to map the appropriate page to that address.
	/// @param va Virtual address that user attempted to access
	/// @param write True if this was an attempt to write to said address, false if it was a read
	/// @param user True if user code attempted to access this address, false if supervisor code
	/// @returns
	///    - E_NOT_ALLOWED if the thread did not have permissions to perform this type
	///      of access on the given area (for example, write to a read only area of access of a
	///      supervisor only area by a user thread.
	///   - E_IO if an error occured reading data from backing store
	///   - E_BAD_ADDRESS if there is no area mapped to the address accessed
	///   - E_NO_ERROR if a page was sucessfully mapped to the address
	status_t HandleFault(unsigned int va, bool write, bool user);

	/// Try to unmap least frequently accessed pages from this address space
	/// @bug Shouldn't this be private?
	void TrimWorkingSet();

	/// Get the underlying, architecture dependent mapping object
	const PhysicalMap* GetPhysicalMap() const;

	/// Get the address space that the current thread is executing in
	static AddressSpace* GetCurrentAddressSpace();

	/// Get the address space for the kernel (note, although the kernel is physically mapped into
	/// the top of every address space, it has its own AddressSpace object representing it).
	static AddressSpace* GetKernelAddressSpace();

	/// Called at boot time to initialize structures
	static void Bootstrap();

	/// This thread just attempts to remove least frequently pages from address spaces
	/// @bug Shouldn't this be private?
	/// @bug Shouldn't this be NORETURN?
	static void PageDaemonLoop();

	/// Print debug information about this address space to the debug log
	void Print() const;

private:
	AddressSpace(PhysicalMap*);
	unsigned int FindFreeRange(unsigned int size, int flags = 0) const;
	static void TrimTeamWorkingSet(void*, Team*);

	RWLock fAreaLock;
	AVLTree fAreas;
	PhysicalMap *fPhysicalMap;
	volatile int fChangeCount;
	int fWorkingSetSize;
	int fMinWorkingSet;
	int fMaxWorkingSet;
	int fFaultCount;
	bigtime_t fLastWorkingSetAdjust;
	unsigned int fNextTrimAddress;
	static AddressSpace *fKernelAddressSpace;
};

#endif
