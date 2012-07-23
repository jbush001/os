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

#ifndef _PCI_H
#define _PCI_H

class PCIInterface {
public:
	static PCIInterface* QueryDevice(int basicClass, int subClass);
	unsigned ReadConfig(int reg);
	void WriteConfig(int reg, unsigned data);

private:
	PCIInterface();
	PCIInterface(int bus, int device);
	static void Scan();
	static unsigned ReadConfig(int bus, int unit, int function, int reg, int bytes);
	static void WriteConfig(int bus, int unit, int function, int reg, unsigned d, int bytes);

	int fClass;
	int fBus;
	int fUnit;

	static PCIInterface* fDevices[255];
	static int fDeviceCount;
};

#define CLASS_LEGACY 0
#define CLASS_MASS_STORAGE 1
#define CLASS_NETWORK 2
#define CLASS_VIDEO 3
#define CLASS_MULTIMEDIA 4
#define CLASS_MEMORY 5
#define CLASS_BRIDGE 6

#define SUBCLASS_SCSI 0
#define SUBCLASS_IDE 1
#define SUBCLASS_FLOPPY 2
#define SUBCLASS_IPI 3
#define SUBCLASS_ETHERNET 0
#define SUBCLASS_TOKEN_RING 1
#define SUBCLASS_FDDI 2
#define SUBCLASS_VGA 0
#define SUBCLASS_XGA 1
#define SUBCLASS_RAM 1
#define SUBCLASS_HOST 0
#define SUBCLASS_ISA 1
#define SUBCLASS_EISA 2
#define SUBCLASS_MCA 3
#define SUBCLASS_PCIPCI 4
#define SUBCLASS_PCMCIA 5
#define SUBCLASS_OTHER 0x80

#endif
