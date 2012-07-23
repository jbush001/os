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

#ifndef __STRING_H
#define __STRING_H

#include "types.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))


#ifdef __cplusplus
extern "C" {
#endif

	void* memcpy(void *dest, const void *src, size_t length);
	void* memset(void *start, int value, size_t size);
	int memcmp(const void *src1, const void *src2, size_t len);
	char *strchr(const char *str, int chr);
	unsigned long strlen(const char*);
	int strcmp(const char *str1, const char *str2);
	int strncmp(const char *str1, const char *str2, int size);
	char* strcpy(char *dest, const char *src);
	char* strcat(char *dest, const char *src);
	char* strncpy(char *dest, const char *src, int len);
	int atoi(const char *num);


#ifdef __cplusplus
}
#endif


#endif
