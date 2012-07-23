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

#include "Device.h"
#include "FileDescriptor.h"
#include "FileSystem.h"
#include "List.h"
#include "string.h"
#include "VNode.h"

class DeviceFileSystem;

static DeviceFileSystem *space = 0;

class DeviceEntry : public VNode, public ListNode {
public:
	DeviceEntry(FileSystem*, const char*, size_t);
	virtual ~DeviceEntry();
	const char* GetName() const;
	virtual void Inactive();
	void MakeDead();
private:
	char *fName;
	bool fDead;
};

class DeviceDir : public DeviceEntry {
public:
	DeviceDir(FileSystem*, const char[], size_t, DeviceDir *parentDir);
	virtual ~DeviceDir();
	virtual int Lookup(const char name[], size_t nameLen, VNode **outNode);
	virtual int Open(FileDescriptor **outFile);
	virtual int MakeDir(const char name[], size_t length);
	virtual int RemoveDir(const char name[], size_t length);
	virtual void AddDevice(const char name[], InstantiateHook);
	void SetParentDir(DeviceDir *dir);
	List* GetEntryList();
	
private:
	DeviceEntry* FindEntry(const char name[], int length);

	List fEntryList;
	DeviceDir *fParentDir;
};

class DeviceFile : public DeviceEntry {
public:
	DeviceFile(FileSystem*, const char name[], int length, InstantiateHook hook);
	virtual int Open(FileDescriptor **outFile);
private:
	InstantiateHook fInstantiateHook;
};

class DeviceFileSystem : public FileSystem {
public:
	DeviceFileSystem();
	virtual VNode* GetRootNode();
	void PublishDevice(const char name[], InstantiateHook);
private:
	DeviceDir *fRoot;
};

class DeviceDirFD : public FileDescriptor {
public:
	DeviceDirFD(DeviceDir *dir);
	virtual int ReadDir(char outName[], size_t size);
private:
	DeviceEntry *fEntry;
	List *fEntryList;
};

class DeviceFileDescriptor : public FileDescriptor {
public:
	DeviceFileDescriptor(VNode *node, Device *device);
	virtual ~DeviceFileDescriptor();
	virtual int ReadAt(off_t offs, void *buf, int size);
	virtual int WriteAt(off_t offs, const void *buf, int size);
	virtual int Control(int command, void *buffer);
private:
	Device *fDevice;
};

DeviceEntry::DeviceEntry(FileSystem *fileSystem, const char name[], size_t nameLen)
	:	VNode(fileSystem),
		fDead(false)
{
	fName = new char[nameLen + 1];
	strncpy(fName, name, nameLen);
	fName[nameLen] = '\0';
}

DeviceEntry::~DeviceEntry()
{
	delete [] fName;
}

const char* DeviceEntry::GetName() const
{
	return fName;
}

void DeviceEntry::Inactive()
{
	if (fDead)
		delete this;
}

void DeviceEntry::MakeDead()
{
	fDead = true;
}

DeviceDir::DeviceDir(FileSystem *fileSystem, const char name[], size_t nameLen, DeviceDir *parentDir)
	:	DeviceEntry(fileSystem, name, nameLen),
		fParentDir(parentDir)
{
	// Dummy entries for . and ..
	fEntryList.AddToTail(new DeviceEntry(fileSystem, ".", 1));
	fEntryList.AddToTail(new DeviceEntry(fileSystem, "..", 2));
}

DeviceDir::~DeviceDir()
{
	// delete . and ..
	while (fEntryList.GetHead()) {
		DeviceEntry *entry = (DeviceEntry*) fEntryList.Remove(fEntryList.GetHead());
		delete entry;
	}
}

int DeviceDir::Lookup(const char name[], size_t nameLen, VNode **outNode)
{
	DeviceEntry *entry = FindEntry(name, nameLen);
	if (entry) {
		entry->AcquireRef();
		*outNode = entry;
		return E_NO_ERROR;
	}

	return E_NO_SUCH_FILE;
}

DeviceEntry* DeviceDir::FindEntry(const char name[], int nameLen)
{
	if (nameLen == 1 && name[0] == '.')
		return this;
	
	if (nameLen == 2 && strncmp(name, "..", 2) == 0)
		return fParentDir;

	for (DeviceEntry *entry = (DeviceEntry*) fEntryList.GetHead(); entry;
		entry = (DeviceEntry*) fEntryList.GetNext(entry)) {
		if (strncmp(name, entry->GetName(), nameLen) == 0
			&& (int) strlen(entry->GetName()) == nameLen)
			return entry;
	}

	return 0;
}

int DeviceDir::Open(FileDescriptor **outFile)
{
	*outFile = new DeviceDirFD(this);
	return E_NO_ERROR;
}

