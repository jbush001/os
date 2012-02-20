#ifndef _VNODE_H
#define _VNODE_H

#include "BackingStore.h"

class FileSystem;
class PageCache;
class FileDescriptor;

class VNode : public BackingStore {
public:
	VNode(FileSystem*);
	virtual ~VNode();
	virtual int Lookup(const char*, size_t, VNode **outNode);
	virtual int Open(FileDescriptor **outFile);
	virtual int MakeDir(const char*, size_t);
	virtual int RemoveDir(const char*, size_t);
	void AcquireRef();
	void ReleaseRef();
	inline FileSystem* GetFileSystem() const;
	inline FileSystem* GetCoveredBy() const;
	void CoverWith(FileSystem*);
	PageCache* GetPageCache();
	virtual void Inactive();
	virtual off_t GetLength();

	int CachedRead(off_t offset, void *data, int size);
	int CachedWrite(off_t offset, const void *data, int size);
	char* CachedOffset(off_t);

	// From BackingStore
	virtual bool HasPage(off_t offset);
	virtual status_t Read(off_t offset, void *va);
	virtual status_t Write(off_t offset, const void *va);
	virtual off_t Commit(off_t size);

private:

	char *fMappedAddress;
	volatile int fRefCount;
	FileSystem *fCoveredBy;
	PageCache *fPageCache;
	class Area *fMapping;
	FileSystem *fFileSystem;
};

inline FileSystem* VNode::GetCoveredBy() const
{
	return fCoveredBy;
}

inline FileSystem* VNode::GetFileSystem() const
{
	return fFileSystem;
}

#endif
