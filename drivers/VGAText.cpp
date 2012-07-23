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

#include <string.h>
#include "AddressSpace.h"
#include "Area.h"
#include "cpu_asm.h"
#include "Device.h"
#include "types.h"

class Area;

const int kScreenWidth = 80;
const int kScreenHeight = 25;
const unsigned kTextPhysicalBase = 0xb8000;
static class VgaText *gText = 0;

class VgaText : public Device {
public:
	VgaText();
	virtual int Write(off_t, const void*, size_t);

private:
	void Clear();
	inline void ScrollUp();
	inline void UpdateHardwareCursor();

	unsigned short *fTextBuffer;
	unsigned short fCurrentAttribute;
	unsigned fOffset;
 	Area *fTextBufferArea;
};

VgaText::VgaText()
	:	fCurrentAttribute(0x0f00),
		fOffset(0)
{
	fTextBufferArea = AddressSpace::GetKernelAddressSpace()->MapPhysicalMemory(
		"Vga Text Buffer", kTextPhysicalBase, PAGE_SIZE, SYSTEM_READ | SYSTEM_WRITE);
	if (fTextBufferArea == 0)
		panic("Couldn't create display buffer area\n");

	fTextBuffer = reinterpret_cast<unsigned short*>(fTextBufferArea->GetBaseAddress());
	Clear();
}

inline void VgaText::UpdateHardwareCursor()
{
	write_io_8(14, 0x3d4);
	write_io_8((fOffset >> 8) & 0xff, 0x3d5);
	write_io_8(15, 0x3d4);
	write_io_8(fOffset & 0xff, 0x3d5);
}

inline void VgaText::ScrollUp()
{
	memcpy(fTextBuffer, fTextBuffer + kScreenWidth, kScreenWidth * (kScreenHeight - 1) * 2);
	for (int col = 0; col < kScreenWidth; col++)
		fTextBuffer[kScreenWidth * (kScreenHeight - 1) + col] = fCurrentAttribute | ' ';
}

int VgaText::Write(off_t, const void *buf, size_t size)
{
	cpu_flags fl = DisableInterrupts();
	char *s = (char*) buf;
	while (size-- > 0) {
		char c = *s++;
		switch (c) {
			case '\n':
				fOffset += kScreenWidth - (fOffset % kScreenWidth);
				if (fOffset >= kScreenWidth * kScreenHeight) {
					fOffset -= kScreenWidth;
					ScrollUp();
				}

				break;

			case '\010':	// Backspace
				if (fOffset > 0)
					fTextBuffer[--fOffset] = fCurrentAttribute | ' ';

				break;

			default:
				fTextBuffer[fOffset++] = fCurrentAttribute | c;
				if (fOffset > kScreenWidth * kScreenHeight) {
					fOffset -= kScreenWidth;
					ScrollUp();
				}
		}
	}

	UpdateHardwareCursor();
	RestoreInterrupts(fl);
	return E_NO_ERROR;
}

void VgaText::Clear()
{
	cpu_flags fl = DisableInterrupts();
	for (int i = 0; i < kScreenWidth * kScreenHeight; i++)
		fTextBuffer[i] = fCurrentAttribute | ' ';

	fOffset = 0;
	UpdateHardwareCursor();
	RestoreInterrupts(fl);
}

Device* VgaTextInstantiate()
{
	if (!gText) {
		gText = new VgaText;
		gText->AcquireRef();
	}

	return gText;
}
