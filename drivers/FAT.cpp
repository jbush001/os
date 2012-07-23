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

#include "ctype.h"
#include "KernelDebug.h"
#include "FileDescriptor.h"
#include "FileSystem.h"
#include "stdio.h"
#include "string.h"
#include "syscall.h"
#include "VNode.h"

const int kBlockSize = 512;
const int kDirEntriesPerBlock = kBlockSize / 32;
const unsigned kEndOfFile = 0xffffffff;
const unsigned kFreeCluster = 0;
const unsigned short kClusterChainDelimiter = 0xfff8;

struct FatSuperBlock {
	uchar jump[3];
	char oemName[8];
	ushort bytesPerSector;
	uchar sectorsPerCluster;
	ushort reservedSectors;
	uchar numFats;
	ushort rootDirEntries;
	ushort sectorCount;
	uchar mediaDescriptor;
	ushort sectorsPerFat;
	ushort sectorsPerTrack;
	ushort headCount;
	ushort hiddenSectorCount;
} PACKED;

struct PartitionBootBlock {
	FatSuperBlock super_block;
	uchar bootProgram[kBlockSize - sizeof(FatSuperBlock)];
} PACKED;

struct PartitionEntry {
	uchar bootFlag;
	uchar partitionStart[3];
	uchar type;
	uchar partitionEnd[3];
	unsigned startSector;
	unsigned sectorCount;
} PACKED;

struct PartitionSector {
	FatSuperBlock bootBlock;
	uchar bootProgram[446 - sizeof(FatSuperBlock)];
	PartitionEntry partitionTable[4];
	uchar signature[2];
} PACKED;

enum DirectoryFlags {
	kAttrReadOnly = 1,
	kAttrHidden = 2,
	kAttrSystem = 4,
	kAttrVolumeLabel = 8,
	kAttrDirectory = 16,
	kAttrArchive = 32	
};

struct FatDirEntry {
	char filename[8];
	char extension[3];
	uchar flags;
	uchar _reserved[10];
	ushort modificationTime;
	ushort modificationDate;
	ushort startCluster;
	unsigned fileLength;

	void GetFilename(char*);
} PACKED;

class FatNode : public VNode {
public:
	FatNode(FileSystem*, int devfd, int startLba, int size, unsigned flags);
	virtual int Lookup(const char name[], size_t nameLen, VNode **outNode);
	virtual int Open(FileDescriptor **outFile);
	virtual int MakeDir(const char*, size_t);
	virtual int RemoveDir(const char*, size_t);
	int ReadBlock(unsigned offset, void *data);
	virtual off_t GetLength();
	virtual void Inactive();
	
private:
	virtual status_t Read(off_t offset, void *va);
	virtual bool HasPage(off_t offset);
	status_t LookupBlock(off_t offset, unsigned *outLba, bool extend = false);

	int fDeviceFd;
	unsigned fStartLba;
	unsigned fLength;
	unsigned fFlags;
	unsigned fHintOffset;
	unsigned fHintLba;
	friend class FatFileSystem;
};

class FatFd : public FileDescriptor {
public:
	FatFd(VNode *node, int devfd, unsigned size);
	virtual int ReadDir(char outName[], size_t size);
private:
	int fCurrentEntry;
	char *fCurrentBlock;
	int fDeviceFd;
	unsigned fLength;
};

class FatFileSystem : public FileSystem {
public:
	FatFileSystem(int fd);
	virtual ~FatFileSystem();
	virtual VNode* GetRootNode();

	unsigned ClusterToLba(unsigned cluster);
	unsigned LbaToCluster(unsigned lba);
	unsigned GetNextBlock(unsigned lba, bool extend = false);
	FatNode *GetNode(unsigned id);
	unsigned AllocateCluster();
	void FreeCluster(unsigned);
	unsigned GetClusterSize() const;

private:
	void FlushFat(unsigned cluster);
	
	int fDeviceFd;
	FatSuperBlock *fSuperBlock;
	unsigned fPartStartSector;
	unsigned fDataStart;
	FatDirEntry	*fEntryBuf;
	unsigned short *fFat;
	int fFatSize;
	FatNode *fRootNode;
	unsigned fRootDirStart;
};

