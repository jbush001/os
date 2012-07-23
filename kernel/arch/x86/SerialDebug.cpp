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

//
//	Hooks for debug console IO over a serial port
//

#include "cpu_asm.h"

const int kBaudRate = 115200;

void DebugConsoleBootstrap()
{
	short divisor = 115200 / kBaudRate;
	write_io_8(0x80, 0x3fb); // Set up to load divisor latch.
	write_io_8(divisor & 0xff, 0x3f8); // Divisor LSB
	write_io_8(divisor >> 8, 0x3f9); // Divisor MSB
	write_io_8(3, 0x3fb); // 8N1
}

char DebugConsoleRead()
{
	while ((read_io_8(0x3fd) & 1) == 0)
		;
		
	return read_io_8(0x3f8);
}

static void SerialOut(const char c)
{
	while ((read_io_8(0x3fd) & 0x64) == 0)
		;
		
	write_io_8(c, 0x3f8);
}

void DebugConsoleWrite(const char c)
{
	if (c == '\n') {
		SerialOut('\r');
		SerialOut('\n');
	} else if (c != '\r')
		SerialOut(c);
}
