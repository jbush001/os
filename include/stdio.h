#ifndef _STDIO_H
#define _STDIO_H

#ifdef __cplusplus
extern "C" {
#endif

#define __VA_ROUNDED_SIZE(TYPE) (((sizeof (TYPE) + sizeof (int) - 1) / sizeof (int)) * sizeof (int))
#define VA_START(AP, LASTARG) (AP = ((char *) &(LASTARG) + __VA_ROUNDED_SIZE (LASTARG)))
#define VA_ARG(AP, TYPE) (AP += __VA_ROUNDED_SIZE(TYPE), *((TYPE *) (AP - __VA_ROUNDED_SIZE (TYPE))))

typedef char *va_list;

void vsnprintf(char *out, int size, const char *fmt, va_list args);

void snprintf(char *out, int size, const char *fmt, ...); 
void printf(const char *fmt, ...);
char getc();

#ifdef __cplusplus
}
#endif

#endif
