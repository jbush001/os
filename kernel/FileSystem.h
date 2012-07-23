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
