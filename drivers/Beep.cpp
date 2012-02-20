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
