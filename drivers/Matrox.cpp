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
