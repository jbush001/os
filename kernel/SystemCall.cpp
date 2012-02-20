#include "AddressSpace.h"
#include "APC.h"
#include "Area.h"
#include "cpu_asm.h"
#include "Dispatcher.h"
#include "FileDescriptor.h"
#include "FileSystem.h"
#include "HandleTable.h"
#include "Image.h"
#include "KernelDebug.h"
#include "PageCache.h"
#include "stdio.h"
#include "string.h"
#include "syscall.h"
#include "SystemCall.h"
#include "Team.h"
#include "Thread.h"
#include "types.h"
#include "VNode.h"

int bad_syscall(...);

SystemCallInfo systemCallTable[] = {
	{ (CallHook) sleep, 2},
	{ (CallHook) _serial_print, 1},
	{ (CallHook) spawn_thread, 4},
	{ (CallHook) thread_exit, 0},
	{ (CallHook) create_sem, 2 },
	{ (CallHook) acquire_sem, 3 },
	{ (CallHook) release_sem, 2 },
	{ (CallHook) think, 0 },
	{ (CallHook) open, 3 },
	{ (CallHook) close_handle, 1 },
	{ (CallHook) stat, 2 },
	{ (CallHook) ioctl, 3 },
	{ (CallHook) write, 3 },
	{ (CallHook) read, 3 },
	{ (CallHook) mkdir, 2 },
	{ (CallHook) rmdir, 1 },
	{ (CallHook) rename, 2 },
	{ (CallHook) readdir, 3 },
	{ (CallHook) rewinddir, 1 },
	{ (CallHook) lseek, 3 },
	{ (CallHook) exec, 1 },
	{ (CallHook) create_area, 6 },
	{ (CallHook) clone_area, 5 },
	{ (CallHook) delete_area, 1 },
	{ (CallHook) resize_area, 2 },
	{ (CallHook) wait_for_multiple_objects, 5 },
	{ (CallHook) bad_syscall, 0 },
	{ (CallHook) kill_thread, 1 },
	{ (CallHook) read_pos, 5 },
	{ (CallHook) write_pos, 5 },
	{ (CallHook) chdir, 1 },
	{ (CallHook) getcwd, 2 },
	{ (CallHook) mount, 5 },
	{ (CallHook) map_file, 6 },
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
	{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},{bad_syscall,0},
};

static Semaphore gSleepSemaphore("sleep_sem", 0);

inline Resource *GetResource(int handle, int type)
{
	return Thread::GetRunningThread()->GetTeam()->GetHandleTable()->GetResource(handle, type);
}

inline int OpenHandle(Resource *obj)
{
	return Thread::GetRunningThread()->GetTeam()->GetHandleTable()->Open(obj);
}

int sleep(bigtime_t timeout)
{
	gSleepSemaphore.Wait(timeout);
	return 0;
}

int think()
{
	return 1;
}

int bad_syscall(...)
{
	printf("invalid syscall");
	return E_INVALID_OPERATION;
}

int _serial_print(const char string[])
{
	printf("%s", string);
	return 0;
}

int spawn_thread(thread_start_t entry, const char name[], void *data, int priority)
{
	Thread *thread = new Thread(name, Thread::GetRunningThread()->GetTeam(), entry,
		data, priority);
	if (thread == 0)
		return E_NO_MEMORY;

	return E_NO_ERROR;	// Returning thread would require user to close it.
}

void thread_exit()
{
	Thread::GetRunningThread()->Exit();
}

int create_sem(const char name[], int count)
{
	Semaphore *sem = new Semaphore(name, count);
	if (sem == 0)
		return E_NO_MEMORY;
		
	return OpenHandle(sem);
}

status_t acquire_sem(int handle, bigtime_t timeout)
{
	Semaphore *sem = static_cast<Semaphore*>(GetResource(handle, OBJ_SEMAPHORE));
	if (sem == 0)
		return E_BAD_HANDLE;
		
	status_t ret = sem->Wait(timeout);
	sem->ReleaseRef();
	return ret;
}

status_t release_sem(int handle, int count)
{
	Semaphore *sem = static_cast<Semaphore*>(GetResource(handle, OBJ_SEMAPHORE));
	if (sem == 0)
		return E_BAD_HANDLE;
		
	sem->Release(count);
	sem->ReleaseRef();
	return E_NO_ERROR;
}

