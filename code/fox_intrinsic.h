/* 
 
Author : Gyuhyeon, Lee 
Email : weanother@gmail.com 
Copyright by GyuHyeon, Lee. All Rights Reserved. 
 
*/

#ifndef FOX_INTRINSIC_H

#include <math.h>
// #include <intrin.h>
//

inline i32
Round(r32 value)
{
    return (i32)(value + 0.5f);
}

inline r32
Cos(r32 rad)
{
	return cosf(rad);
}

inline r32 
Sin(r32 rad)
{
	return sinf(rad);
}

inline r32
Sqrt(r32 value)
{
	Assert(value >= 0.0f);
	return sqrtf(value);
}


#define FOX_INTRINSIC_H
#endif
