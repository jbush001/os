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
