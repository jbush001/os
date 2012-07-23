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