FatFileSystem::FatFileSystem(int devfd)
	:	fDeviceFd(devfd),
		fSuperBlock(0)
{
	// Read Partition table
	PartitionSector sectorData;
	int status = read_pos(fDeviceFd, 0, &sectorData, kBlockSize);
	if (status < 0) {
		printf("device error\n");
		return;
	}
	
	if (sectorData.signature[0] != 0x55 || sectorData.signature[1] != 0xaa) {
		printf("Bad signature\n");
		return;
	}
		
	for (int i = 0; i < 4; i++) {
		printf("partition %d:  start = %08x size = %08x type %02x bootable = %s\n",
			i, sectorData.partitionTable[i].startSector,
			sectorData.partitionTable[i].sectorCount,
			sectorData.partitionTable[i].type,
			sectorData.partitionTable[i].bootFlag == 0x80 ? "y" : "n");

		if (sectorData.partitionTable[i].type == 6) {
			PartitionBootBlock bootBlock;
			int status = read_pos(fDeviceFd, sectorData.partitionTable[i]
				.startSector * kBlockSize, &bootBlock, kBlockSize);
			if (status < 0) {
				printf("drive error\n");
				return;
			}
			
			fSuperBlock = new FatSuperBlock;
			memcpy(fSuperBlock, &bootBlock.super_block, sizeof(FatSuperBlock));
			fPartStartSector =  sectorData.partitionTable[i].startSector;

			// Read the entire FAT into memory (this should be memory mapped).
			fFatSize = fSuperBlock->sectorsPerFat * kBlockSize;
			fFat = new unsigned short[fFatSize / 2];
			for (int i = 0; i < fSuperBlock->sectorsPerFat; i++) {
				int status = read_pos(fDeviceFd, (fPartStartSector +
					fSuperBlock->reservedSectors + i) * kBlockSize,
					reinterpret_cast<char*>(reinterpret_cast<unsigned>(fFat) + i * kBlockSize), kBlockSize);
				if (status < 0) {
					printf("drive error\n");
					return;
				}
			}
				
			break;
		}
	}
	
	fEntryBuf = new FatDirEntry[kDirEntriesPerBlock];
	fRootDirStart = fPartStartSector + fSuperBlock->reservedSectors
		+ fSuperBlock->numFats * fSuperBlock->sectorsPerFat;
	fDataStart = fRootDirStart + fSuperBlock->rootDirEntries / kDirEntriesPerBlock;

#if CHATTY
	printf("Media descriptor = %d\n", fSuperBlock->mediaDescriptor);
	printf("Fat is %d sectors\n", fSuperBlock->sectorsPerFat);
	printf("start of data is sector %d\n", fDataStart);
	printf("%d Sectors per cluster\n", fSuperBlock->sectorsPerCluster);
	printf("%d reserved sectors\n", fSuperBlock->reservedSectors);
	printf("Start of root dir is %d\n", fRootDirStart);
	printf("%d bytes per sector\n", fSuperBlock->bytesPerSector);
#endif

	fRootNode = new FatNode(this, fDeviceFd, fRootDirStart,
		fSuperBlock->rootDirEntries * sizeof(FatDirEntry), kAttrDirectory);
}

FatFileSystem::~FatFileSystem()
{
	delete [] fEntryBuf;
	delete [] fFat;
	delete fSuperBlock;
}

unsigned FatFileSystem::ClusterToLba(unsigned cluster)
{
	ASSERT(cluster >= 2);
	unsigned lba = (cluster - 2) * fSuperBlock->sectorsPerCluster + fDataStart;
	return lba;
}

unsigned FatFileSystem::LbaToCluster(unsigned lba)
{
	return (lba - fDataStart) / fSuperBlock->sectorsPerCluster + 2;
}

unsigned FatFileSystem::GetNextBlock(unsigned lba, bool extend)
{
	// Check to see if this is the end of the root directory.
	if (lba == fDataStart - 1)
		return kEndOfFile;
		
	// If this is a block in the root directory, return the next sequential
	// block.
	if (lba < fDataStart)
		return lba + 1;

	// If this block is in the middle of a cluster, return the
	// next sequential block
	if ((lba % fSuperBlock->sectorsPerCluster) != 0)
		return lba + 1;

	// This block is in the data area, and is in the beginning of a new
	// cluster, so do a Fat lookup to resolve next block.
	unsigned cluster = LbaToCluster(lba);
	unsigned nextCluster = fFat[cluster];
	if (nextCluster >= kClusterChainDelimiter) {
		if (extend) {
			printf("Extending Fat chain\n");
			nextCluster = AllocateCluster();
			fFat[cluster] = nextCluster;
			FlushFat(cluster);
		} else
			return kEndOfFile;
	}
		
	return ClusterToLba(nextCluster);
}

unsigned FatFileSystem::AllocateCluster()
{
	for (int cluster = 0; cluster < fFatSize / 2; cluster++) {
		if (fFat[cluster] == kFreeCluster) {
			fFat[cluster] = kClusterChainDelimiter;
			FlushFat(cluster);
			return cluster;
		}
	}

	return kEndOfFile;
}

void FatFileSystem::FreeCluster(unsigned cluster)
{
	fFat[cluster] = kFreeCluster;
	FlushFat(cluster);
}

unsigned FatFileSystem::GetClusterSize() const
{
	return fSuperBlock->sectorsPerCluster * kBlockSize;
}

