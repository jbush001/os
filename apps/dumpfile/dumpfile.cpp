#include <types.h>
#include <syscall.h>
#include <string.h>


const int kBufSize = 237;
static int ofd = -1;
static char buf[kBufSize];

int main()
{
	ofd = open("/dev/display/vga_text", 0);
	if (ofd < 0)
		_serial_print("Error opening display device\n");

	int ifd = open("/boot/datafile.txt", 0);
	if (ifd < 0)
		_serial_print("Error opening data file\n");

	while (true) {
		ssize_t sizeRead = read(ifd, buf, kBufSize);
		if (sizeRead <= 0)
			break;
			
		write(ofd, buf, sizeRead);
	}

	return 0;
}
