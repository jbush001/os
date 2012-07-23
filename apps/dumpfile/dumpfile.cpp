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
