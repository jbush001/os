/// @file BootParams.h
#ifndef _BOOT_PARAMS_H
#define _BOOT_PARAMS_H

const int kMaxRanges = 64;

/// This structure is passed from the second stage loader to the
/// kernel to let it know the state of the system.  Primarily, it tells
/// the operating system which memory has been allocated.
struct BootParams {
	inline BootParams();
	inline void SetAllocated(unsigned begin, unsigned end);

	unsigned memsize;
	int rangeCount;
	struct Range {
		unsigned begin;
		unsigned end;
	} allocatedRange[kMaxRanges];
};

inline BootParams::BootParams()
	:	rangeCount(0)
{
}

inline void BootParams::SetAllocated(unsigned begin, unsigned end)
{
	allocatedRange[rangeCount].begin = begin;
	allocatedRange[rangeCount].end = end;
	rangeCount++;
}

extern BootParams bootParams;

#endif
