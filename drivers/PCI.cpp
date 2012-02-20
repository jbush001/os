#include "cpu_asm.h"
#include "PCI.h"
#include "types.h"

#define CONFIG_ADDRESS 0xcf8
#define CONFIG_DATA 0xcfc

struct ConfigAddress {
	unsigned reg : 8,
		function : 3,
		unit : 5,
		bus : 8,
		reserved : 7,
		enable : 1;
} PACKED;

PCIInterface* PCIInterface::fDevices[255];
int PCIInterface::fDeviceCount = 0;

PCIInterface::PCIInterface()
{
}

PCIInterface::PCIInterface(int bus, int unit)
	:	fBus(bus),
		fUnit(unit)
{
}

PCIInterface* PCIInterface::QueryDevice(int basicClass, int subClass)
{
	if (fDeviceCount == 0)
		Scan();
		
	for (int i = 0; i < fDeviceCount; i++)
		if ((fDevices[i]->fClass >> 24) == basicClass
			&& ((fDevices[i]->fClass >> 16) & 0xff) == subClass)
			return fDevices[i];

	return 0;
}

unsigned PCIInterface::ReadConfig(int bus, int unit, int function, int reg, int bytes)
{
	ConfigAddress address;
	address.enable = 1;
	address.reserved = 0;
	address.bus = bus;
	address.unit = unit;
	address.function = function;
	address.reg = reg & 0xfc;
	
	write_io_32(*(unsigned*)&address, CONFIG_ADDRESS); 
	switch (bytes) {
	case 1:
		return read_io_8(CONFIG_DATA + (reg & 3));
	case 2:
		return read_io_16(CONFIG_DATA + (reg & 3));
	case 4:
		return read_io_32(CONFIG_DATA + (reg & 3));
	}

	return 0;
}

void PCIInterface::WriteConfig(int bus, int unit, int function, int reg, unsigned data,
	int bytes)
{
	ConfigAddress address;
	address.enable = 1;
	address.reserved = 0;
	address.bus = bus;
	address.unit = unit;
	address.function = function;
	address.reg = reg & 0xfc;
	
	write_io_32(*(unsigned*)&address, CONFIG_ADDRESS); 
	switch (bytes) {
	case 1:
		write_io_8(data, CONFIG_DATA + (reg & 3));
	case 2:
		write_io_16(data, CONFIG_DATA + (reg & 3));
	case 4:
		write_io_32(data, CONFIG_DATA + (reg & 3));
	}
}

void PCIInterface::Scan()
{
	for (int bus = 0; bus < 255; bus++) {
		for (int unit = 0; unit < 32; unit++) {
			if (ReadConfig(bus, unit, 0, 0, 2) == 0xffff)
				continue;

			PCIInterface *interface = new PCIInterface(bus, unit);
			interface->fClass = ReadConfig(bus, unit, 0, 8, 4);
			fDevices[fDeviceCount++] = interface;
		}
	}
}

unsigned PCIInterface::ReadConfig(int reg)
{
	return ReadConfig(fBus, fUnit, 0, reg, 4);
}

void PCIInterface::WriteConfig(int reg, unsigned data)
{
	WriteConfig(fBus, fUnit, 0, reg, data, 4);
}


