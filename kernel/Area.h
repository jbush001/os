/// @file Area.h
#ifndef _AREA_H
#define _AREA_H

#include "AVLTree.h"
#include "Resource.h"
#include "PageCache.h"

/// An area represents a range of virtual addresses with the same backing store
/// This can be a memory mapped file, an anonymous range of swapped back space (like user
/// stacks or application heaps), or a wired down range of pages like a kernel stack.
class Area : public AVLNode, public Resource {
public:
	inline Area(const char name[], PageProtection = 0, PageCache* = 0, off_t = 0,
		AreaWiring lock = AREA_WIRED);
	virtual ~Area();

	/// Return the lowest virtual address of this area
	inline unsigned int GetBaseAddress() const;

	/// Return the number of bytes between the lowest virtual address and the highest virtual
	/// address.
	inline unsigned int GetSize() const;

	/// Return the page cache associated with this area.  The page cache is used
	/// to satisfy page faults that occur in this area.
	inline PageCache* GetPageCache() const;

	/// Get architecture independent protection flags for all pages in this area (can
	/// the page be written, read and if so, by user, system, or both)
	inline PageProtection GetProtection() const;

	///What is the offset into the PageCache for the first virtual address of this area.
	inline off_t GetCacheOffset() const;

	/// Determine if the pages can be replaced (swapped out) or not.
	inline AreaWiring GetWiring() const;

private:
	PageProtection fProtection;
	PageCache *fPageCache;
	off_t fCacheOffset;
	AreaWiring fWiring;
};

inline Area::Area(const char name[], PageProtection protection, PageCache *cache,
	off_t offset, AreaWiring lock)
	:	Resource(OBJ_AREA, name),
		fProtection(protection),
		fPageCache(cache),
		fCacheOffset(offset),
		fWiring(lock)
{
	if (fPageCache)
		fPageCache->AcquireRef();

	AcquireRef();
}

inline Area::~Area()
{
	if (fPageCache)
		fPageCache->ReleaseRef();
}

inline unsigned int Area::GetBaseAddress() const
{
	return GetLowKey();
}

inline unsigned int Area::GetSize() const
{
	return GetHighKey() - GetLowKey() + 1;
}

inline PageCache* Area::GetPageCache() const
{
	return fPageCache;
}

inline PageProtection Area::GetProtection() const
{
	return fProtection;
}

inline off_t Area::GetCacheOffset() const
{
	return fCacheOffset;
}

inline AreaWiring Area::GetWiring() const
{
	return fWiring;
}

#endif
