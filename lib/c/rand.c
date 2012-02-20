#include <stdlib.h>

static int seed = 0;

void srand(int new_seed)
{
	seed = new_seed;
}

int rand_r(int *seed)
{
	int result;

    *seed = *seed * 1103515245 + 12345;
	result = *seed;

    *seed = *seed * 1103515245 + 12345;
	result = (result >> 24) ^ (result << 7) ^ *seed;

    *seed = *seed * 1103515245 + 12345;
	result = (result >> 24) ^ (result << 7) ^ *seed;

    return result & 0x7fffffff;
}

int rand()
{
	return rand_r(&seed);
}


