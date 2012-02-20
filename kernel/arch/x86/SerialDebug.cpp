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