void FatFileSystem::FlushFat(unsigned cluster)
{
	if (write_pos(fDeviceFd, (fPartStartSector + fSuperBlock->reservedSectors) * kBlockSize
		+ cluster * sizeof(short), fFat + cluster, kBlockSize) < 0)
		panic("Error writing fat");	
}

VNode* FatFileSystem::GetRootNode()
{
	return fRootNode;
}

FatNode::FatNode(FileSystem *fileSystem, int deviceFd, int startLba, int size, unsigned flags)
	:	VNode(fileSystem),
		fDeviceFd(deviceFd),
		fStartLba(startLba),
		fLength(size),
		fFlags(flags),
		fHintOffset(0),
		fHintLba(startLba)
{
}

int FatNode::Lookup(const char name[], size_t nameLen, VNode **outNode)
{
	if ((fFlags & kAttrDirectory) == 0) {
		printf("Lookup on a regular file, returning error\n");
		return E_INVALID_OPERATION;
	}

	if (nameLen == 1 && name[0] == '.') {
		AcquireRef();
		*outNode = this;
		return E_NO_ERROR;
	}

	FatDirEntry entries[kDirEntriesPerBlock];
	for (int entryNum = 0; ; entryNum++) {
		if (entryNum % kDirEntriesPerBlock == 0)
			if (ReadBlock((entryNum / kDirEntriesPerBlock) * kBlockSize, entries) < 0)
				break;
	
		FatDirEntry *entry = entries + (entryNum % kDirEntriesPerBlock);
		if (entry->filename[0] == 0 || static_cast<uchar>(entry->filename[0]) == 0xe5
			|| (entry->flags & kAttrVolumeLabel))
			continue; // Skip unused entries

		char fileName[13];
		entry->GetFilename(fileName);
		if (strlen(fileName) == nameLen && memcmp(name, fileName, nameLen) == 0) {
			*outNode = new FatNode(GetFileSystem(), fDeviceFd, static_cast<FatFileSystem*>(
				GetFileSystem())->ClusterToLba(entry->startCluster), entry->fileLength,
				entry->flags);
			(*outNode)->AcquireRef();
			return E_NO_ERROR;
		}
	}

	printf("Lookup failed\n");	
	return E_NO_SUCH_FILE;
}

int FatNode::Open(FileDescriptor **outFile)
{
	*outFile = new FatFd(this, fDeviceFd, fLength);
	return 0;
}

int FatNode::MakeDir(const char name[], size_t nameLength)
{
	printf("FatNode::MakeDir\n");
	if (!(fFlags & kAttrDirectory)) {
		printf("This isn't a directory\n");
		return E_INVALID_OPERATION;
	}

	FatFileSystem *fs = static_cast<FatFileSystem*>(GetFileSystem());
	char sanitizedName[11];
	const char *c = name;
	for (int i = 0; i < 8; i++) {
		if (*c == '.' || *c == '\0')
			sanitizedName[i] = ' ';
		else
			sanitizedName[i] = *c++;
	}
	
	if (*c == '.')
		c++;
		
	for (int i = 8; i < 11; i++) {
		if (*c == '.' || *c == '\0')
			sanitizedName[i] = ' ';
		else
			sanitizedName[i] = *c++;
	}
	

	// Look for a free entry and make sure this isn't a duplicate
	char dirBlock[kBlockSize];
	unsigned lba;
	int error;
	for (int entryNum = 0; ; entryNum++) {
		printf("try to insert entry %d\n", entryNum);
		if (entryNum % kDirEntriesPerBlock == 0) {
			error = LookupBlock((entryNum / kDirEntriesPerBlock) * kBlockSize, &lba, true);
			if (error < 0) {
				printf("MakeDir: LookupBlock returned error\n");
				return error;
			}
		
			// Read the block
			error = read_pos(fDeviceFd, lba * kBlockSize, dirBlock, kBlockSize);
			if (error < 0)
				printf("sys read returned error %d\n", error);
		}
	
		FatDirEntry *entry = reinterpret_cast<FatDirEntry*>(dirBlock) + (entryNum % kDirEntriesPerBlock);
		if (entry->filename[0] == 0 || static_cast<uchar>(entry->filename[0]) == 0xe5) {
			// Use this entry.
			printf("Created new entry\n");
			memcpy(entry->filename, name, MIN(8, nameLength));
			memset(entry->extension, ' ', 3);
			entry->startCluster = fs->AllocateCluster();
			entry->flags = kAttrDirectory;

			// Create information for the new directory.
			char *dirData = new char[fs->GetClusterSize()];
			memset(dirData, 0, fs->GetClusterSize());
			FatDirEntry *newDir = reinterpret_cast<FatDirEntry*>(dirData);
			strcpy(newDir[0].filename, ".");
			newDir[0].flags = kAttrDirectory;
			newDir[0].startCluster = entry->startCluster;
			strcpy(newDir[1].filename, "..");
			newDir[1].flags = kAttrDirectory;
			newDir[1].startCluster = entry->startCluster;	// How should I get this?
			
			error = write_pos(fDeviceFd, entry->startCluster, dirData, fs->GetClusterSize());
			if (error < 0)
				printf("Error writing new directory\n");
			
			error = write_pos(fDeviceFd, lba * kBlockSize, dirBlock, kBlockSize);
			if (error < 0)
				printf("Error writing directory block\n");

			delete [] dirData;
			return error;
		}
	}
	
	return E_NO_ERROR;
}

