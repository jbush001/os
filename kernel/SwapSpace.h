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
