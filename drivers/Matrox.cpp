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

#include "AddressSpace.h"
#include "Area.h"
#include "Device.h"
#include "PCI.h"
#include "stdio.h"

class Matrox : public Device {
public:
	Matrox();
	~Matrox();

private:
	PCIInterface *fCard;
	unsigned *fControlArea;
	char *fFrameBuffer;
};

Matrox::Matrox()
{
	fCard = PCIInterface::QueryDevice(CLASS_VIDEO, 0);
	if (fCard == 0) {
		printf("Couldn't find video card.\n");
		return;
	}
	
	AddressSpace *space = AddressSpace::GetKernelAddressSpace(); 
	fControlArea = (unsigned*) space->MapPhysicalMemory("Matrox Control Aperature",
		fCard->ReadConfig(0x10) & ~(PAGE_SIZE - 1), 0x4000, SYSTEM_WRITE | SYSTEM_READ)
		->GetBaseAddress();
	fFrameBuffer = (char*) space->MapPhysicalMemory("Matrox Frame Buffer",
		fCard->ReadConfig(0x14) & ~(PAGE_SIZE - 1), 0x400000, USER_READ |
		USER_WRITE | SYSTEM_READ | SYSTEM_WRITE)->GetBaseAddress();
}

Matrox::~Matrox()
{
}

Device* MatroxInstantiate()
{
	return new Matrox;
}
