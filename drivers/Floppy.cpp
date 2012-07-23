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
#include "cpu_asm.h"
#include "Device.h"
#include "InterruptHandler.h"
#include "PhysicalMap.h"
#include "Semaphore.h"
#include "stdio.h"
#include "string.h"
#include "syscall.h"
#include "Timer.h"

const bigtime_t kMotorOffTimeout = 3000000;
const int kSectorSize = 512;
const int kSectorsPerTrack = 18;
const int kHeadCount = 2;

enum RegisterOffset {
	kStatusA = 1,
	kStatusB = 1,
	kDigitalOutput = 2,
	kMainStatus = 4,
	kDataRateSelect = 4,
	kData = 5,
	kDigitalInput = 7,
	kConfigurationControl = 7
};

struct FloppyCommand {
	FloppyCommand();
	void SetLBA(int lba);

	uchar id;
	uchar drive;
	uchar cylinder;
	uchar head;
	uchar sector;
	uchar sectorSize;
	uchar trackLength;
	uchar gap3Length;
	uchar dataLength;
};

struct FloppyResult {
	void Print();

	uchar st0;
	uchar st1;
	uchar st2;
	uchar cylinder;
	uchar head;
	uchar sector;
	uchar sectorSize;
};

class Floppy : public Device, public InterruptHandler, public Timer {
public:
	Floppy(int chain, int drive);
	virtual ~Floppy();
	virtual int Read(off_t, void*, size_t);
	virtual int Write(off_t, const void*, size_t);
	virtual int Control(int op, void*);

private:
	void SendCommand(uchar data[], int size);
	void ReadResult(uchar data[], int size);
	void RecalibrateDrive();
	int ReadSector(int lba, unsigned pa);
	virtual InterruptStatus HandleInterrupt();
	void WriteRegister(RegisterOffset, int);
	int ReadRegister(RegisterOffset);
	void Reset();
	void TurnOnMotor();
	void TurnOffMotor();
	void SpinDownMotor();
	virtual InterruptStatus HandleTimeout();
	bool MediaChanged();
	void CheckStatus();

	int fBaseAddress;
	int fDrive;
	Semaphore fCommandComplete;
};

FloppyCommand::FloppyCommand()
{
	memset(this, 0, sizeof(*this));
}

void FloppyCommand::SetLBA(int lba)
{
	cylinder = lba / (kSectorsPerTrack * kHeadCount);
	head = (lba / kSectorsPerTrack) % kHeadCount;
	sector = lba % kSectorsPerTrack + 1;
	drive = drive & ~(1 << 2) | (head << 2);
}

void FloppyResult::Print()
{
	if (st0 & 0xc0) {
		printf("The following error(s) have occured\n");
		if (st0 & 8) printf("  Drive is not ready\n");
		if (st1 & 128) printf("  Read past end of track\n");
		if (st1 & 32) printf("  Data Error\n");
		if (st1 & 16) printf("  DMA transfer timed out\n");
		if (st1 & 4) printf("  Couldn't find data\n");
		if (st1 & 2) printf("  Disk is write protected\n");
		if (st1 & 1) printf("  Could not find ID address mark\n");
		if (st2 & 64) printf("  Deleted address mark\n");
		if (st2 & 32) printf("  CRC error in data\n");
		if (st2 & 16) printf("  Wrong cylinder\n");
		if (st2 & 4) printf("  Seek error\n");
		if (st2 & 2) printf("  ID address mark differs from track address\n");
		if (st2 & 1) printf("  Couldn't data address mark\n");
	} else
		printf("Command completed successfully\n");
}

