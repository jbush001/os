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

#include "CircularBuffer.h"
#include "cpu_asm.h"
#include "KernelDebug.h"
#include "Device.h"
#include "InterruptHandler.h"
#include "Semaphore.h"
#include "stdio.h"

enum Locks {
	kScrollLock = 1,
	kNumLock = 2,
	kCapsLock = 4,
};

enum Scancodes {
	kScanEsc = 0x37,
	kScanCapsLock = 0x3a,
	kScanLeftShift = 0x42,
	kScanRightShift = 0x54
};

class Keyboard : public Device, public InterruptHandler {
public:
	Keyboard();
	virtual ~Keyboard();
	virtual int Read(off_t, void*, size_t);
	void SetLeds();

private:
	virtual InterruptStatus HandleInterrupt();
	static void Reboot(int argc, const char *argv[]);
	static void WaitOutput();

	Semaphore fKeyEventsQueued;	
	CircularBuffer<char, 1024> fBuffer;
	bool fShift;
	unsigned fLocks;
	static const char kUnshiftedKeymap[];
	static const char kShiftedKeymap[];
};

const char Keyboard::kUnshiftedKeymap[] = {
	 0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 8, 8,
	 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0, 'a', 's',
	 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', 0, 0, 0, 'z', 'x', 'c', 'v',
	 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0, '\t' /*hack*/, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

const char Keyboard::kShiftedKeymap[] = {
	 0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 8, 8,
	 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0, 'A', 'S',
	 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', 0, 0, 0, 'Z', 'X', 'C', 'V',
	 'B', 'N', 'M', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static class Keyboard *gKeyboard = 0;

Keyboard::Keyboard()
	:	fKeyEventsQueued("keyboard:wait_keypress"),
		fShift(false),
		fLocks(kNumLock)
{
	ObserveInterrupt(1);
	SetLeds();
	AddDebugCommand("reboot", "Reboot the machine", Reboot);
}

Keyboard::~Keyboard()
{
	IgnoreInterrupts();
}

InterruptStatus Keyboard::HandleInterrupt()
{
	InterruptStatus result = kHandledInterrupt;
	int scancode = read_io_8(0x60);
	if (scancode & 128) {
		// Break
		if (scancode == (kScanLeftShift | 128) || scancode == (kScanRightShift | 128))
			fShift = false;	// Released shift
	} else {
		// Make
		if (scancode == kScanLeftShift || scancode == kScanRightShift) 
			fShift = true;
		else if (scancode == kScanCapsLock)	{ // caps lock
			if (fLocks & kCapsLock)
				fLocks &= ~kCapsLock;
			else
				fLocks |= kCapsLock;

			SetLeds();	
		} else {
			char ascii;
			if (fShift || (fLocks & kCapsLock))
				ascii = kShiftedKeymap[scancode];
			else 
				ascii = kUnshiftedKeymap[scancode];

			if (scancode == kScanEsc) {
				Debugger();
				return result;
			}

			if (ascii != 0) {
				// Wake reader if this is first char
				if (fBuffer.IsEmpty()) {
					fKeyEventsQueued.Release(1, false);
					result = kReschedule;
				}
				
				if (!fBuffer.IsFull())
					fBuffer.Insert(ascii);
			}
		}
	}
	
	return result;
}

void Keyboard::SetLeds()
{
	WaitOutput();
	write_io_8(0xed, 0x60);
	WaitOutput();
	write_io_8(fLocks, 0x60);
}

int Keyboard::Read(off_t, void *buffer, size_t size)
{
	cpu_flags fl = DisableInterrupts();
	while (fBuffer.IsEmpty()) {
		if (fKeyEventsQueued.Wait() == E_INTERRUPTED) {
			RestoreInterrupts(fl);
			return E_INTERRUPTED;
		}
	}

	int sizeRead = fBuffer.Remove(reinterpret_cast<char*>(buffer), size);
	RestoreInterrupts(fl);
	
	return sizeRead;
}

void Keyboard::Reboot(int, const char*[])
{
	WaitOutput();
	write_io_8(0xd1, 0x64);
	WaitOutput();
	write_io_8(0x82, 0x60);
}

void Keyboard::WaitOutput()
{
	while (read_io_8(0x64) & 2)	
		;
}

Device* KeyboardInstantiate()
{
	if (!gKeyboard) {
		gKeyboard = new Keyboard;
		gKeyboard->AcquireRef();
	}
	
	return gKeyboard;
}
