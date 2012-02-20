#include "AddressSpace.h"
#include "elf.h"
#include "Image.h"
#include "stdio.h"
#include "string.h"
#include "syscall.h"
#include "SystemCall.h"
#include "Team.h"
#include "Thread.h"

Image::Image()
	:	fFileHandle(-1),
		fBaseAddress(0),
		fPath(0),
		fHeader(0),
		fSectionTable(0),
		fSymbolTable(0),
		fProgramHeaderTable(0),
		fStringTable(0),
		fStringTableSize(0),
		fSectionStringTable(0),
		fSectionStringTableSize(0)
{
}

Image::~Image()
{
	close_handle(fFileHandle);
	delete [] fPath;
	delete fHeader;
	delete [] fSectionTable;
	delete [] fProgramHeaderTable;
	delete [] fSectionStringTable;
}

int Image::Open(const char path[])
{
	status_t error = E_NO_ERROR;
	fFileHandle = open(path, 0);
	if (fFileHandle < 0)
		return fFileHandle;

	fPath = new char[strlen(path) + 1];
	strcpy(fPath, path);

	error = ReadHeader();
	if (error < 0)
		return error;

	return 0;
}

int Image::Load(Team &team)
{
	fProgramHeaderTable = new Elf32_Phdr[fHeader->e_phnum];
	if (fProgramHeaderTable == 0)
		return E_NO_MEMORY;
		
	if (read_pos(fFileHandle, fHeader->e_phoff, fProgramHeaderTable, fHeader->e_phnum *
		fHeader->e_phentsize) != fHeader->e_phnum * fHeader->e_phentsize) {
		printf("Error reading program header\n");
		return E_NOT_IMAGE;
	}

	const char *filename = fPath;
	for (const char *c = fPath; *c; c++)
		if (*c == '/')
			filename = c + 1;

	for (int segmentIndex = 0; segmentIndex < fHeader->e_phnum; segmentIndex++) {
		char areaName[OS_NAME_LENGTH];
		snprintf(areaName, OS_NAME_LENGTH, "%.12s seg_%d", filename, segmentIndex);
		if (fProgramHeaderTable[segmentIndex].p_vaddr % PAGE_SIZE != 
			fProgramHeaderTable[segmentIndex].p_offset % PAGE_SIZE) {
			printf("Unaligned Segment\n");
			return E_NOT_IMAGE;
		}
		
#if DISABLE_DEMAND_PAGE
		Area *seg = space->CreateArea(areaName, ((fProgramHeaderTable[segmentIndex].p_memsz
			+ PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE, AREA_NOT_LOCKED,
			USER_READ | SYSTEM_READ | SYSTEM_WRITE, WRITABLE_PAGE,
			static_cast<void*>(fProgramHeaderTable[segmentIndex].p_vaddr), EXACT_ADDRESS);
		if (seg == 0)
			return E_NOT_IMAGE;

		lseek(fFileHandle, fProgramHeaderTable[segmentIndex].p_offset, SEEK_SET);
		if (fProgramHeaderTable[segmentIndex].p_type == PT_LOAD
			&& fProgramHeaderTable[segmentIndex].p_filesz > 0)
			read(fFileHandle, static_cast<void*>(fProgramHeaderTable[segmentIndex].p_vaddr),
				fProgramHeaderTable[segmentIndex].p_filesz);
#else
		status_t error = CreateFileArea(areaName, fPath, fProgramHeaderTable[segmentIndex].p_vaddr
			& ~(PAGE_SIZE - 1), fProgramHeaderTable[segmentIndex].p_offset & ~(PAGE_SIZE - 1),
			((fProgramHeaderTable[segmentIndex].p_memsz + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE,
			EXACT_ADDRESS | MAP_PRIVATE, ((fProgramHeaderTable[segmentIndex].p_flags
				& PF_W) ? (USER_WRITE | SYSTEM_WRITE) : 0) | USER_READ | USER_EXEC | SYSTEM_READ,
				team);
		if (error < 0) {
			printf("Failed to map image file\n");
			return error;
		}
#endif

		if (segmentIndex == 0)
			fBaseAddress = reinterpret_cast<void*>(fProgramHeaderTable[segmentIndex].p_vaddr);
	}

	return 0;
}

unsigned int Image::GetEntryAddress() const
{
	return fHeader->e_entry;
}

const char* Image::GetPath() const
{
	return fPath;
}

Elf32_Shdr* Image::FindSection(const char name[]) const
{
	for (int sectionIndex = 0; sectionIndex < fHeader->e_shnum; sectionIndex++)
		if (strcmp(&fSectionStringTable[fSectionTable[sectionIndex].sh_name], name) == 0)
			return &fSectionTable[sectionIndex];
	
	return 0;
}

int Image::ReadHeader()
{
	fHeader = new Elf32_Ehdr;
	if (fHeader == 0)
		return E_NO_MEMORY;
		
	if (read_pos(fFileHandle, 0, fHeader, sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr)) {
		printf("incomplete header\n");
		return E_NOT_IMAGE;
	}

	if (memcmp(fHeader->e_ident, ELF_MAGIC, 4) != 0) {
		printf("not an elf file\n");
		return E_NOT_IMAGE;
	}
			
	if (fHeader->e_ident[4] != ELFCLASS32 || fHeader->e_ident[5] != ELFDATA2LSB) {
		printf("invalid object type\n");
		return E_NOT_IMAGE;
	}
	
	if (fHeader->e_phoff == 0) {
		printf("file has no program header\n");
		return E_NOT_IMAGE;
	}

	if (fHeader->e_phentsize < sizeof(Elf32_Phdr)) {
		printf("File segment info struct is too small!!!\n");
		return E_NOT_IMAGE;
	}

	return 0;
}

void Image::PrintSections() const
{
	printf("sections\n");	
	printf("name             address  offset   size\n");
	for (int sectionIndex = 0; sectionIndex < fHeader->e_shnum; sectionIndex++) {
		if (fSectionTable[sectionIndex].sh_type != SHT_NULL)
			printf("%16s %08x %08x %08x\n",
				&fSectionStringTable[fSectionTable[sectionIndex].sh_name], fSectionTable[sectionIndex].sh_addr,
				static_cast<unsigned int>(fSectionTable[sectionIndex].sh_offset),
				static_cast<unsigned int>(fSectionTable[sectionIndex].sh_size));
	}
}

void Image::PrintSymbols() const
{
	Elf32_Shdr *symbolTable = FindSection(".symtab");
	if (symbolTable == 0) {
		printf("no symbol table!\n");
		return;
	}

	int symbolCount = symbolTable->sh_size / symbolTable->sh_entsize;
	printf("%d symbols\n", symbolCount);
	printf("Type Name                                               value    size\n");
	for (int symbolIndex = 0; symbolIndex < symbolCount; symbolIndex++) {
		switch (ELF32_ST_TYPE(fSymbolTable[symbolIndex].st_info)) {
			case STT_NOTYPE:
				continue;
				
			case STT_OBJECT:
				printf("Obj  ");
				break;
				
			case STT_FUNC:
				printf("Func ");
				break;
				
			case STT_SECTION:
				continue;
				
			case STT_FILE:
				printf("File ");
				break;
				
			default:
				printf("     ");
				break;
		}

		char *name;
		if ((int) fSymbolTable[symbolIndex].st_name > fStringTableSize)
			name = "<huh?>";
		else
			name = &fStringTable[fSymbolTable[symbolIndex].st_name];
		
		printf("%50s %08x %08x\n", name, fSymbolTable[symbolIndex].st_value,
			fSymbolTable[symbolIndex].st_size);
	}
}
