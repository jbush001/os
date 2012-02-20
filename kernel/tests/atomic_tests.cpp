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