int FatNode::RemoveDir(const char[], size_t)
{
	return E_INVALID_OPERATION;
}

int FatNode::ReadBlock(unsigned offset, void *data)
{
	ASSERT(offset % kBlockSize == 0);
	unsigned lba;
	int error = LookupBlock(offset, &lba, false);
	if (error < 0)
		return error;

	// Read the block
	error = read_pos(fDeviceFd, lba * kBlockSize, data, kBlockSize);
	if (error < 0)
		printf("sys read returned error %d\n", error);

	return error;
}

off_t FatNode::GetLength()
{
	return fLength;
}

void FatNode::Inactive()
{
	delete this;
}

bool FatNode::HasPage(off_t offset)
{
	return offset < fLength;
}

status_t FatNode::LookupBlock(off_t requestOffset, unsigned *outLba, bool extend)
{
	unsigned lba = kEndOfFile;
	if (requestOffset < fHintOffset) {
		// This offset is past the saved hint block.  Start
		// over from the beginning of the file
		fHintOffset = 0;				
		fHintLba = fStartLba;
	}

	// Walk the Fat chain starting from the last hint
	lba = fHintLba;
	unsigned currentOffset = fHintOffset;
	while (currentOffset < requestOffset) {
		lba = static_cast<FatFileSystem*>(GetFileSystem())->GetNextBlock(lba, extend);
		if (lba == kEndOfFile)
			return E_IO;

		currentOffset += kBlockSize;
	}

	ASSERT(currentOffset == requestOffset);

	// Reset the hint	
	fHintOffset = requestOffset;
	fHintLba = lba;
	*outLba = lba;
	return E_NO_ERROR;
}

status_t FatNode::Read(off_t offset, void *va)
{
	char *outPtr = reinterpret_cast<char*>(va);
	int err = E_NO_ERROR;
	for (int i = 0; i < PAGE_SIZE / kBlockSize; i++) {
		if (offset >= fLength)
			break;

		err = ReadBlock(offset, outPtr);
		if (err < 0)
			break;
		
		offset += kBlockSize;
		outPtr += kBlockSize;
	}

	return outPtr == va ? err : E_NO_ERROR;
}

FatFd::FatFd(VNode *node, int devfd, unsigned size)
	:	FileDescriptor(node),
		fCurrentEntry(0),
		fDeviceFd(devfd),
		fLength(size)
{
	fCurrentBlock = new char[kBlockSize];
	ASSERT(fCurrentBlock != 0);
}

int FatFd::ReadDir(char outName[], size_t)
{
	int error;

	for (;;) {
		int blockNum = fCurrentEntry / kDirEntriesPerBlock;
		if (fCurrentEntry % kDirEntriesPerBlock == 0) {
			error = static_cast<FatNode*>(GetNode())->ReadBlock(blockNum * kBlockSize, fCurrentBlock);
			if (error < 0)
				return error;
		}
	
		FatDirEntry *entry = reinterpret_cast<FatDirEntry*>(fCurrentBlock) + (fCurrentEntry
			% kDirEntriesPerBlock);
		if (entry->filename[0] == 0 || static_cast<uchar>(entry->filename[0]) == 0xe5 ||
			(entry->flags & kAttrVolumeLabel)) {
			// Skip unused entries
			fCurrentEntry++;
			continue;
		}

		entry->GetFilename(outName);
		fCurrentEntry++;
		return E_NO_ERROR;
	}
}

void FatDirEntry::GetFilename(char outName[])
{
	int nameLen = 8;
	while (isspace(filename[nameLen - 1]))
		nameLen--;
		
	for (int i = 0; i < nameLen; i++) {
		char c = filename[i];
		outName[i] = c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c;
	}
	
	for (int i = 0; i < 3; i++) {
		if (extension[i] == ' ')
			break;

		if (i == 0)
			outName[nameLen++] = '.';
			
		char c = extension[i];
		outName[nameLen++] = c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c;
	}
	
	outName[nameLen] = '\0';
}


FileSystem* FatFsInstantiate(int device)
{
	return new FatFileSystem(device);
}
