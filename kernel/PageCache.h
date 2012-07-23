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

/// @file PageCache
#ifndef _PAGE_CACHE_H
#define _PAGE_CACHE_H

#include "types.h"

class BackingStore;
class Page;

/// A PageCache represents a collection of physical memory pages that contain
/// a subset of data from a BackingStore object.  In our model, all of physical memory
/// is simply a cache of data stored on some backing store.  For example, parts of a
/// file from a disk drive.
class PageCache {
public:
	PageCache(BackingStore *backingStore = 0, PageCache *copyOf = 0);
	~PageCache();

	/// Return a physical Page that contains data from a specific offset in the backing store.
	/// If a physical page already exists in memory that contains this data, it will be returned.
	/// However, if there is no physical page, one will be allocated (potentially taken from
	/// another PageCache), and data will be loaded onto it from the BackingStore
	/// @param offset Number of bytes into backing store from which data should be retrieved
	/// @param privateCopy
	///    - If this is false, then any thread that calls GetPage with the same offset will potentially
	///       get a pointer to the same Page object (they will share the physical page)
	///    - If this is true, then the returned Page will belong to the calling thread alone.  Note
	///       that this method is only called with privateCopy true from another PageCache (this is used to
	///       implement copy on write)
	/// @returns Page containing requested data
	Page* GetPage(off_t offset, bool privateCopy = false);

	/// The virtual memory system needs to reuse a page that is in this cache.  The cache
	/// must relenquish ownership of this page.
	/// @param page Page to be removed from this cache.  It necessarily will be a member of this
	///    cache.
	/// @param dirty true if the virtual memory detects that the contents have been modified since
	///    GetPage was called.  If it is dirty, the page cache will inform the backing store that it should
	///    write out the new version of the data.
	void StealPage(Page *page, bool dirty);

	/// Determine if this page cache is copy on write and receives unmodified pages from
	/// another cache.
	inline bool IsCopy() const;

	/// Increment the reference count of this object.
	void AcquireRef();

	/// Decrement the reference count of this object.  If the reference count goes to zero, free
	/// this object and release all of its pages
	void ReleaseRef();

	/// Commit guarantees that a certain amount of space on the backing store will be available
	/// @param size Number of bytes that will be required (generally a multiple of the page size)
	/// @returns Number of bytes that were actually resedved.
	off_t Commit(off_t size);

	/// Prevent pages in this cache from being swapped in our out by other threads
	/// This will block of other threads are interacting with the cache.
	void Lock();

	/// Opposite of lock
	void Unlock();

	/// Called at boot time to initialize structures
	static void Bootstrap();

	/// Print all pages in this cache to debug output.
	void Print() const;

private:
	inline int GenerateHash(off_t) const;
	void InsertPage(off_t, Page*);
	void RemovePage(Page*);
	Page* LookupPage(off_t) const;
	static void HashStats(int, const char**);

	BackingStore *fBackingStore;
	PageCache *fSourceCache;
	Page *fResidentPages;
	volatile int fRefCount;
	static int fPageHashSize;
	static Page **fPageHash;
	static class Mutex fCacheLock;
};

inline bool PageCache::IsCopy() const
{
	return fSourceCache != 0;
}

#endif