int create_area(const char name[], void **requestAddr, int flags, unsigned int size,
	AreaWiring lock, PageProtection protection)
{
	unsigned int va = INVALID_PAGE;
	if (flags & EXACT_ADDRESS)
		va = reinterpret_cast<unsigned int>(*requestAddr);
		
	PageCache *cache = new PageCache;
	if (cache == 0)
		return E_NO_MEMORY;	

	// It is important that CreateArea not incur a fault!		
	char nameCopy[OS_NAME_LENGTH];
	if (!CopyUser(nameCopy, name, OS_NAME_LENGTH))
		return E_BAD_ADDRESS;

	Area *newArea = AddressSpace::GetCurrentAddressSpace()->CreateArea(nameCopy, size, lock,
		protection | USER_READ | SYSTEM_READ | ((protection & USER_WRITE)
		? SYSTEM_WRITE : 0), cache, 0, va, flags);
	if (newArea == 0) {
		delete cache;
		return E_NO_MEMORY;
	}

	*requestAddr = reinterpret_cast<void*>(newArea->GetBaseAddress());		
	return OpenHandle(newArea);
}

int clone_area(const char name[], void **clone_addr, int searchFlags,
	PageProtection protection, int sourceArea)
{
	Area *area = static_cast<Area*>(GetResource(sourceArea, OBJ_AREA));
	if (area == 0) {
		printf("clone_area: source area id %08x is invalid\n", sourceArea);
		return E_BAD_HANDLE;
	}

	unsigned int addr = INVALID_PAGE;
	if (searchFlags & EXACT_ADDRESS)
		addr = reinterpret_cast<unsigned int>(*clone_addr);
		
	// It is important that CreateArea not incur a fault!		
	char nameCopy[OS_NAME_LENGTH];
	if (!CopyUser(nameCopy, name, OS_NAME_LENGTH))
		return E_BAD_ADDRESS;

	Area *newArea = AddressSpace::GetCurrentAddressSpace()->CreateArea(nameCopy, area->GetSize(), AREA_NOT_WIRED,
		protection | USER_READ | SYSTEM_READ | ((protection & USER_WRITE)
		? SYSTEM_WRITE : 0), area->GetPageCache(), 0, addr, searchFlags);
	area->ReleaseRef();
	if (newArea == 0)
		return E_ERROR;

	*clone_addr = reinterpret_cast<void*>(newArea->GetBaseAddress());
	return OpenHandle(newArea);
}

int delete_area(int handle)
{
	Area *area = static_cast<Area*>(GetResource(handle, OBJ_AREA));
	if (area == 0)
		return E_BAD_HANDLE;

	Thread::GetRunningThread()->GetTeam()->GetHandleTable()->Close(handle);
	AddressSpace::GetCurrentAddressSpace()->DeleteArea(area);
	area->ReleaseRef();
	return E_NO_ERROR;
}

// Only works for areas in the current team.
int resize_area(int area_id, unsigned int newSize)
{
	Area *area = static_cast<Area*>(GetResource(area_id, OBJ_AREA));
	if (area == 0)
		return E_BAD_HANDLE;

	int error = AddressSpace::GetCurrentAddressSpace()->ResizeArea(area, newSize);
	area->ReleaseRef();
	return error;
}

void kill_apc(void *thread)
{
	static_cast<Thread*>(thread)->Exit();
}

int kill_thread(int handle)
{
	Thread *thread = static_cast<Thread*>(GetResource(handle, OBJ_THREAD));
	if (thread == 0)
		return E_BAD_HANDLE;

	if (thread == Thread::GetRunningThread()) {
		thread->ReleaseRef();
		thread->Exit();
	} else {
		APC *apc = new APC;
		apc->fCallback = kill_apc;
		apc->fIsKernel = true;
		apc->fData = thread;
		thread->EnqueueAPC(apc);
		thread->ReleaseRef();
	}
	
	return E_NO_ERROR;
}

int open(const char path[], int)
{
	VNode *node;
	int error = FileSystem::WalkPath(path, strlen(path), &node);
	if (error < 0)
		return error;

	FileDescriptor *desc;
	error = node->Open(&desc);
	if (error < 0) {
		node->ReleaseRef();
		return error;
	}

	desc->SetName(path);
	return OpenHandle(desc);
}

status_t close_handle(int handle)
{
	Resource *resource = static_cast<FileDescriptor*>(GetResource(handle, OBJ_ANY));
	if (resource == 0)
		return E_BAD_HANDLE;

	Thread::GetRunningThread()->GetTeam()->GetHandleTable()->Close(handle);
	resource->ReleaseRef();
	return 0;
}

status_t readdir(int handle, char out_path[], size_t length)
{
	FileDescriptor *descriptor = static_cast<FileDescriptor*>(GetResource(handle, OBJ_FD));
	if (descriptor == 0)
		return E_BAD_HANDLE;

	status_t ret = descriptor->ReadDir(out_path, length);
	descriptor->ReleaseRef();
	return ret;
}

