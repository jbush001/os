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
