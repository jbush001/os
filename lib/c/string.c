#include "string.h"

void* memcpy(void *dest, const void *src, size_t length)
{
	char *d = (char*) dest;
	const char *s = (const char*) src;
	while (length > 4) {
		*(unsigned*) d = *(unsigned*) s;
		d += 4;
		s += 4;
		length -= 4;
	}
	
	while (length > 0) {
		*d++ = *s++;
		length--;
	}

	return dest;
}

void* memset(void *start, int value, size_t size)
{
	char *out = (char*) start;
	const int kLongValue = (value | (value << 8) | (value << 16) | (value << 24));

	while (((unsigned) out & 3) != 0 && size > 0) {
		*out++ = value;
		size--;
	}

	while (size > 4) {
		*(unsigned*) out = kLongValue;
		out += 4;
		size -= 4;
	}
	
	while (size) {
		*out++ = value;
		size--;
	}

	return start;
}

int memcmp(const void *src1, const void *src2, size_t len)
{
	const char *sp1 = (const char*) src1;
	const char *sp2 = (const char*) src2;

	while (len-- > 0) {
		if (*sp1 != *sp2)
			return *sp1 - *sp2;
			
		sp1++;
		sp2++;
	}

	return 0;
}

char* strchr(const char *str, int chr)
{
	while (*str) {
		if (*str == chr)
			return (char*) str;
			
		str++;
	}
	
	return 0;
}

unsigned long strlen(const char *str)
{
	long len = 0;
	while (*str++)
		len++;
		
	return len;
}

int strcmp(const char *str1, const char *str2)
{
	while (*str1) {
		if (*str2 == 0)
			return -1;
			
		if (*str1 != *str2)
			return *str1 - *str2;
			
		str1++;
		str2++;
	}

	if (*str2)
		return 1;

	return 0;
}

int strncmp(const char *str1, const char *str2, int size)
{
	while (*str1) {
		if (*str2 == 0)
			return -1;
			
		if (*str1 != *str2 || --size <= 0)
			return *str1 - *str2;
			
		str1++;
		str2++;
	}

	if (*str2)
		return 1;

	return 0;
}

char* strcpy(char *dest, const char *src)
{
	char *d = dest;
	while (*src)
		*d++ = *src++;

	*d = 0;
	return dest;
}

char* strncpy(char *dest, const char *src, int len)
{
	char *d = dest;
	while (*src && len-- > 0)
		*d++ = *src++;

	*d = 0;
	return dest;
}

char* strcat(char *dest, const char *src)
{
	char *ret = dest;
	while (*dest != 0)
		dest++;

	while (*src != 0)
		*dest++ = *src++;
	
	*dest = 0;
	return ret;
}
