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

#ifndef _FILE_DESCRIPTOR_H
#define _FILE_DESCRIPTOR_H

#include "Resource.h"

class VNode;

class FileDescriptor : public Resource {
public:
	FileDescriptor(VNode*);
	virtual ~FileDescriptor();
	VNode* GetNode() const;
	off_t Seek(off_t, int whence);
	virtual int ReadDir(char outName[], size_t size);
	virtual int RewindDir(); 
	virtual int ReadAt(off_t, void*, int);
	virtual int WriteAt(off_t, const void*, int);
	virtual int Read(void*, int);
	virtual int Write(const void*, int);
	virtual int Control(int op, void*);

private:
	VNode *fNode;
	off_t fCurrentPosition;
};

#endif
