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

#include "cpu_asm.h"
#include "HandleTable.h"
#include "Semaphore.h"
#include "stdio.h"
#include "string.h"

struct Handle {
	bool IsFree() const;
	int fSerialNumber;
	union {
		Resource *fResource;
		int fNextFree;
	} fDatum;
};

struct SubTable {
	SubTable();
	int fFreeHandles;
	int fFirstFree;
	Handle fHandle[kLevel2Size];
};

bool Handle::IsFree() const
{
	return fDatum.fNextFree < kLevel2Size && fDatum.fNextFree >= 0;
}

SubTable::SubTable()
	: 	fFreeHandles(kLevel2Size)
{
	fFirstFree = kLevel2Size - 1;
	fHandle[0].fDatum.fNextFree = 0;
	fHandle[0].fSerialNumber = 0;
	for (int level2Index = 1; level2Index < kLevel2Size; level2Index++) {
		fHandle[level2Index].fSerialNumber = 0;
		fHandle[level2Index].fDatum.fNextFree = level2Index - 1;
	}
}

HandleTable::HandleTable()
	:	fFreeHint(0)
{
	memset(fMainTable, 0, sizeof(fMainTable));
}

HandleTable::~HandleTable()
{
	// Close handles that are still open
	for (int level1Index = 0; level1Index < kLevel1Size; level1Index++) {
		SubTable *sub = fMainTable[level1Index];
		if (sub == 0)
			continue;
			
		for (int level2Index = 0; level2Index < kLevel2Size; level2Index++)
			if (!sub->fHandle[level2Index].IsFree())
				sub->fHandle[level2Index].fDatum.fResource->ReleaseRef();

		delete sub;
	}
}

int HandleTable::Open(Resource *resource)
{
	resource->AcquireRef();
	
	fLock.LockWrite();
	int level1Index;
	int firstEmpty = -1;
	for (level1Index = fFreeHint; level1Index < kLevel1Size; level1Index++) {
		if (fMainTable[level1Index] == 0) {
			if (firstEmpty == -1)
				firstEmpty = level1Index;
		} else if (fMainTable[level1Index]->fFreeHandles > 0)
			break;
	}

	if (level1Index == kLevel1Size) {
		// All level 2 tables are full.  Add a new one.
		if (firstEmpty == -1)
			panic("Handle table is full");

		fMainTable[firstEmpty] = new SubTable;
		level1Index = firstEmpty;
	}

	fFreeHint = level1Index;
	fMainTable[level1Index]->fFreeHandles--;
	Handle *handles = fMainTable[level1Index]->fHandle;
	int level2Index = fMainTable[level1Index]->fFirstFree;
	fMainTable[level1Index]->fFirstFree = handles[level2Index].fDatum.fNextFree;
	handles[level2Index].fDatum.fResource = resource;
	fLock.UnlockWrite();
	return (((handles[level2Index].fSerialNumber * kLevel2Size) + level1Index)
		* kLevel1Size) + level2Index;
}

void HandleTable::Close(int id)
{
	fLock.LockWrite();
	int level1Index = (id / kLevel2Size) % kLevel1Size;
	int level2Index = id % kLevel2Size;
	fMainTable[level1Index]->fFreeHandles++;
	if (level1Index < fFreeHint)
		fFreeHint = level1Index;
		
	Handle &handle = fMainTable[level1Index]->fHandle[level2Index];
	handle.fSerialNumber = (handle.fSerialNumber + 1) % kSerialNumberCount;
	Resource *resource = handle.fDatum.fResource;
	handle.fDatum.fNextFree = fMainTable[level1Index]->fFirstFree;
	fMainTable[level1Index]->fFirstFree = level2Index;
	fLock.UnlockWrite();

	resource->ReleaseRef();
}

Resource* HandleTable::GetResource(int id, int matchType) const
{
	if (id < 0)
		return 0;
	
	fLock.LockRead();
	int level1Index = (id / kLevel2Size) % kLevel1Size;
	if (fMainTable[level1Index] == 0) {
		fLock.UnlockRead();		
		return 0;
	}

	int level2Index = id % kLevel2Size;
	Handle &handle = fMainTable[level1Index]->fHandle[level2Index];
	if (handle.fSerialNumber != id / (kLevel1Size * kLevel2Size)) {
		fLock.UnlockRead();		
		return 0;
	}

	if (handle.IsFree()) {
		fLock.UnlockRead();		
		return 0;
	}

	if (matchType != OBJ_ANY && matchType != handle.fDatum.fResource->GetType()) {
		fLock.UnlockRead();		
		return 0;
	}

	Resource *resource = handle.fDatum.fResource;
	resource->AcquireRef();
	fLock.UnlockRead();		
	return resource;
}

void HandleTable::Print() const
{
	printf("ID       Type    Ptr      Refcnt Name\n");
	for (int level1Index = 0; level1Index < kLevel1Size; level1Index++) {
		SubTable *array = fMainTable[level1Index];
		if (array == 0)
			continue;

		for (int level2Index = 0; level2Index < kLevel2Size; level2Index++) {
			if (array->fHandle[level2Index].IsFree())
				continue;
				
			Resource *resource = array->fHandle[level2Index].fDatum.fResource;
			int id = level1Index * kLevel1Size + level2Index
				+ array->fHandle[level2Index].fSerialNumber * kLevel1Size *
				kLevel2Size;
			printf("%08x ", id);
			resource->Print();
		}
	}
}
