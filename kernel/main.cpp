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
