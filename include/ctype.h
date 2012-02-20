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

