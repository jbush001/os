#include "cpu_asm.h"
#include "Device.h"
#include "Semaphore.h"
#include "stdio.h"
#include "string.h"
#include "syscall.h"

enum InterruptStatusFlags {
	kPacketReceived = 1,
	kPacketTransmitted = 2,
	kReceiveError = 4,
	kTransmitError = 8,
	kOverwriteWarning = 0x10,
	kCounterOverflow = 0x20,
	kRemoteDMAComplete = 0x40,
	kResetStatus = 0x80,
};

enum CommandFlags {
	kCommandPage0 = 0,
	kCommandStop = 1,
	kCommandStart = 2,
	kCommandTransmit = 4,
	kCommandRead = 8,
	kCommandWrite = 0x10,
	kCommandDmaDisable = 0x20,
	kCommandPage1 = 0x40,
	kCommandPage2 = 0x80
};

enum Ne2kRegister {
	kCommandRegister = 0,
	kPhysicalAddressRegister = 1,
	kPageStartRegister = 1,
	kPageStopRegister = 2,
	kBoundaryRegister = 3,
	kTransmitPageRegister = 4,
	kTransmitByteCount0 = 5,
	kTransmitByteCount1 = 6,
	kCurrentPageRegister = 7,
	kInterruptStatusRegister = 7,
	kMulticastRegister = 8,
	kRemoteStart0Register = 8,
	kRemoteStart1Register = 9,
	kRemoteCount0Register = 0xa,
	kRemoteCount1Register = 0xb,
	kReceiveConfigRegister = 0xc,
	kReceiveStatusRegister = 0xc,
	kTransmitConfigRegister = 0xd,
	kDataConfigRegister = 0xe,
	kInterruptMaskRegister = 0xf,
	kDataRegister = 0x10,
	kResetRegister = 0x1f
};

const int kRingPageSize = 256;
const size_t kMtu = 1550;

class Ne2000 : public Device, public InterruptHandler {
public:
	Ne2000(int base, int irq);
	virtual ~Ne2000();
	virtual int Read(off_t offset, void *buffer, size_t size);
	virtual int Write(off_t offset, const void *buffer, size_t size);
	virtual int Control(int op, void *data);

private:
	virtual InterruptStatus HandleInterrupt();
	void Reset();
	void Initialize();
	inline void WriteRegister(int value, Ne2kRegister);
	inline int ReadRegister(Ne2kRegister);
	void WriteCardMemory(int cardAddress, const void *data, int size);
	void ReadCardMemory(int cardAddress, void *data, int size);
	void HandleRingOverflow();

	int fBaseAddress;
	int fNextReceivePacket;
	int fWordLength;
	Semaphore fTransmitReady;
	Semaphore fReceiveReady;
	int fReceiveRingStart;
	int fReceiveRingEnd;
	int fTransmitRingStart;
	char fMacAddress[6];
};

Ne2000::Ne2000(int base, int irq)
	:	fBaseAddress(base),
		fNextReceivePacket(0),
		fWordLength(1),
		fTransmitReady("transmit_ready"),
		fReceiveReady("packets_queued", 0)
{
	ObserveInterrupt(irq);
	Initialize();
}

Ne2000::~Ne2000()
{
	IgnoreInterrupts();
}

inline void Ne2000::WriteRegister(int value, Ne2kRegister reg)
{
	write_io_8(value, fBaseAddress + reg);
}

inline int Ne2000::ReadRegister(Ne2kRegister reg)
{
	return read_io_8(fBaseAddress + reg);
}

int Ne2000::Read(off_t, void *buffer, size_t count)
{
	cpu_flags fl = DisableInterrupts();

	// Wait for packets.
	fReceiveReady.Wait();
	if (ReadRegister(kInterruptStatusRegister) & kOverwriteWarning) {
		HandleRingOverflow();
		return -1;
	}

	// Grab the card header for this packet.
	struct ReceiveBufferHeader {
		unsigned char status;
		unsigned char nextPacket;
		unsigned short receiveCount;		
	} header;
	ReadCardMemory(fNextReceivePacket * kRingPageSize, &header, sizeof(header));
	if (header.receiveCount > count)
		panic("need bigger buffer: %08x\n", header.receiveCount);

	// Read the body of the packet.
	ReadCardMemory(fNextReceivePacket * kRingPageSize + sizeof(header), buffer,
		header.receiveCount - sizeof(header));
		
	// Update to the next packet.
	fNextReceivePacket = header.nextPacket;
	WriteRegister(fNextReceivePacket == fReceiveRingStart ? fReceiveRingEnd
		: fNextReceivePacket - 1, kBoundaryRegister);
	RestoreInterrupts(fl);

	return header.receiveCount - sizeof(header);
}

