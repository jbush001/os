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

