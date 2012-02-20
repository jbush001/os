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
