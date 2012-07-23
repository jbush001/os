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

static int fp_worker(void*)
{
	float a = 0.0;
	float b = 1.0;
	int update = 0;
	while (true) {
		b += 0.01;
		a = 2 * b;
		if (a != 2 * b)
			_serial_print("FP error\n");
			
		if (update++ > 10000) {
			printf(".");
			update = 0;
		}
	}
}

void test_fp()
{
	spawn_thread(fp_worker, "fp_worker", (void*) "-", 16);	
	spawn_thread(fp_worker, "fp_worker", (void*) "+", 16);
}

