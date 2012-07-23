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

/// @file Team.h
#ifndef _TEAM_H
#define _TEAM_H

#include "HandleTable.h"
#include "Resource.h"
#include "List.h"

class AddressSpace;
class Thread;

/// A team is a group of threads and an associated address space
class Team : public Resource, public ListNode {
public:
	Team(const char name[]);
	virtual ~Team();
	inline AddressSpace* GetAddressSpace() const;

	/// Called when a thread is created.  Add to the team's list of threads.
	void ThreadCreated(Thread*);

	/// Called when a thread terminates.  Remove from the team's list of threads.
	void ThreadTerminated(Thread*);

	/// Each team has it's own private handle table.
	inline const HandleTable *GetHandleTable() const;
	inline HandleTable* GetHandleTable();

	/// Call a function for every team in the system
	static void DoForEach(void (*EachTeamFunc)(void*, Team*), void*);

	/// Called at boot time to initialize structures
	static void Bootstrap();

private:
	Team(const char name[], AddressSpace *);
	static void PrintThreads(int, const char**);
	static void PrintAreas(int, const char*[]);
	static void PrintHandles(int, const char*[]);

	AddressSpace *fAddressSpace;
	Thread *fThreadList;
	HandleTable fHandleTable;
	static List fTeamList;
};

inline AddressSpace* Team::GetAddressSpace() const
{
	return fAddressSpace;
}

inline const HandleTable* Team::GetHandleTable() const
{
	return &fHandleTable;
}

inline HandleTable* Team::GetHandleTable()
{
	return &fHandleTable;
}

#endif
