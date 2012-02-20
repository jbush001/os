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
