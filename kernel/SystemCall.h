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

#ifndef _SYSTEM_CALL_H
#define _SYSTEM_CALL_H

#include "cpu_asm.h"
#include "Team.h"
#include "Thread.h"
#include "types.h"

int CreateFileArea(const char name[], const char path[], unsigned base, off_t fileOffset,
	size_t, int flags, PageProtection, Team&);

inline bool CopyUser(void *dest, const void *src, int size)
{
	return Thread::GetRunningThread()->CopyUser(dest, src, size);
}

extern struct SystemCallInfo {
	CallHook hook;
	int parameterSize;
} systemCallTable[];

#endif
