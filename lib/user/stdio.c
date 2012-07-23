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
#include <stdio.h>
#include <syscall.h>

#define KEYBOARD_BUF_SIZE 1024

int outfd = -1;
int infd = -1;
static char keyboard_buf[KEYBOARD_BUF_SIZE];
static int kb_buf_size = 0;
static int kb_buf_offset = 0;

void __stdio_init()
{
	infd = open("/dev/input/keyboard", 0);
	if (infd < 0)
		_serial_print("Error opening keyboard device\n");

	outfd = open("/dev/display/vga_text", 0);
	if (outfd < 0)
		_serial_print("Error opening keyboard device\n");
}

char getc()
{
	if (kb_buf_offset >= kb_buf_size) {
		kb_buf_offset = 0;
		kb_buf_size = read(infd, keyboard_buf, KEYBOARD_BUF_SIZE);
		if (kb_buf_size <= 0)
			return -1;
	}

	return keyboard_buf[kb_buf_offset++];
}

void printf(const char *fmt, ...)
{
	char buf[1024];

	va_list arglist;
	VA_START(arglist, fmt);
	vsnprintf(buf, 1024, fmt, arglist);

	write(outfd, buf, strlen(buf));
}
