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

/// @file BackingStore.h

#ifndef _BACKING_STORE_H
#define _BACKING_STORE_H

#include "types.h"

/// A backing store is a place where data is stored that is not physical memory.
/// It provides IO operations on this space.  It can represent data from a file
/// or an anonymous chunk of swap space.
class BackingStore {
public:
	virtual ~BackingStore();

	/// Returns whether there is valid data to be read from the specified offset in the backing store.
	/// For a file, this will always return true.  For swap space, this will return false if the address
	/// has never been written.  If this returns false, the virtual memory will simply zero out a
	/// physical memory page, skipping the task of reading meaningless data from the disk.
	/// @param offset Offset in bytes into this backing store
	/// @returns true if there is data to be read.
	virtual bool HasPage(off_t offset) = 0;

	/// Read data from the specified physical device into memory
	/// @param offset Offset in bytes into this backing store
	/// @param va Virtual address of place to copy a physical page worth of data (the virtual
	///   memory system has kindly mapped it for you already)
	virtual status_t Read(off_t offset, void *va) = 0;

	/// Write data to the specified physical device from memory
	/// @param offset Offset in bytes into this backing store
	/// @param va Virtual address of place to copy a physical page worth of data.
	virtual status_t Write(off_t offset, const void *va) = 0;

	/// Guarantee that a certain amount of data will be able to be written to the backing store
	/// @returns Number of bytes actually available.
	virtual off_t Commit(off_t size) = 0;
};

inline BackingStore::~BackingStore()
{
}

#endif
