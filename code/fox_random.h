#ifndef FOX_RANDOM_H
#define FOX_RANDOM_H

// TODO : Get this back to hexadecimal random numbers
#include <stdlib.h>
#include <time.h>

internal void
ChangeRandomSeed(void)
{
	time_t t;
	srand((unsigned)time(&t));
}

internal r32
Random0To1(void)
{
	return rand()/(r32)RAND_MAX;
}

internal r32
RandomBetween(r32 min, r32 max)
{
	return Lerp(min, Random0To1(), max);
}

#endif