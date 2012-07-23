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

#include <ctype.h>
#include <types.h>
#include <syscall.h>
#include <string.h>
#include <stdio.h>

#define MAX_ARGS 15
#define MAX_COMMAND_LEN 128

static status_t ls(const char *path);
static int parse_args(char *out_buf, const char *in_buf, const char **argptr, int max_args);
static status_t dump(const char *path);
static void readline(char *buf, int len);
static void beep();
static status_t auto_complete(const char *buffer, int end, char *suffix);

static int beepfd = -1;

int main()
{
	char arg_buffer[255 + MAX_ARGS];
	int argc;
	const char *argv[MAX_ARGS];
	status_t err;
	char command[MAX_COMMAND_LEN];
	
	printf("Shell\n");
	for (;;) {
		printf("\n> ");
		readline(command, MAX_COMMAND_LEN);		
		argc = parse_args(arg_buffer, command, argv, MAX_ARGS);
		if (argc == 0)
			continue;

		// Check for builtin commands
		if (strcmp(argv[0], "ls") == 0) {
			if (argc == 1)
				err = ls(".");
			else
				err = ls(argv[1]);
		} else if (argc == 2 && strcmp(argv[0], "cd") == 0)
			err = chdir(argv[1]);
		else if (argc == 2 && strcmp(argv[0], "mkdir") == 0)
			err = mkdir(argv[1], 0);
		else if (argc == 2 && strcmp(argv[0], "rmdir") == 0)
			err = rmdir(argv[1]);
		else if (argc == 4 && strcmp(argv[0], "mount") == 0)
			err = mount(argv[1], argv[2], argv[3], 0, 0);
		else if (argc == 2 && strcmp(argv[0], "dump") == 0)
			err = dump(argv[1]);
		else
			err = exec(argv[0]);

		if (err < 0)
			printf("\n%s\n", strerror(err));
	}
}

void readline(char *buf, int len)
{
	char c;
	int pos = 0;
	
	for (;;) {
		c = getc();
		if (c == 8) {
			/* backspace */
			if (pos > 0) {
				pos--;
				printf("\010");
			} else
				beep();
		} else if (c == '\n') {
			buf[pos] = 0;
			break;
		} else if (c == '\t') {
			char suffix[256];
			buf[pos] = '\0';
			if (auto_complete(buf, pos, suffix) == E_NO_ERROR) {
				printf("%s", suffix);
				memcpy(&buf[pos], suffix, strlen(suffix));
				pos += strlen(suffix);
			} else
				beep();
		} else {
			buf[pos] = c;
			if (pos < len - 1)
				pos++;
			else
				printf("\010");

			printf("%c", c);
		}
	}
}

status_t ls(const char *path)
{
	int fd;
	char name[256];
	
	printf("\n");
	fd = open(path, 0);		
	if (fd < 0)
		return fd;
		
	while (readdir(fd, name, 256) == E_NO_ERROR)
		printf("%s\n", name);
		
	close_handle(fd);
	return E_NO_ERROR;
}

status_t dump(const char *path)
{
	int fd = open(path, 0);
	char buf[1024];
	for (;;) {
		ssize_t sizeRead = read(fd, buf, 1023);
		if (sizeRead <= 0)
			break;

		buf[sizeRead] = 0;
		printf("%s", buf);
	}

	close_handle(fd);
	return 0;
}

int parse_args(char *out_buf, const char *in_buf, const char **argptr, int max_args)
{
	const char *in = in_buf;
	int num_args = 0;
	enum {
		kScanArgument,
		kScanSpace
	} state = kScanSpace;
	
	while (*in != 0 && num_args < max_args) {
		switch (state) {
			case kScanArgument:
				if (isspace(*in)) {
					*out_buf++ = '\0';
					state = kScanSpace;
				} else
					*out_buf++ = *in++;
					
				break;
				
			case kScanSpace:
				if (!isspace(*in)) {
					state = kScanArgument;
					if (num_args >= max_args)
						break;
						
					argptr[num_args++] = out_buf;
				} else
					in++;
				
				break;
		}
	}

	*out_buf = '\0';
	return num_args;
}

status_t auto_complete(const char *buffer, int end, char *suffix)
{
	enum State {
		kScanFile,
		kScanBasePath,
		kScanDone
	} state = kScanFile;
	int baseEnd = end;
	int baseStart = 0;
	int offset;
	char basePath[255];
	char leafName[255];
	char catPath[255];
	int dirfd;
	int foundMatch;

	/* Split the buffer into a base and part of a filename */
	for (offset = end; offset >= 0; offset--) {
		if (state == kScanDone)
			break;
			
		switch (state) {
			case kScanFile:
				if (buffer[offset] == ' ') {
					baseEnd = offset;
					baseStart = offset;
					state = kScanDone;
				} else if (buffer[offset] == '/') {
					baseEnd = offset;
					state = kScanBasePath;
				}
				
				break;
			
			case kScanBasePath:
				if (buffer[offset] == ' ') {
					baseStart = offset + 1;
					state = kScanDone;
				}
				
				break;

			default:
				;
		}
	}

	memcpy(basePath, &buffer[baseStart], baseEnd - baseStart + 1);
	basePath[baseEnd - baseStart + 1] = '\0';

	/* Open the directory */
	if (strlen(basePath) == 0)
		strcpy(basePath, ".");
		
	dirfd = open(basePath, O_RDONLY);
	if (dirfd < 0)
		return E_NO_SUCH_FILE;

	/* Walk the directory */
	strcpy(suffix, "");
	foundMatch = 0;
	while (readdir(dirfd, leafName, 256) == E_NO_ERROR) {
		if (strcmp(leafName, ".") == 0 || strcmp(leafName, "..") == 0)
			continue;

		if (strlen(leafName) + 1 >= (unsigned)(end - baseEnd)
			&& memcmp(leafName, &buffer[baseEnd + 1], end - baseEnd - 1) == 0) {
			if (foundMatch) {
				close_handle(dirfd);
				return E_NO_SUCH_FILE;
			}

			foundMatch = 1;
			strcpy(suffix, leafName + (end - baseEnd - 1));
		}
	}

	close_handle(dirfd);

	/* determine if the leaf is a directory and add a slash if so */
	strcpy(catPath, buffer + baseStart);
	strcat(catPath, suffix);
	strcat(catPath, "/.");
	dirfd = open(catPath, O_RDONLY);
	if (dirfd > 0) {
		strcat(suffix, "/");
		close_handle(dirfd);
	}
	
	if (!foundMatch)
		return E_NO_SUCH_FILE;
	
	return E_NO_ERROR;
}

void beep()
{
	char c;
	if (beepfd < 0)
		beepfd = open("/dev/sound/beep", O_RDONLY);
	
	read(beepfd, &c, 1);
}
