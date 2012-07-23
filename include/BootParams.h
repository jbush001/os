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
