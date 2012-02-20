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
