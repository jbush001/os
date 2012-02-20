#include "cpu_asm.h"
#include "Device.h"

Device::Device()
{
}

Device::~Device()
{
}

void Device::AcquireRef()
{
	AtomicAdd(&fRefCount, 1);
}

void Device::ReleaseRef()
{
	if (AtomicAdd(&fRefCount, -1) == 1)
		delete this;
}

int Device::Read(off_t, void*, size_t)
{
	return E_INVALID_OPERATION;
}

int Device::Write(off_t, const void*, size_t)
{
	return E_INVALID_OPERATION;
}

int Device::Control(int, void*)
{
	return E_INVALID_OPERATION;
}
