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

#include <stdio.h>

#define panic printf
#define TEST(x) { if (!(x)) { printf("FAIL (line %d): "#x"\n", __LINE__); exit(1); } }
#define ASSERT(x) TEST(x)
#define SIZE 10

#include "CircularBuffer.h"


int main()
{
	CircularBuffer<int, SIZE> buf;
	TEST(buf.IsEmpty());
	TEST(!buf.IsFull());	
	TEST(buf.CountQueuedItems() == 0);
	for (int i = 0; i < SIZE - 1; i++) {
		buf.Insert(i);
		TEST(buf.CountQueuedItems() == i + 1);
		TEST(!buf.IsEmpty());
		TEST(!buf.IsFull());
	}

	buf.Insert(SIZE - 1);
	TEST(!buf.IsEmpty());
	TEST(buf.IsFull());
	TEST(buf.CountQueuedItems() == SIZE);
	
	for (int i = 0; i < SIZE - 1; i++) {
		TEST(buf.Remove() == i);
		TEST(buf.CountQueuedItems() == SIZE - i - 1);
		TEST(!buf.IsEmpty());
		TEST(!buf.IsFull());
	}

	TEST(buf.Remove() == SIZE - 1);
	TEST(buf.IsEmpty());
	TEST(!buf.IsFull());
	TEST(buf.CountQueuedItems() == 0);

	// now test overlap conditions
	for (int j = 1; j < SIZE; j++) {
		for (int i = 0; i < SIZE; i++) {
			for (int k = 0; k < j; k++) {
				TEST(buf.CountQueuedItems() == k);
				buf.Insert(k);
			}

			TEST(!buf.IsEmpty());
			TEST(buf.CountQueuedItems() == j);
				
			for (int k = 0; k < j; k++)
				TEST(buf.Remove() == k);
			
			TEST(buf.IsEmpty());
			TEST(!buf.IsFull());
			TEST(buf.CountQueuedItems() == 0);
		}
	}

	// multi insert/remove
	int arr[SIZE];
	for (int j = 1; j < SIZE; j++) {
		for (int i = 0; i < SIZE; i++) {
			for (int k = 0; k < SIZE; k++)
				arr[k] = k < j ? k : 0;				

			TEST(buf.Insert(arr, j) == j);

			TEST(!buf.IsEmpty());
			TEST(buf.CountQueuedItems() == j);

			for (int k = 0; k < SIZE; k++)
				arr[k] = 0xffffffff;				
				
			TEST(buf.Remove(arr, j) == j);

			for (int k = 0; k < SIZE; k++)
				TEST(arr[k] == k < j ? k : 0xffffffff);				
			
			TEST(buf.IsEmpty());
			TEST(!buf.IsFull());
			TEST(buf.CountQueuedItems() == 0);
		}
	}

	// Truncate
	TEST(buf.Insert(arr, SIZE * 2) == SIZE);
	TEST(buf.Remove(arr, SIZE * 2) == SIZE);

	printf("PASSED\n");
}

