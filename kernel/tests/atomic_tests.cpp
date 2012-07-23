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

#include "../arch/x86/cpu_asm.h"

extern "C" int printf(const char *fmt, ...);

#define TEST(x) { if (!(x)) { printf("FAILED(%d): "#x"\n", __LINE__); return 0; } }

int main()
{
	volatile int v = 0;
	
	printf("testing\n");
	v = 12;
	TEST(cmpxchg32(&v, 12, 13));
	TEST(v == 13);
	TEST(!cmpxchg32(&v, 12, 14));
	TEST(v == 13);
	TEST(cmpxchg32(&v, 13, 14));
	TEST(v == 14);

	v = 0;	
	TEST(AtomicAdd(&v, 1) == 0);
	TEST(v == 1);
	TEST(AtomicAdd(&v, -1) == 1);
	TEST(v == 0);

	v = 0;
	TEST(AtomicOr(&v, 1) == 0);
	TEST(v == 1);
	TEST(AtomicOr(&v, 2) == 1);
	TEST(v == 3);

	v = 0;
	TEST(AtomicAnd(&v, 1) == 0);
	TEST(v == 0);
	v = 1;
	TEST(AtomicAnd(&v, 1) == 1);
	TEST(v == 1);
	
	printf("Passed\n");
}