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

#ifndef _SWAP_SPACE_H
#define _SWAP_SPACE_H

#include "BackingStore.h"
#include "Lock.h"

class SwapSpace : public BackingStore {
public:
	SwapSpace();
	virtual ~SwapSpace();
	virtual bool HasPage(off_t);
	virtual status_t Read(off_t, void*);
	virtual status_t Write(off_t, const void*);
	virtual off_t Commit(off_t size);
	static status_t SwapOn(const char path[], off_t size);

private:
	static int AllocSwapSpace();
	static void FreeSwapSpace(int offset);

	int fChunkArraySize;
	struct SwapChunk *fChunkArray;
	Mutex fLock;
	int fCommittedSize;
	static int *fSwapMap;
	static int fSwapDevice;
	static off_t fSwapChunkCount;
	static off_t fFreeSwapChunks;
	static Mutex fSwapLock;
};


#endif