int Ne2000::Write(off_t, const void *buffer, size_t size)
{
	if (size > kMtu) {
		printf("Oversized packet, truncating.\n");
		size = kMtu;
	}

	if (size < 64) {
		// The hardware requires that the packet be at least this long,
		// based on the physics of sending it on the wire.
		printf("Undersized packet, dropping\n");
		return -1;
	}

	cpu_flags fl = DisableInterrupts();
	fTransmitReady.Wait();
	WriteCardMemory(fTransmitRingStart * kRingPageSize, buffer, size);
	WriteRegister(fTransmitRingStart, kTransmitPageRegister);
	WriteRegister(size >> 8, kTransmitByteCount0);
	WriteRegister(size & 0xff, kTransmitByteCount1);
	WriteRegister(kCommandTransmit | kCommandDmaDisable, kCommandRegister);
	RestoreInterrupts(fl);

	return size;
}

int Ne2000::Control(int op, void *data)
{
	switch (op) {
		case 1:	// Get MAC Address
			memcpy(data, fMacAddress, sizeof(fMacAddress));
			break;
			
		default:
			return E_INVALID_OPERATION;
	}

	return E_NO_ERROR;
}

InterruptStatus Ne2000::HandleInterrupt()
{
	int events = ReadRegister(kInterruptStatusRegister);
	if (events == 0)
		return kUnhandledInterrupt;
		
	if (events & kPacketReceived)
		fReceiveReady.Release(1, false);

	if (events & (kPacketTransmitted | kTransmitError))
		fTransmitReady.Release(1, false);

	if (events & kTransmitError)
		printf("transmit error\n");

	WriteRegister(events, kInterruptStatusRegister);
	return kReschedule;
}

void Ne2000::Reset()
{
	WriteRegister(ReadRegister(kResetRegister), kResetRegister);
	while (!(ReadRegister(kInterruptStatusRegister) & kResetStatus))
		;

	WriteRegister(0xff, kInterruptStatusRegister);
}