Floppy::Floppy(int chain, int drive)
	:	fDrive(drive),
		fCommandComplete("Floppy I/O", 0)
{
	if (chain == 0)
		fBaseAddress = 0x3f0;
	else
		fBaseAddress = 0x370;

	ObserveInterrupt(6);
#if 0
	Area *area = AddressSpace::GetKernelAddressSpace()->CreateArea("floppy hack", PAGE_SIZE,
		AREA_WIRED, SYSTEM_READ | SYSTEM_WRITE, new PageCache, 0);
	unsigned pa = AddressSpace::GetKernelAddressSpace()->HackGetPhysicalMap()->GetPhysicalAddress(
		area->GetBaseAddress());

	printf("Status = %02x\n", ReadRegister(kMainStatus));

	// Set up geometry.
	printf("reset controller\n");
	Reset();

	printf("set up geometry\n");
	uchar cmd[3] = { 3, 0xff, 0xfe };
	SendCommand(cmd, 3);
#endif

	printf("Turn on motor\n");
	TurnOnMotor();

	printf("Calibrate drive\n");
	RecalibrateDrive();

	printf("Check status\n");
	CheckStatus();

#if 0
	printf("Read sector\n");
	for (int retry = 0; retry < 5; retry++) {
		if (ReadSector(60, pa) < 0) {
			printf("An error occured\n");
			sleep(50000);
			CheckStatus();
			RecalibrateDrive();
		} else {
			printf("Read from floppy sector 0:\n");
			bindump((uchar*) area->GetBaseAddress(), kSectorSize);
			break;
		}
	}
#endif

	SpinDownMotor();
	printf("\n");
}

Floppy::~Floppy()
{
	IgnoreInterrupts();
	CancelTimeout();
}

int Floppy::Read(off_t, void*, size_t)
{
	TurnOnMotor();
	SpinDownMotor();
	return -1;
}

int Floppy::Write(off_t, const void*, size_t)
{
	TurnOnMotor();
	SpinDownMotor();
	return -1;
}

int Floppy::Control(int, void*)
{
	TurnOnMotor();
	SpinDownMotor();
	return 0;
}

void Floppy::SendCommand(uchar data[], int size)
{
	WriteRegister(kDigitalOutput, (0x10 << fDrive) | 0xc);

	printf("Sending Command [ ");
	for (int i = 0; i < size; i++) {
		while ((ReadRegister(kMainStatus) & 0x80) == 0)
			;

		WriteRegister(kData, data[i]);
		printf("%02x ", data[i]);
	}

	printf("]\n");
}

void Floppy::ReadResult(uchar data[], int size)
{
	printf("Received result [ ");
	for (int i = 0; i < size; i++) {
		while ((ReadRegister(kMainStatus) & 0x80) == 0)
			;

		data[i] = ReadRegister(kData);
		printf("%02x ", data[i]);
	}

	printf("]\n");
}

void Floppy::RecalibrateDrive()
{
	// Move the drive back to cylinder 0.
	printf("recalibrating drive...\n");
	for (int retry = 0; retry < 5; retry++) {
		uchar command[2] = { 7, 0 };
		SendCommand(command, sizeof(command));
//		fCommandComplete.Wait();

		uchar command2 = 8;
		SendCommand(&command2, 1);

		uchar result[2];
		ReadResult(result, 2);
		if (result[1] != 0) {
			if (result[0] & 0xc0)
				printf("errors occured recalibrating drive\n");

			printf("Drive is at cylinder %d, expecting 0\n", result[1]);
		} else
			break;
	}

	printf("Everything is back on track (so to speak)\n");
}

