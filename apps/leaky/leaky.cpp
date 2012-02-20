#include <types.h>
#include <syscall.h>
#include <string.h>
#include <stdio.h>

class Global {
public:
	Global() {
		printf("Global::Global\n");
	}
	
	~Global() {
		printf("Global::~Global\n");
	}
} glob;

const int kBufSize = 15386;
static char buf[kBufSize];

int main()
{

	// 1. Open some files
	for (int i = 0; i < 256; i++) {
		int ifd = open("/boot/datafile.txt", 0);
		if (ifd < 0)
			_serial_print("Error opening data file\n");

		ssize_t sizeRead = read(ifd, buf, kBufSize);
		if (sizeRead <= 0)
			break;
	}

	// 2. Create some semaphores
	for (int i = 0; i < 1024; i++)
		create_sem("leak sem", 0);

	// 3. Create some areas and clone them
	for (int i = 0; i < 5; i++) {
		uint *addr;
		int area = create_area("original area", (void**) &addr, 0, 0x2000,
			AREA_NOT_WIRED, USER_READ | USER_WRITE);
		if (area < 0) {
			_serial_print("error creating original area\n");
			return 0;
		}
		
		for (int j = 0; j < 20; j++) {
			uint *clone_addr;
			int clone = clone_area("clone area", (void**) &clone_addr, 0,
				USER_READ | USER_WRITE, area);
			if (clone < 0) {
				_serial_print("error creating clone area\n");
				return 0;
			}
		}
	}

	printf("Leak... ");
	return 0;
}