void Ne2000::Initialize()
{
	Reset();

	// Put the card in loopback mode.
	WriteRegister(kCommandStop | kCommandDmaDisable | kCommandPage0, kCommandRegister);
	WriteRegister(0x48, kDataConfigRegister);		// byte wide transfers
	WriteRegister(0, kRemoteCount0Register);
	WriteRegister(0, kRemoteCount1Register);
	WriteRegister(0xff, kInterruptStatusRegister);
	WriteRegister(0, kInterruptMaskRegister);
	WriteRegister(0x20, kReceiveConfigRegister);	// Enter monitor mode
	WriteRegister(2, kTransmitConfigRegister);		// Loopback transmit mode

	// Read data from the prom.
	char prom[32];
	ReadCardMemory(0, prom, 32);

	// This is a goofy card behavior.  See if every byte is doubled.
	// If so, this card accepts 16 bit wide transfers.
	fWordLength = 2;
	for (int i = 0; i < 32; i += 2) {
		if (prom[i] != prom[i + 1]) {
			fWordLength = 1;
			break;
		}
	}

	if (fWordLength == 2) {
		for (int i = 0; i < 32; i += 2)
			prom[i / 2] = prom[i];
		
		fTransmitRingStart = 0x40;
		fReceiveRingStart = 0x4c;
		fReceiveRingEnd = 0x80;
	} else {
		fTransmitRingStart = 0x20;
		fReceiveRingStart = 0x26;	
		fReceiveRingEnd = 0x40;
	}

	if (prom[14] != 0x57 || prom[15] != 0x57) {
		printf("Ne2000: PROM signature does not match Ne2000 0x57.\n");
		bindump(prom, 16);
	}

	memcpy(fMacAddress, prom, 6);

	// Set up card memory
	fNextReceivePacket = fReceiveRingStart;
	WriteRegister(kCommandDmaDisable | kCommandPage0 | kCommandStop, kCommandRegister);
	WriteRegister(fWordLength == 2 ? 0x49 : 0x48, kDataConfigRegister);
	WriteRegister(0, kRemoteCount0Register);
	WriteRegister(0, kRemoteCount1Register);
	WriteRegister(0x20, kReceiveConfigRegister);	// Enter monitor mode
	WriteRegister(2, kTransmitConfigRegister);		// Loopback transmit mode
	WriteRegister(fTransmitRingStart, kTransmitPageRegister);
	WriteRegister(fReceiveRingStart, kPageStartRegister);
	WriteRegister(fNextReceivePacket - 1, kBoundaryRegister);
	WriteRegister(fReceiveRingEnd, kPageStopRegister);
	WriteRegister(0xff, kInterruptStatusRegister);
	WriteRegister(0, kInterruptMaskRegister);

	// Now tell the card what its address is.
	WriteRegister(kCommandDmaDisable | kCommandPage1 | kCommandStop, kCommandRegister);
	for (int i = 0; i < 6; i++)
		WriteRegister(prom[i], static_cast<Ne2kRegister>(kPhysicalAddressRegister + i));

	// Set up multicast filter to accept all packets.
	for (int i=0; i < 8; i++)
		WriteRegister(0xff, static_cast<Ne2kRegister>(kMulticastRegister + i));

	WriteRegister(fNextReceivePacket, kCurrentPageRegister);

	// Start up the card.
	WriteRegister(kCommandDmaDisable | kCommandPage0 | kCommandStop, kCommandRegister);
	WriteRegister(0xff, kInterruptStatusRegister);
	WriteRegister(kPacketReceived | kPacketTransmitted | kTransmitError | kOverwriteWarning,
		kInterruptMaskRegister);
	WriteRegister(kCommandDmaDisable | kCommandStart | kCommandPage0, kCommandRegister);
	WriteRegister(0, kTransmitConfigRegister);
	WriteRegister(4, kReceiveConfigRegister);
}

void Ne2000::ReadCardMemory(int cardAddress, void *data, int count)
{
	if (fWordLength == 2)	
		count = (count + 1) & 0xfffe;

	WriteRegister(kCommandDmaDisable | kCommandStart | kCommandPage0, kCommandRegister);
	WriteRegister(count & 0xff, kRemoteCount0Register);
	WriteRegister(count >> 8, kRemoteCount1Register);
	WriteRegister(cardAddress & 0xff, kRemoteStart0Register);
	WriteRegister(cardAddress >> 8, kRemoteStart1Register);
	WriteRegister(kCommandStart | kCommandRead | kCommandPage0, kCommandRegister);

	if (fWordLength == 2)
		read_io_str_16(fBaseAddress + kDataRegister, (short*) data, count / 2);
	else {
		unsigned char *c = (unsigned char*) data;
		while (count-- > 0)
			*c++ = ReadRegister(kDataRegister);
	}
}

void Ne2000::WriteCardMemory(int cardAddress, const void *data, int count)
{
	// Under certain conditions, the NIC can raise the read or write line before
	// raising the port request line, which can corrupt data in the transfer
	// or lock up the bus.  Issuing a "dummy remote read" first makes sure that
	// the line is high.
	char dummy[2];
	ReadCardMemory(cardAddress, dummy, 2);

	// Now the data can safely be sent to the card.
	if (fWordLength == 2)	
		count = (count + 1) & 0xfffe;

	WriteRegister(kCommandDmaDisable | kCommandStart | kCommandPage0, kCommandRegister);
	WriteRegister(count & 0xff, kRemoteCount0Register);
	WriteRegister(count >> 8, kRemoteCount1Register);
	WriteRegister(cardAddress & 0xff, kRemoteStart0Register);
	WriteRegister(cardAddress >> 8, kRemoteStart1Register);
	WriteRegister(kCommandStart | kCommandWrite, kCommandRegister);

	if (fWordLength == 2)
		write_io_str_16(fBaseAddress + kDataRegister, (short*) data, count / 2);
	else {
		unsigned char *c = (unsigned char*) data;
		while (count-- > 0)
			WriteRegister(*c++, kDataRegister);
	}
}

void Ne2000::HandleRingOverflow()
{
	panic("Ring overflow");
}

Device *Ne2000Instantiate()
{
	return new Ne2000(0x300, 5);
}
