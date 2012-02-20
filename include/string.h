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
