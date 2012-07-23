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

#ifndef _CTYPE_H
#define _CTYPE_H

#define isdigit(c) ((__ctype_table[(int)(c)] & kTypeNum) != 0)
#define ishexdigit(c) ((__ctype_table[(int)(c)] & kTypeHexDigit) != 0)
#define isspace(c) ((__ctype_table[(int)(c)] & kTypeSpace) != 0)
#define isgraphic(c) ((__ctype_table[(int)(c)] & kTypeGraphic) != 0)
#define isalphanum(c) ((__ctype_table[(int)(c)] & (kTypeAlpha | kTypeNum)) != 0)
#define isalpha(c) ((__ctype_table[(int)(c)] & kTypeAlpha) != 0)
#define islower(c) ((__ctype_table[(int)(c)] & kTypeLower)
#define isupper(c) ((__ctype_table[(int)(c)] & kTypeAlpha) != 0 && !islower((int)(c)))

enum {
	kTypeNum = 1,
	kTypeAlpha = 2,
	kTypeHexDigit = 4,
	kTypeSpace = 8,
	kTypeGraphic = 16,
	kTypeLower = 32
};

extern unsigned char __ctype_table[];

#endif

