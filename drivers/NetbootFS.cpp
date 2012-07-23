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

#include "AddressSpace.h"
#include "Area.h"
#include "FileDescriptor.h"
#include "FileSystem.h"
#include "netboot.h"
#include "string.h"
#include "VNode.h"

const boot_entry *bootDir = 0;

class BootDir;

class NetbootFileSystem : public FileSystem {
public:
	NetbootFileSystem();
	virtual VNode* GetRootNode();
private:
	BootDir *fRoot;
};

class BootEntry : public VNode {
public:
	BootEntry(FileSystem*, int index);
	virtual int Open(FileDescriptor **outFile);
	virtual bool HasPage(off_t offset);
	virtual status_t Read(off_t offset, void *va);
	virtual off_t GetLength();
private:
	int fBootDirIndex;
	char *fStart;
	int fSize;
};

class BootDir : public VNode {
public:
	BootDir(FileSystem*);
	virtual int Lookup(const char name[], size_t nameLen, VNode **outNode);
	virtual int Open(FileDescriptor **outFile);
private:
	BootEntry *fBootDirEntries[NETBOOT_DIR_SIZE];
};

class BootDirFD : public FileDescriptor {
public:
	BootDirFD(VNode *node);
	virtual int ReadDir(char outName[], size_t size);
private:
	int fCurrentIndex;
};

class BootItemFD : public FileDescriptor {
public:
	BootItemFD(VNode *node);
};

NetbootFileSystem::NetbootFileSystem()
{
	bootDir = (boot_entry*) AddressSpace::GetKernelAddressSpace()->MapPhysicalMemory(
		"Netboot Files", 0x100000, 0x80000, SYSTEM_READ)->GetBaseAddress();
	fRoot = new BootDir(this);
}

VNode* NetbootFileSystem::GetRootNode()
{
	fRoot->AcquireRef();
	return fRoot;
}

BootDir::BootDir(FileSystem *fileSystem)
	:	VNode(fileSystem)
{
	// Note: start at 1, skip the boot directory
	for (int i = 1; i < NETBOOT_DIR_SIZE; i++)
		fBootDirEntries[i] = new BootEntry(fileSystem, i);
}

int BootDir::Lookup(const char name[], size_t nameLen, VNode **outNode)
{
	if (nameLen == 1 && name[0] == '.') {
		AcquireRef();
		*outNode = this;
		return 0;	
	}

	for (int i = 1; i < NETBOOT_DIR_SIZE; i++) {
		if (bootDir[i].be_type == BE_TYPE_NONE)
			break;
			
		if (strcmp(name, bootDir[i].be_name) == 0) {
			*outNode = fBootDirEntries[i];
			(*outNode)->AcquireRef();
			return 0;
		}
	}
	
	return E_NO_SUCH_FILE;
}

int BootDir::Open(FileDescriptor **outFile)
{
	*outFile = new BootDirFD(this);
	return E_NO_ERROR;
}

BootEntry::BootEntry(FileSystem *fileSystem, int index)
	:	VNode(fileSystem),
		fStart((char*)((unsigned) bootDir[index].be_offset * PAGE_SIZE +
			(char*) bootDir)),
		fSize(bootDir[index].be_size * PAGE_SIZE)
{
}

int BootEntry::Open(FileDescriptor **outFile)
{
	*outFile = new BootItemFD(this);
	return 0;
}

bool BootEntry::HasPage(off_t offset)
{
	return offset < fSize;
}

status_t BootEntry::Read(off_t offset, void *va)
{
	if (offset >= fSize)
		return E_ERROR;

	int copySize = MIN(fSize - (unsigned) offset, PAGE_SIZE);
	memcpy(va, fStart + offset, copySize);
	return copySize;
}

off_t BootEntry::GetLength()
{
	return fSize;
}

BootDirFD::BootDirFD(VNode *node)
	:	FileDescriptor(node),
		fCurrentIndex(-2)
{
}

int BootDirFD::ReadDir(char outName[], size_t size)
{
	// Hack-o-rama
	if (fCurrentIndex == -2) {
		fCurrentIndex++;
		strcpy(outName, ".");
		return 0;
	}
	
	if (fCurrentIndex == -1) {
		fCurrentIndex++;
		strcpy(outName, "..");
		return 0;
	}

	if (bootDir[fCurrentIndex].be_type == BE_TYPE_NONE)
		return -1;
	
	if (fCurrentIndex == 0)
		fCurrentIndex++;	// Skip directory

	strncpy(outName, bootDir[fCurrentIndex++].be_name, size);
	return 0;
}

BootItemFD::BootItemFD(VNode *node)
	:	FileDescriptor(node)
{
}


FileSystem *NetbootFsInstantiate(int)
{
	return new NetbootFileSystem;
}