status_t mkdir(const char path[], mode_t)
{
	VNode *dir;
	char entry[256];
	int error;
	
	error = FileSystem::GetDirEntry(path, strlen(path), entry, 256, &dir);
	if (error < 0)
		return error;

	error = dir->MakeDir(entry, strlen(entry));
	dir->ReleaseRef();
	return error;
}

int rmdir(const char path[])
{
	VNode *dir;
	char entry[256];
	int error;
	
	error = FileSystem::GetDirEntry(path, strlen(path), entry, 256, &dir);
	if (error < 0)
		return error;

	error = dir->RemoveDir(entry, strlen(entry));
	dir->ReleaseRef();
	return error;
}

ssize_t read(int handle, void *out, size_t length)
{
	FileDescriptor *descriptor = static_cast<FileDescriptor*>(GetResource(handle, OBJ_FD));
	if (descriptor == 0)
		return E_BAD_HANDLE;

	ssize_t err = descriptor->Read(out, length);
	descriptor->ReleaseRef();
	return err;
}

ssize_t write(int handle, const void *in, size_t length)
{
	FileDescriptor *descriptor = static_cast<FileDescriptor*>(GetResource(handle, OBJ_FD));
	if (descriptor == 0)
		return E_BAD_HANDLE;

	ssize_t err = descriptor->Write(in, length);
	descriptor->ReleaseRef();
	return err;
}

ssize_t read_pos(int handle, off_t offs, void *out, size_t length)
{
	FileDescriptor *descriptor = static_cast<FileDescriptor*>(GetResource(handle, OBJ_FD));
	if (descriptor == 0)
		return E_BAD_HANDLE;

	ssize_t err = descriptor->ReadAt(offs, out, length);
	descriptor->ReleaseRef();
	return err;
}

ssize_t write_pos(int handle, off_t offs, const void *in, size_t length)
{
	FileDescriptor *descriptor = static_cast<FileDescriptor*>(GetResource(handle, OBJ_FD));
	if (descriptor == 0)
		return E_BAD_HANDLE;

	size_t ret = descriptor->WriteAt(offs, in, length);
	descriptor->ReleaseRef();
	return ret;
}

off_t lseek(int handle, off_t offs, int whence)
{
	FileDescriptor *descriptor = static_cast<FileDescriptor*>(GetResource(handle, OBJ_FD));
	if (descriptor == 0)
		return E_BAD_HANDLE;

	off_t ret = descriptor->Seek(offs, whence);
	descriptor->ReleaseRef();
	return ret;
}

int ioctl(int handle, int command, void *buffer)
{
	FileDescriptor *descriptor = static_cast<FileDescriptor*>(GetResource(handle, OBJ_FD));
	if (descriptor == 0)
		return E_BAD_HANDLE;

	int ret = descriptor->Control(command, buffer);
	descriptor->ReleaseRef();
	return ret;
}

int rewinddir(int handle)
{
	FileDescriptor *descriptor = static_cast<FileDescriptor*>(GetResource(handle, OBJ_FD));
	if (descriptor == 0)
		return E_BAD_HANDLE;

	int ret = descriptor->RewindDir();
	descriptor->ReleaseRef();
	return ret;
}

int rename(const char*, const char*)
{
	return E_INVALID_OPERATION;
}

status_t stat(const char*, struct stat*)
{
	return E_INVALID_OPERATION;
}

status_t mount(const char device[], const char dir[], const char type[], int,
	char*)
{
	int error;
	int devfd = -1;
	if (strlen(device) > 0) {
		devfd = open(device, 0);
		if (devfd < 0) {
			printf("mount: error opening device\n");
			return E_NO_SUCH_FILE;
		}
	}
	
	VNode *node;
	error = FileSystem::WalkPath(dir, strlen(dir), &node);
	if (error < 0) {
		printf("mount: mount point does not exist\n");
		close_handle(devfd);
		return error;
	}

	if (node->GetCoveredBy() != 0) {
		// Attempting to re-mount an already mounted directory
		printf("mount: filesystem already mounted at this point\n");
		node->ReleaseRef();
		close_handle(devfd);
		return E_NOT_ALLOWED;
	}

	FileSystem *fs;
	error = FileSystem::InstantiateFsType(type, devfd, &fs);
	if (error < 0) {
		node->ReleaseRef();
		close_handle(devfd);
		return error;
	}
	
	node->CoverWith(fs);
	fs->Cover(node);
	return 0;
}

