#include "AddressSpace.h"
#include "Area.h"
#include "cpu_asm.h"
#include "Device.h"
#include "graphics.h"
#include "types.h"

class Area;

const int MiscOutputReg = 0x3c2;
const int DataReg = 0x3c0;
const int AddressReg = 0x3c0;

const uchar mode13[][32] = {
  { 0x03, 0x01, 0x0f, 0x00, 0x0e },            // 0x3c4, index 0-4
  { 0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0xbf, 0x1f,
    0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x9c, 0x0e, 0x8f, 0x28, 0x40, 0x96, 0xb9, 0xa3,
    0xff },                                    // 0x3d4, index 0-0x18
  { 0, 0, 0, 0, 0, 0x40, 0x05, 0x0f, 0xff },   // 0x3ce, index 0-8
  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    0x41, 0, 0x0f, 0, 0 }                      // 0x3c0, index 0-0x14
};

#define PaletteMask              0x3C6  // bit mask register
#define PaletteRegisterRead      0x3C7  // read index
#define PaletteRegisterWrite     0x3C8  // write index
#define PaletteData              0x3C9  // send/receive data here
#define RomCharSegment           0xF000
#define RomCharOffset            0xFA6E
#define CharWidth                8
#define CharHeight               8


class VGAGraphics : public Device {
public:
	VGAGraphics();
	virtual ~VGAGraphics();
	virtual int Control(int command, void *buffer);

private:
	void InitGraphics();
	void SetPalette(int color, int red, int green, int blue);

 	Area *fFrameBufferArea;
	char *fFrameBuffer;
};


VGAGraphics::VGAGraphics()
{
}

VGAGraphics::~VGAGraphics()
{
}

void VGAGraphics::InitGraphics()
{
	//	vga init code by Paul Swanson
	int i;

	write_io_8(0x63, MiscOutputReg);
	write_io_8(0x00, 0x3da);

	for (i=0; i < 5; i++) {
		write_io_8(i, 0x3c4);
		write_io_8(mode13[0][i], 0x3c4 + 1);
	}

	write_io_16(0x0e11, 0x3d4);

	for (i=0; i < 0x19; i++) {
		write_io_8(i, 0x3d4);
		write_io_8(mode13[1][i], 0x3d4 + 1);
	}

	for (i=0; i < 0x9; i++) {
		write_io_8(i, 0x3ce);
		write_io_8(mode13[2][i], 0x3ce + 1);
	}

	read_io_8(0x3da);

	for (i=0; i < 0x15; i++) {
		read_io_16(DataReg);
		write_io_8(i, AddressReg);
		write_io_8(mode13[3][i], DataReg);
	}

	write_io_8(0x20, 0x3c0);

	fFrameBufferArea = AddressSpace::GetKernelAddressSpace()->MapPhysicalMemory("VGA frame buffer", 0xa0000, 0x13000,
		USER_READ | USER_WRITE | SYSTEM_READ | SYSTEM_WRITE);
	if (fFrameBufferArea == 0)
		panic("Couldn't create frame buffer area\n");

	fFrameBuffer = (char*) fFrameBufferArea->GetBaseAddress();
	for (int i = 0; i < 256; i++)
		SetPalette(i, (i & 7) * 32, ((i >> 3) & 7) * 32, ((i >> 6) & 3) * 32);
}

void VGAGraphics::SetPalette(int color, int red, int green, int blue)
{
	write_io_8(0xff, PaletteMask);
	write_io_8(color, PaletteRegisterWrite);
	write_io_8(red, PaletteData);
	write_io_8(green, PaletteData);
	write_io_8(blue, PaletteData);
}

int VGAGraphics::Control(int command, void *buffer)
{
	switch (command) {
		case INIT_GRAPHICS:
			InitGraphics();
			break;

		case GET_FRAME_BUFFER_INFO: {
			frame_buffer_info *info = (frame_buffer_info*) buffer;
			info->width = 320;
			info->height = 200;
			info->bpp = 8;
			info->base_address = (void*) fFrameBuffer;
			info->bytes_per_row = 320;
			break;
		}

		default:
			return -1;
	}

	return E_NO_ERROR;
}

Device* VgaGraphicsInstantiate()
{
	return new VGAGraphics;
}