int Floppy::ReadSector(int lba, unsigned physaddr)
{
	enum {
		kDmaAddress = 4,
		kDmaCount = 5,
		kDmaCommand = 8,
		kDmaChannelMask = 0xa,
		kDmaMode = 0xb,
		kDmaFlipFlop = 0xc,
		kDmaPage = 0x81,
	};

	if ((physaddr >> 24) != 0)
		panic("DMA address is out of range (must be < 24M)");


	// Set up the DMA controller to transfer data from the floppy
	// controller.
	write_io_8(0x14, kDmaCommand);	// Disable DMA
	write_io_8(0x56, kDmaMode);
	write_io_8(0, kDmaFlipFlop);
	write_io_8(physaddr & 0xff, kDmaAddress);			// LSB
	write_io_8((physaddr >> 8) & 0xff, kDmaAddress);	// MSB
	write_io_8((physaddr >> 16) & 0xff, kDmaPage);		// Page
	write_io_8(0, kDmaFlipFlop);
	write_io_8(0xff, kDmaCount);	// Count LSB (512 bytes)
	write_io_8(1, kDmaCount);		// Count MSB
	write_io_8(2, kDmaChannelMask);	// Release channel 2
	write_io_8(0x10, kDmaCommand);	// Reenable DMA

	// Send the command
	FloppyCommand command;
	command.id = 0x46;		// Read MFM sector, single head
	command.sectorSize = 2;		// 512 bytes per sector
	command.trackLength = kSectorsPerTrack;
	command.gap3Length = 27;	// For 3.5" floppy
	command.dataLength = 0xff;
	command.SetLBA(lba);

	SendCommand(reinterpret_cast<uchar*>(&command), sizeof(command));
	printf("Wait for command to finish\n");
	fCommandComplete.Wait();
	printf("reading result...\n");
	FloppyResult result;
	ReadResult(reinterpret_cast<uchar*>(&result), sizeof(result));
	result.Print();

	if (result.st0 & 0xc0)
		return -1;

	return kSectorSize;
}

void Floppy::Reset()
{
	WriteRegister(kDigitalOutput, 0);
	fCommandComplete.Wait();
	WriteRegister(kDigitalOutput, 0xc);
}

void Floppy::TurnOnMotor()
{
	if (!CancelTimeout()) {
		printf("Starting up motor\n");
		sleep(500000);
		WriteRegister(kDigitalOutput, (0x10 << fDrive) | 0xc);
	}
}

void Floppy::TurnOffMotor()
{
	printf("About to turn off motor\n");
	WriteRegister(kDigitalOutput, 4);
	printf("Motor is off\n");
}

void Floppy::SpinDownMotor()
{
	CancelTimeout();
	SetTimeout(SystemTime() + kMotorOffTimeout, kOneShotTimer);
}

InterruptStatus Floppy::HandleTimeout()
{
	TurnOffMotor();
	return kHandledInterrupt;
}

InterruptStatus Floppy::HandleInterrupt()
{
	printf("Floppy interrupt\n");
	if (!(ReadRegister(kStatusA) & 128)) {
		printf("Huh... interrupt is not pending\n");
		return kUnhandledInterrupt;
	}

	fCommandComplete.Release(1, false);
	return kReschedule;
}

void Floppy::WriteRegister(RegisterOffset offset, int value)
{
	write_io_8(value, fBaseAddress + offset);
}

int Floppy::ReadRegister(RegisterOffset offset)
{
	return read_io_8(fBaseAddress + offset);
}

bool Floppy::MediaChanged()
{
	return (ReadRegister(kDigitalInput) & 128) != 0;
}

void Floppy::CheckStatus()
{
	uchar command[2] = { 4, 0 };
	uchar result;
	SendCommand(command, sizeof(command));
	fCommandComplete.Wait();
	ReadResult(&result, 1);
	printf("Floppy Status:\n");
	if (result & 8)
		printf("  Double Sided Drive\n");
	else
		printf("  Single Sided Drive\n");

	if (result & 64)
		printf("  The disk is read-only\n");
	else
		printf("  the disk is writeable\n");

	if (result & 0x10)
		printf("Drive is over track 0\n");

	printf("head %d is selected\n", (result >> 2) & 1);
	printf("Drive %d is selected\n", result & 3);

	if (result & 128)
		printf("An error has occured\n");
}

Device* FloppyInstantiate()
{
	static Floppy *gFloppy = 0;
	if (!gFloppy) {
		gFloppy = new Floppy(0, 0);
		gFloppy->AcquireRef();
	}

	return gFloppy;
}
