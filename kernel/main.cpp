#include "AddressSpace.h"
#include "Processor.h"
#include "KernelDebug.h"
#include "FileSystem.h"
#include "interrupt.h"
#include "Page.h"
#include "PageCache.h"
#include "PhysicalMap.h"
#include "syscall.h"
#include "Team.h"
#include "Thread.h"
#include "Timer.h"
#include "types.h"

int main()
{
	KernelDebugBootstrap();
	Thread::Bootstrap();
	InterruptBootstrap();
	Timer::Bootstrap();
	Page::Bootstrap();
	PageCache::Bootstrap();
	PhysicalMap::Bootstrap();
	AddressSpace::Bootstrap();
	Team::Bootstrap();
	Processor::Bootstrap();
	FileSystem::Bootstrap();
	Page::StartPageEraser();

	exec("/boot/net_server");
	exec("/boot/shell");

	AddressSpace::PageDaemonLoop(); // This thread becomes the page daemon.
}
