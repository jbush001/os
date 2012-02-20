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
