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
