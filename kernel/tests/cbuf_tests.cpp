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

