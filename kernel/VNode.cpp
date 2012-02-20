#include "AddressSpace.h"
#include "Area.h"
#include "cpu_asm.h"
#include "KernelDebug.h"
#include "PageCache.h"
#include "SystemCall.h"
#include "VNode.h"

VNode::VNode(FileSystem *fileSystem)
	:	fMappedAddress(0),
		fRefCount(0),
		fCoveredBy(0),
		fPageCache(0),
		fMapping(0),
		fFileSystem(fileSystem)
{
}

VNode::~VNode()
{
	if (fPageCache)
		fPageCache->ReleaseRef();
}

int VNode::Lookup(const char[], size_t, VNode**)
{
	return E_INVALID_OPERATION;
}

int VNode::Open(FileDescriptor**)
{
	return E_INVALID_OPERATION;
}

int VNode::MakeDir(const char[], size_t)
{
	return E_INVALID_OPERATION;
}

int VNode::RemoveDir(const char[], size_t)
{
	return E_INVALID_OPERATION;
}

void VNode::AcquireRef()
{
	AtomicAdd(&fRefCount, 1);
}

void VNode::ReleaseRef()
{
	// Need more locking here.
	if (AtomicAdd(&fRefCount, -1) - 1 == 0)
		Inactive();
}

PageCache* VNode::GetPageCache()
{
	if (fPageCache == 0) {
		fPageCache = new PageCache(this);
		fPageCache->AcquireRef();
	}

	return fPageCache;
}

void VNode::CoverWith(FileSystem *fileSystem)
{
	fCoveredBy = fileSystem;
}

void VNode::Inactive()
{
}

off_t VNode::GetLength()
{
	return 0;
}

int VNode::CachedRead(off_t offset, void *data, int count)
{
	off_t len = GetLength();
	if (offset > len)
		return E_ERROR;
	
	if (offset + count > len)
		count = len - offset;

	if (!CopyUser(data, CachedOffset(offset), count))
		return E_BAD_ADDRESS;

	return count;
}

int VNode::CachedWrite(off_t offset, const void *data, int count)
{
	off_t len = GetLength();
	if (offset > len)
		return E_ERROR;
	
	if (offset + count > len)
		count = len - offset;

	if (!CopyUser(CachedOffset(offset), data, count))
		return E_BAD_ADDRESS;

	return count;
}

char* VNode::CachedOffset(off_t offset)
{
	// xxx Locking
	if (fMappedAddress == 0) {
		fMapping = AddressSpace::GetKernelAddressSpace()->
			CreateArea("Cached File", (GetLength() + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1),
			AREA_NOT_WIRED, SYSTEM_READ | SYSTEM_WRITE, GetPageCache(), 0, INVALID_PAGE, 0);
		if (fMapping == 0)
			panic("Can't create cache view: out of virtual space");
		fMappedAddress = reinterpret_cast<char*>(fMapping->GetBaseAddress());
	}
	
	return fMappedAddress + offset;
}

bool VNode::HasPage(off_t)
{
	return true;
}

status_t VNode::Read(off_t, void*)
{
	return E_ERROR;
}

status_t VNode::Write(off_t, const void*)
{
	return E_ERROR;
}

off_t VNode::Commit(off_t size)
{
	return size;
}

