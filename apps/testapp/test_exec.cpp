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

#include <types.h>
#include <syscall.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int exec_loop(void *name)
{
	for (;;) {
		exec((char*) name);
		sleep(200000 + rand() % 50000);
	}
}

void test_exec()
{
	spawn_thread(exec_loop, "exec0", (void*) "/boot/leaky", 5);	
	sleep(3700);
	spawn_thread(exec_loop, "exec1", (void*) "/boot/leaky", 5);
}


