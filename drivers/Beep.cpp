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

#include "cpu_asm.h"
#include "Device.h"
#include "syscall.h"

class Beep : public Device {
public:
	Beep();
	virtual int Write(off_t, const void*, size_t);
	virtual int Read(off_t, void*, size_t);
	void Play();
};

Beep::Beep()
{
	write_io_8(0xb6, 0x43);
	write_io_8(30, 0x42);
	write_io_8(5, 0x42);
}

int Beep::Write(off_t, const void *, size_t)
{
	Play();
	return 0;
}

int Beep::Read(off_t, void *, size_t)
{
	Play();
	return 0;
}

void Beep::Play()
{
	write_io_8(read_io_8(0x61) | 3, 0x61);
	sleep(70000);
	write_io_8(read_io_8(0x61) & ~3, 0x61);
}

Device* BeepInstantiate()
{
	return new Beep;
}
