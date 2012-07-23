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

#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>

static volatile int thread_count = 0;
static uchar *booty = 0;

static int vm_bang(void *str)
{
	for (int i = 0; i < 10; i++) {
		uint *addr;
		int area = create_area("original area", (void**) &addr, 0, 0x2000,
			AREA_NOT_WIRED, USER_READ | USER_WRITE);
		if (area < 0) {
			_serial_print("error creating original area\n");
			return 0;
		}
		
		unsigned var = rand();
		*addr = var;
		
		uint *clone_addr;
		int clone = clone_area("clone area", (void**) &clone_addr, 0,
			USER_WRITE | USER_READ, area);
		if (clone < 0) {
			_serial_print("error creating clone area\n");
			return 0;
		}
			
		if (*clone_addr != var) {
			_serial_print("clone failed to copy pages\n");
			return 0;
		}
		
		addr += 1024;
		clone_addr += 1024;
		*clone_addr = var;
		if (*addr != var) {
			_serial_print("page failed failed to be propigated\n");
			return 0;
		}
			
		for (int i = 0; i < 10; i++)
			resize_area(area, (i % 4) * PAGE_SIZE + PAGE_SIZE);
			
		delete_area(area);
		delete_area(clone);
	}

	printf("%s", (char*) str);
	atomic_add(&thread_count, -1);
	return 0;
}

static int starter(void*)
{
	for (;;) {
		atomic_add(&thread_count, 10);
		spawn_thread(vm_bang, "VM Bang", (void*) "1 ", 18);
		spawn_thread(vm_bang, "VM Bang", (void*) "2 ", 18);
		spawn_thread(vm_bang, "VM Bang", (void*) "3 ", 18);
		spawn_thread(vm_bang, "VM Bang", (void*) "4 ", 18);
		spawn_thread(vm_bang, "VM Bang", (void*) "5 ", 18);
		spawn_thread(vm_bang, "VM Bang", (void*) "6 ", 18);
		spawn_thread(vm_bang, "VM Bang", (void*) "7 ", 18);
		spawn_thread(vm_bang, "VM Bang", (void*) "8 ", 18);
		spawn_thread(vm_bang, "VM Bang", (void*) "9 ", 18);
		spawn_thread(vm_bang, "VM Bang", (void*) "10 ", 18);

#if SWAP_OUT
		if (cnt-- == 0) {
			cnt = 20;
			// Swap in some booty pages
			for (int i = 0; i < 0x600000; i += PAGE_SIZE) {
				if (booty[i] != 0x24)
					_serial_print("swap in goofed");
			}
		}
#endif

		// wait for threads to die
		while (thread_count)
			sleep(20000);
	}

	return 0;
}

int test_vm()
{
	// Map an area to steal from
	create_area("page booty", (void**) &booty, 0, 0x600000,
		AREA_NOT_WIRED, USER_READ | USER_WRITE);
	int b = 0;
	for (int i = 0; i < 0x600000; i += PAGE_SIZE) {
#if SWAP_OUT
		booty[i] = 0x24;
#else
		b += booty[i];
#endif
	}

	// Simple area test
	spawn_thread(starter, "starter1", 0, 5);
	spawn_thread(starter, "starter2", 0, 5);
	spawn_thread(starter, "starter3", 0, 5);
	spawn_thread(starter, "starter4", 0, 5);
	spawn_thread(starter, "starter5", 0, 5);
	return b;
}

void test_fault()
{
	char buf[1024];
	int fd = open("/boot/datafile.txt", O_RDWR);
	if (fd < 0) {
		printf("couldn't open data file (oops)\n");
		return;
	}
	
	printf("test on good buffer...   ");
	if (read(fd, buf, 1024) > 0)
		printf("passed\n");
	else
		printf("returned error... oops\n");
	
	printf("test on bad buffer...   ");
	if (read(fd, 0, 1024) == E_BAD_ADDRESS)
		printf("returned E_BAD_ADDRESS (passed)\n");
	else
		printf("Didn't return correct error... oops\n");

	close_handle(fd);
}

void time_faults()
{
	const int FAULT_PAGES = 100;
	char *addr;
	int area = create_area("fault area", (void**) &addr, 0, FAULT_PAGES
		* PAGE_SIZE, AREA_NOT_WIRED, USER_READ | USER_WRITE);

	bigtime_t start = system_time();
	char *c = addr;
	for (int i = 0; i < FAULT_PAGES; i++) {
		*c = 0;
		c += PAGE_SIZE;
	}
	
	bigtime_t time1 = system_time() - start;
	start = system_time();
	c = addr;
	for (int i = 0; i < FAULT_PAGES; i++) {
		*c = 0;
		c += PAGE_SIZE;
	}
	
	bigtime_t time2 = system_time() - start;
	printf("\n%d pages.  fault time %Ldus (%Ldus per fault) no fault time %Ldus (%Ldus per fault)\n",
		FAULT_PAGES, time1, time1 / FAULT_PAGES, time2, time2 / FAULT_PAGES);

	delete_area(area);
}
