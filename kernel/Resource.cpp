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
#include "KernelDebug.h"
#include "Resource.h"
#include "stdio.h"
#include "string.h"

Resource::Resource(ResourceType type, const char name[])
	:	fType(type),
		fRefCount(0)
{
	SetName(name);
}

Resource::~Resource()
{
	ASSERT(fRefCount == 0);
}

const char* Resource::GetName() const
{
	return fName;
}

void Resource::SetName(const char name[])
{
	/// @bug use strlcpy
	strncpy(fName, name, OS_NAME_LENGTH - 1);
	fName[OS_NAME_LENGTH - 1] = '\0';
}

void Resource::AcquireRef()
{
	AtomicAdd(&fRefCount, 1);
}

void Resource::ReleaseRef()
{
	ASSERT(fRefCount > 0);
	if (AtomicAdd(&fRefCount, -1) == 1)
		delete this;
}

void Resource::Print() const
{
	const char *kTypeNames[] = {"Sem", "Team", "Thread", "Area", "FD", "Image"};
	printf("%7s %p %6d %20s\n", kTypeNames[fType], this, fRefCount, fName);
}

