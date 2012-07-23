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

int victim1(void*)
{
	acquire_sem(create_sem("test", 0), INFINITE_TIMEOUT);
}

int victim2(void*)
{
	for (;;)
		printf("victim2\n");
}

void test_kill_thread()
{
	int vthr;
	
	printf("spawn victim thread 1\n");
	vthr = spawn_thread(victim1, "victim", 0, 16);
	sleep(250000);
	kill_thread(vthr);
	printf("killed it\n");

	printf("spawn victim thread 2\n");
	vthr = spawn_thread(victim2, "victim", 0, 16);
	sleep(250000);
	kill_thread(vthr);
	printf("killed it\n");
}
