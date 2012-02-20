#ifndef _FILE_SYSTEM_H
#define _FILE_SYSTEM_H

#include "types.h"

class VNode;

class FileSystem {
public:
	FileSystem();
	virtual ~FileSystem();
	virtual VNode* GetRootNode() = 0;
	void Cover(VNode*);
	static void RegisterFsType(const char name[], FileSystem* (*Instantiate)(int devfd));
	static status_t InstantiateFsType(const char name[], int devfd, FileSystem**);
	static int WalkPath(const char path[], int pathLength, VNode **outNode);
	static int GetDirEntry(const char path[], int pathLength, char outEntry[], int entryLen,
		VNode **outNode);
	static void Bootstrap();

private:
	VNode *fCovers;
	static class List fFsTypeList;
	static FileSystem *fRootFileSystem;
};

#endif
