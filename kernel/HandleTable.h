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

/// @file HandleTable.h
#ifndef _HANDLE_TABLE_H
#define _HANDLE_TABLE_H

#include "Lock.h"
#include "types.h"

const int kLevel1Size = 1024;
const int kLevel2Size = 1024;
const int kSerialNumberCount = 2048;

class Resource;

/// HandleTable is an identifier to Resource pointer mapping object.  It is implemented
/// as a two level table, allowing it to grow arbitrarily.
class HandleTable {
public:
	HandleTable();
	~HandleTable();

	/// Stick a resource into the table and return an identifier for it.
	int Open(Resource*);

	/// Remove the given resource from the table
	/// @param id Identifier for this resource, as returned by Open
	void Close(int id);

	/// Find a resource int the table given an id and optional type
	Resource* GetResource(int id, int matchType = OBJ_ANY) const;

	/// Print all handles in this table to the debug log.
	void Print() const;

private:
	struct SubTable *fMainTable[kLevel1Size];
	mutable RWLock fLock;
	int fFreeHint;
};

#endif
