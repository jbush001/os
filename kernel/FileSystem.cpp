#include "cpu_asm.h"
#include "FileSystem.h"
#include "List.h"
#include "string.h"
#include "syscall.h"
#include "Thread.h"
#include "VNode.h"

class FsType : public ListNode {
public:
	char name[OS_NAME_LENGTH];
	FileSystem* (*Instantiate)(int deviceFD);
};

extern FileSystem* RootFsInstantiate(int);
extern FileSystem* NetbootFsInstantiate(int);
extern FileSystem* DevFsInstantiate(int);
extern FileSystem* FatFsInstantiate(int); 

FileSystem* FileSystem::fRootFileSystem = 0;
List FileSystem::fFsTypeList;

FileSystem::FileSystem()
	:	fCovers(0)
{
}

FileSystem::~FileSystem()
{
	panic("unmount not implemented\n");
}

VNode* FileSystem::GetRootNode()
{
	return 0;
}

void FileSystem::Cover(VNode *covers)
{
	fCovers = covers;
}

void FileSystem::RegisterFsType(const char name[], FileSystem* (*Instantiate)(int devfd))
{
	FsType *type = new FsType;
	if (type == 0)
		panic("OOM registering type");
	
	strncpy(type->name, name, OS_NAME_LENGTH);
	type->Instantiate = Instantiate;

	cpu_flags fl = DisableInterrupts();
	fFsTypeList.AddToTail(type);
	RestoreInterrupts(fl);
}

status_t FileSystem::InstantiateFsType(const char type[], int devfd, FileSystem **space)
{
	cpu_flags fl = DisableInterrupts();
	const FsType *fsType;
	for (fsType = static_cast<const FsType*>(fFsTypeList.GetHead()); fsType;
		fsType = static_cast<const FsType*>(fFsTypeList.GetNext(fsType)))
		if (strcmp(type, fsType->name) == 0)
			break;
			
	RestoreInterrupts(fl);
	if (fsType == 0)
		return E_INVALID_OPERATION; // This filesystem type is not registered.

	*space = (*fsType->Instantiate)(devfd);
	return E_NO_ERROR;
}

int FileSystem::WalkPath(const char path[], int pathLength, VNode **outNode)
{
	VNode *currentNode;
	int index = 0;
	int error = 0;

	if (path[0] == '/' || Thread::GetRunningThread()->GetCurrentDir() == 0)
		currentNode = fRootFileSystem->GetRootNode();
	else
		currentNode = Thread::GetRunningThread()->GetCurrentDir();

	currentNode->AcquireRef();

	int nameLen;
	for (index = 0; index < pathLength; index += nameLen) {
		// Scan to the start of the next entry name.
		while (index < pathLength && path[index] == '/')
			index++;

		// Find the length of this entry.
		nameLen = 0;
		while (index + nameLen < pathLength && path[index + nameLen] != '/')
			nameLen++;

		if (strncmp(path + index, "..", 2) == 0 && nameLen == 2) {
			if (currentNode == currentNode->GetFileSystem()->GetRootNode()
				&& currentNode->GetFileSystem()->fCovers != 0) {
				// This is the root of this filesystem.  Walk back into the
				// previous filesystem.
				VNode *prevNode = currentNode->GetFileSystem()->fCovers;
				currentNode->ReleaseRef();
				prevNode->AcquireRef();
				currentNode = prevNode;

				// Falls through.
				// The next bit of code will look up the parent of the
				// directory that is being used as the mount point.
			}
		}

		// Call into the filesystem to resolve this entry
		VNode *nextNode;
		if (nameLen != 0)
			error = currentNode->Lookup(path + index, nameLen, &nextNode);
		else
			error = currentNode->Lookup(".", 1, &nextNode);

		if (error < 0)
			break;
		
		currentNode->ReleaseRef();
		if (nextNode->GetCoveredBy()) {
			// This directory is mounted, walk into the new filesystem.
			currentNode = nextNode->GetCoveredBy()->GetRootNode();
			currentNode->AcquireRef();
			nextNode->ReleaseRef();
		} else
			currentNode = nextNode;
	}

	if (error == E_NO_ERROR) 
		*outNode = currentNode;	

	return error;
}

int FileSystem::GetDirEntry(const char path[], int pathLength, char outEntry[], int entryLen,
	VNode **outNode)
{
	int entryStart;
	for (entryStart = pathLength - 1; entryStart > 0; entryStart--)
		if (path[entryStart - 1] == '/')
			break;

	int copySize = pathLength - entryStart;
	if (copySize > entryLen)
		copySize = entryLen;

	strncpy(outEntry, path + entryStart, copySize);
	outEntry[copySize] = 0;
	return WalkPath(path, entryStart, outNode);
}

void FileSystem::Bootstrap()
{
	fRootFileSystem = RootFsInstantiate(-1);	

	FileSystem::RegisterFsType("netbootfs", NetbootFsInstantiate);
	FileSystem::RegisterFsType("devfs", DevFsInstantiate);
	FileSystem::RegisterFsType("msdos", FatFsInstantiate);
	mkdir("/boot", 0);
	mount("", "/boot", "netbootfs", 0, 0);
	mkdir("/dev", 0);
	mount("", "/dev", "devfs", 0, 0);
}
