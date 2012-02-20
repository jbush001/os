#ifndef _IMAGE_H
#define _IMAGE_H

#include "types.h"

class Team;

class Image {
public:
	Image();
	virtual ~Image();
	int Open(const char path[]);
	int Load(Team&);
	unsigned int GetEntryAddress() const;
	const char* GetPath() const;

private:
	struct Elf32_Shdr* FindSection(const char name[]) const;
	int ReadHeader();
	void PrintSections() const;
	void PrintSymbols() const;
	
	int fFileHandle;
	void *fBaseAddress;
	char *fPath;
	struct Elf32_Ehdr *fHeader;
	struct Elf32_Shdr *fSectionTable;
	struct Elf32_Sym *fSymbolTable;
	struct Elf32_Phdr *fProgramHeaderTable;
	char *fStringTable;
	int fStringTableSize;
	char *fSectionStringTable;
	int fSectionStringTableSize;	
};

#endif
