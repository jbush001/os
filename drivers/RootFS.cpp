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

#include "FileDescriptor.h"
#include "FileSystem.h"
#include "List.h"
#include "string.h"
#include "VNode.h"

class RootFileSystem;

class RootEntry : public VNode, public ListNode {
public:
	RootEntry(FileSystem *fileSystem, const char name[], size_t length);
	virtual ~RootEntry();
	const char* GetName() const;
	virtual void Inactive();
	void MakeDead();
private:
	char *fName;
	bool fDead;
};

class RootDir : public RootEntry {
public:
	RootDir(FileSystem *fileSystem, const char name[], size_t length, RootDir *parentDir);
	virtual ~RootDir();
	virtual int Lookup(const char name[], size_t nameLen, VNode **outNode);
	virtual int Open(FileDescriptor **outFile);
	virtual int MakeDir(const char name[], size_t length);
	virtual int RemoveDir(const char name[], size_t length);
	void SetParentDir(RootDir *dir);
	List* GetEntryList();
	
private:
	RootEntry *FindEntry(const char name[], int len);

	List fEntryList;
	RootDir *fParentDir;
};

class RootFileSystem : public FileSystem {
public:
	RootFileSystem();
	virtual VNode* GetRootNode();
private:
	RootDir *fRoot;
};

class RootDirFD : public FileDescriptor {
public:
	RootDirFD(RootDir *dir);
	virtual int ReadDir(char outName[], size_t size);
private:
	RootEntry *fEntry;
	List *fEntryList;
};

RootEntry::RootEntry(FileSystem *fileSystem, const char name[], size_t nameLen)
	:	VNode(fileSystem),
		fDead(false)
{
	fName = new char[nameLen + 1];
	strncpy(fName, name, nameLen);
	fName[nameLen] = '\0';
}

RootEntry::~RootEntry()
{
	delete [] fName;
}

const char* RootEntry::GetName() const
{
	return fName;
}

void RootEntry::Inactive()
{
	if (fDead)
		delete this;
}

void RootEntry::MakeDead()
{
	fDead = true;
}

RootDir::RootDir(FileSystem *fileSystem, const char name[], size_t nameLen, RootDir *parentDir)
	:	RootEntry(fileSystem, name, nameLen),
		fParentDir(parentDir)
{
	// Dummy entries for . and ..
	fEntryList.AddToTail(new RootEntry(fileSystem, ".", 1));
	fEntryList.AddToTail(new RootEntry(fileSystem, "..", 2));
}

RootDir::~RootDir()
{
	// delete . and ..
	while (fEntryList.GetHead()) {
		RootEntry *entry = (RootEntry*) fEntryList.Remove(fEntryList.GetHead());
		delete entry;
	}
}

int RootDir::Lookup(const char name[], size_t nameLen, VNode **outNode)
{
	RootEntry *entry = FindEntry(name, nameLen);
	if (entry) {
		entry->AcquireRef();
		*outNode = entry;
		return E_NO_ERROR;
	}

	return E_NO_SUCH_FILE;
}

RootEntry* RootDir::FindEntry(const char name[], int length)
{
	if (length == 1 && name[0] == '.')
		return this;
	
	if (length == 2 && strncmp(name, "..", 2) == 0)
		return fParentDir;

	for (RootEntry *entry = (RootEntry*) fEntryList.GetHead(); entry;
		entry = (RootEntry*) fEntryList.GetNext(entry))
		if (strncmp(name, entry->GetName(), length) == 0
			&& (int) strlen(entry->GetName()) == length)
			return entry;

	return 0;
}

int RootDir::Open(FileDescriptor **outFile)
{
	*outFile = new RootDirFD(this);
	return E_NO_ERROR;
}

int RootDir::MakeDir(const char name[], size_t length)
{
	if (FindEntry(name, length))
		return E_ENTRY_EXISTS;
		
	fEntryList.AddToTail(new RootDir(GetFileSystem(), name, length, this));
	return E_NO_ERROR;
}

int RootDir::RemoveDir(const char name[], size_t length)
{
	RootEntry *entry = FindEntry(name, length);
	if (entry == 0)
		return E_NO_SUCH_FILE;

	if (entry->Lookup(".", 1, (VNode**) &entry) < 0)	// note: acquires reference if successful
		return E_NOT_DIR;
		
	if (((RootDir*)entry)->GetEntryList()->CountItems() != 2) {
		entry->ReleaseRef();
		return E_ENTRY_EXISTS;
	}
		
	fEntryList.Remove(entry);
	entry->MakeDead();
	entry->ReleaseRef();
	return E_NO_ERROR;
}

void RootDir::SetParentDir(RootDir *dir)
{
	fParentDir = dir;
}

List* RootDir::GetEntryList()
{
	return &fEntryList;
}

RootFileSystem::RootFileSystem()
{
	fRoot = new RootDir(this, "", 0, 0);
	fRoot->SetParentDir(fRoot);
}

VNode* RootFileSystem::GetRootNode()
{
	fRoot->AcquireRef();
	return fRoot;
}

// This is not thread safe
RootDirFD::RootDirFD(RootDir *dir)
	:	FileDescriptor(dir)
{
	fEntryList = dir->GetEntryList();
	fEntry = (RootEntry*) fEntryList->GetHead();
}

int RootDirFD::ReadDir(char outName[], size_t size)
{
	if (fEntry == 0)
		return -1;

	strncpy(outName, fEntry->GetName(), size);
	fEntry = (RootEntry*) fEntryList->GetNext(fEntry);
	return E_NO_ERROR;
}

FileSystem *RootFsInstantiate(int)
{
	return new RootFileSystem;
}
