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

#include "FileSystem.h"
#include "string.h"
#include "stdio.h"
#include "SwapSpace.h"
#include "syscall.h"
#include "VNode.h"

const int kChunkSize = PAGE_SIZE * 4;

struct SwapChunk {
	int offset : (32 - (kChunkSize / PAGE_SIZE)), // 1 based
		alloc : (kChunkSize / PAGE_SIZE);
};

int *SwapSpace::fSwapMap = 0;
int SwapSpace::fSwapDevice = -1;
off_t SwapSpace::fSwapChunkCount = 0;
Mutex SwapSpace::fSwapLock;
off_t SwapSpace::fFreeSwapChunks = 0x7fffffff;	// Hack

SwapSpace::SwapSpace()
	:	fChunkArraySize(0),
		fChunkArray(0),
		fCommittedSize(0)
{
}

SwapSpace::~SwapSpace()
{
	for (int chunkIndex = 0; chunkIndex < fChunkArraySize; chunkIndex++)
		if (fChunkArray[chunkIndex].offset)
			FreeSwapSpace(fChunkArray[chunkIndex].offset);

	delete [] fChunkArray;
	fSwapLock.Lock();
	fFreeSwapChunks += fCommittedSize;
	fSwapLock.Unlock();
}

bool SwapSpace::HasPage(off_t offset)
{
	int arrayOffset = offset / kChunkSize;
	int chunkOffset = offset % kChunkSize;
	if (arrayOffset >= fChunkArraySize)
		return false;

	return (fChunkArray[arrayOffset].alloc & (1 << (chunkOffset / PAGE_SIZE))) != 0;
}

status_t SwapSpace::Read(off_t offset, void *va)
{
	printf("<");
	if (offset / kChunkSize > fChunkArraySize)
		panic("reading unswapped page");

	off_t deviceOffset = (fChunkArray[offset / kChunkSize].offset - 1) * kChunkSize +
		(offset % kChunkSize);
	return read_pos(fSwapDevice, deviceOffset, va, PAGE_SIZE);
}

status_t SwapSpace::Write(off_t offset, const void *va)
{
	if (fSwapDevice < 0)
		panic("no swap file opened");

	fLock.Lock();
	int arrayOffset = offset / kChunkSize;
	int chunkOffset = offset % kChunkSize;
	if (arrayOffset >= fChunkArraySize) {
		SwapChunk *newChunkArray = new SwapChunk[arrayOffset + 1];
		if (fChunkArray) {
			memcpy(newChunkArray, fChunkArray, fChunkArraySize * sizeof(SwapChunk));
			delete [] fChunkArray;
		}

		fChunkArray = newChunkArray;
		memset(fChunkArray + fChunkArraySize, 0, (arrayOffset - fChunkArraySize + 1) * sizeof(SwapChunk));
		fChunkArraySize = arrayOffset + 1;
	}

	if (fChunkArray[arrayOffset].offset == 0)
		fChunkArray[arrayOffset].offset = AllocSwapSpace();

	fChunkArray[arrayOffset].alloc |= (1 << (chunkOffset / PAGE_SIZE));
	off_t deviceOffset = (fChunkArray[offset / kChunkSize].offset - 1) * kChunkSize +
		(offset % kChunkSize);
	printf(">");
	ASSERT(HasPage(offset));
	fLock.Unlock();
	return write_pos(fSwapDevice, deviceOffset, va, PAGE_SIZE);
}

off_t SwapSpace::Commit(off_t size)
{
	fSwapLock.Lock();
	int delta = (fCommittedSize - size) / kChunkSize;
	if (delta > 0 && delta > fFreeSwapChunks) {
		fSwapLock.Unlock();
		return fCommittedSize;	// Fail
	}

	fFreeSwapChunks += (fCommittedSize - size) / kChunkSize;
	fCommittedSize = size;
	fSwapLock.Unlock();
	return size;
}

status_t SwapSpace::SwapOn(const char file[], off_t size)
{
	fSwapDevice = open(file, 0);
	if (fSwapDevice < 0) {
		printf("Error opening swap device\n");
		return fSwapDevice;
	}

	fSwapMap = new int[size / kChunkSize / 32 + 1];
	memset(fSwapMap, 0, size / kChunkSize / 8);
	fSwapChunkCount = size / kChunkSize;

	return 0;
}

int SwapSpace::AllocSwapSpace()
{
	fSwapLock.Lock();
	int wordCount = fSwapChunkCount / 32;
	for (int wordIndex = 0; wordIndex < wordCount; wordIndex += 32) {
		if (fSwapMap[wordIndex] == static_cast<int>(0xffffffff))
			continue;

		for (int bitIndex = 0; bitIndex < 32; bitIndex++) {
			if ((fSwapMap[wordIndex] & (1 << bitIndex)) == 0) {
				fSwapMap[wordIndex] |= (1 << bitIndex);
				fSwapLock.Unlock();
				return wordIndex + bitIndex + 1;
			}
		}
	}

	panic("Swap space exhausted");
}

void SwapSpace::FreeSwapSpace(int offset)
{
	fSwapLock.Lock();
	fSwapMap[offset / 32] &= ~(1 << (offset % 32));
	fSwapLock.Unlock();
}