status_t chdir(const char path[])
{
	VNode *dir;
	int error = FileSystem::WalkPath(path, strlen(path), &dir);
	if (error < 0)
		return error;

	VNode *subdir;
	error = dir->Lookup(".", 1, &subdir);
	if (error < 0) {
		dir->ReleaseRef();
		return E_NO_SUCH_FILE;
	}

	subdir->ReleaseRef();

	Thread::GetRunningThread()->SetCurrentDir(dir);
	return E_NO_ERROR;
}

char* getcwd(char buf[], size_t)
{
	strcpy(buf, "");
	printf("implement getcwd\n");
	return buf;
}

int map_file(const char path[], unsigned int va, off_t offset, size_t size, int flags)
{
	return CreateFileArea(path, path, va, offset, size, flags,
		USER_READ | USER_WRITE | SYSTEM_READ | SYSTEM_WRITE,
		*Thread::GetRunningThread()->GetTeam());
}

int CreateFileArea(const char name[], const char path[], unsigned int va, off_t offset,
	size_t size, int flags, PageProtection prot, Team &team)
{
	VNode *node;
	int error;
	
	if (offset % PAGE_SIZE) {
		printf("Unaligned file offset for area\n");
		return E_INVALID_OPERATION; // File offset must be page aligned
	}

	error = FileSystem::WalkPath(path, strlen(path), &node);
	if (error < 0) {
		printf("map_file: file not found\n");
		return error;
	}

	PageCache *cache = node->GetPageCache();

	// If this is a private area, construct a copy cache
	if ((flags & MAP_PRIVATE) != 0) {
		cache = new PageCache(0, cache);
		if (cache == 0) {
			node->ReleaseRef();
			return E_NO_MEMORY;
		}
	}

	// It is important that CreateArea not incur a fault!		
	char nameCopy[OS_NAME_LENGTH];
	if (!CopyUser(nameCopy, name, OS_NAME_LENGTH))
		return E_BAD_ADDRESS;

	Area *area = team.GetAddressSpace()->CreateArea(nameCopy, size, AREA_NOT_WIRED,
		prot, cache, offset, va, flags);
		
	if (area == 0) {
		printf("CreateArea failed\n");
		node->ReleaseRef();
		return E_ERROR;
	}
		
	return team.GetHandleTable()->Open(area);
}

int exec(const char path[])
{
	Image image;
	if (image.Open(path) != E_NO_ERROR)
		return E_NO_SUCH_FILE;

	Team *newTeam = new Team(path);
	if (newTeam == 0)
		return E_NO_MEMORY;

	if (image.Load(*newTeam) != E_NO_ERROR) {
		printf("error loading image\n");
		newTeam->ReleaseRef();
		return E_NOT_IMAGE;
	}

	const char *filename = path + strlen(path);
	while (filename > path && *filename != '/')
		filename--;
	
	char appName[OS_NAME_LENGTH];
	snprintf(appName, OS_NAME_LENGTH, "%.12s thread", filename + 1);

	thread_start_t entry = reinterpret_cast<thread_start_t>(image.GetEntryAddress());
	Thread *child = new Thread(appName, newTeam, entry, 0);
	if (child == 0) {
		newTeam->ReleaseRef();
		return E_NO_MEMORY;
	}
		
	return E_NO_ERROR;	// Maybe return a handle to the team, but then it would
						// have to be explicitly closed
}

status_t wait_for_multiple_objects(int handleCount, const object_id *handles,
	bigtime_t timeout, WaitFlags flags)
{
	if (handleCount <= 0)
		return E_INVALID_OPERATION;

	status_t result = E_NO_ERROR;
	Resource **resources = new Resource*[handleCount];
	for (int handleIndex = 0; handleIndex < handleCount; handleIndex++)
		resources[handleIndex] = 0;

	for (int handleIndex = 0; handleIndex < handleCount; handleIndex++) {
		object_id id;
		if (!CopyUser(&id, &handles[handleIndex], sizeof(id))) {
			result = E_BAD_ADDRESS;
			break;
		}

		resources[handleIndex] = GetResource(handles[handleIndex], OBJ_ANY);
		if (resources[handleIndex] == 0) {
			result = E_BAD_HANDLE;
			break;
		}
	}

	if (result == E_NO_ERROR) {
		result = Dispatcher::WaitForMultipleDispatchers(handleCount,
			reinterpret_cast<Dispatcher**>(resources), flags, timeout);
	}

	for (int handleIndex = 0; handleIndex < handleCount; handleIndex++)
		if (resources[handleIndex])
			resources[handleIndex]->ReleaseRef();	

	delete [] resources;
	return result;
}
