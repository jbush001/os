#ifndef _DEVICE_H
#define _DEVICE_H

#include "types.h"

class Device {
public:
	Device();
	virtual ~Device();
	void AcquireRef();
	void ReleaseRef();
	virtual int Read(off_t, void*, size_t);
	virtual int Write(off_t, const void*, size_t);
	virtual int Control(int op, void*);

private:
	int fRefCount;
};

typedef Device* (*InstantiateHook)();
extern int PublishDevice(const char name[], InstantiateHook);

#endif