int DeviceDir::MakeDir(const char name[], size_t length)
{
	if (FindEntry(name, length))
		return E_ENTRY_EXISTS;

	fEntryList.AddToTail(new DeviceDir(GetFileSystem(), name, length, this));
	return E_NO_ERROR;
}

int DeviceDir::RemoveDir(const char name[], size_t length)
{
	DeviceEntry *entry = FindEntry(name, length);
	if (entry == 0)
		return E_NO_SUCH_FILE;

	if (entry->Lookup(".", 1, (VNode**) &entry) < 0)	// note: acquires reference if successful
		return E_NOT_DIR;

	if (((DeviceDir*) entry)->GetEntryList()->CountItems() != 2) {
		entry->ReleaseRef();
		return E_ENTRY_EXISTS;
	}
		
	fEntryList.Remove(entry);
	entry->MakeDead();
	entry->ReleaseRef();
	return E_NO_ERROR;
}

void DeviceDir::AddDevice(const char name[], InstantiateHook hook)
{
	fEntryList.AddToTail(new DeviceFile(GetFileSystem(), name, strlen(name), hook));
}

void DeviceDir::SetParentDir(DeviceDir *dir)
{
	fParentDir = dir;
}

List* DeviceDir::GetEntryList()
{
	return &fEntryList;
}

extern Device* KeyboardInstantiate();
extern Device* VgaTextInstantiate();
extern Device* VgaGraphicsInstantiate();
extern Device* IdeInstantiate();
extern Device* Ne2000Instantiate();
extern Device* MatroxInstantiate();
extern Device* BeepInstantiate();
extern Device* FloppyInstantiate();

DeviceFileSystem::DeviceFileSystem()
{
	space = this;
	fRoot = new DeviceDir(this, "", 0, 0);
	fRoot->SetParentDir(fRoot);

	// This is a bit of a cop-out for now.  At some point, device loading
	// will be dynamic.
	PublishDevice("input/keyboard", KeyboardInstantiate);
	PublishDevice("display/vga_text", VgaTextInstantiate);
	PublishDevice("display/vga_graphics", VgaGraphicsInstantiate);
	PublishDevice("disk/ide0", IdeInstantiate);
	PublishDevice("disk/floppy", FloppyInstantiate);
	PublishDevice("network/ne2000", Ne2000Instantiate);
	PublishDevice("display/matrox", MatroxInstantiate);
	PublishDevice("sound/beep", BeepInstantiate);
}

VNode* DeviceFileSystem::GetRootNode()
{
	fRoot->AcquireRef();
	return fRoot;
}

void DeviceFileSystem::PublishDevice(const char name[], InstantiateHook hook)
{
	DeviceDir *ent = fRoot;
	for (;;) {
		int len = 0;
		while (name[len] && name[len] != '/')
			len++;
			
		if (!name[len]) {
			ent->AddDevice(name, hook);
			break;
		}	
	
		if (ent->Lookup(name, len, (VNode**) &ent) < 0) {
			ent->MakeDir(name, len);
			ent->Lookup(name, len, (VNode**) &ent);
		}

		name += len + 1;
	}
}

// This is not thread safe
DeviceDirFD::DeviceDirFD(DeviceDir *dir)
	:	FileDescriptor(dir)
{
	fEntryList = dir->GetEntryList();
	fEntry = (DeviceEntry*) fEntryList->GetHead();
}

int DeviceDirFD::ReadDir(char outName[], size_t size)
{
	if (fEntry == 0)
		return -1;

	strncpy(outName, fEntry->GetName(), size);
	fEntry = (DeviceEntry*) fEntryList->GetNext(fEntry);
	return E_NO_ERROR;
}


DeviceFile::DeviceFile(FileSystem *fileSystem, const char name[], int length, InstantiateHook hook)
	:	DeviceEntry(fileSystem, name, length),
		fInstantiateHook(hook)
{
}

int DeviceFile::Open(FileDescriptor **outFile)
{
	*outFile = new DeviceFileDescriptor(this, (*fInstantiateHook)());
	return 0;
}

DeviceFileDescriptor::DeviceFileDescriptor(VNode *node, Device *device)
	:	FileDescriptor(node),
		fDevice(device)
{
	fDevice->AcquireRef();
}

DeviceFileDescriptor::~DeviceFileDescriptor()
{
	fDevice->ReleaseRef();
}

int DeviceFileDescriptor::ReadAt(off_t offs, void *buf, int size)
{
	return fDevice->Read(offs, buf, size);
}

int DeviceFileDescriptor::WriteAt(off_t offs, const void *buf, int size)
{
	return fDevice->Write(offs, buf, size);
}

int DeviceFileDescriptor::Control(int command, void *buffer)
{
	return fDevice->Control(command, buffer);
}

int PublishDevice(const char name[], InstantiateHook hook)
{
	space->PublishDevice(name, hook);
	return E_NO_ERROR;
}

FileSystem *DevFsInstantiate(int)
{
	return new DeviceFileSystem;
}
