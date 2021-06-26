#ifndef MONSOON_INTRINSIC_H
#define MONSOON_INTRINSIC_H

#include <math.h> // TODO : Get rid math.h

inline u32
RoundR32ToUInt32(r32 Value)
{
    return (u32)roundf(Value);
}

inline i32
RoundR32ToInt32(r32 Value)
{
    // TODO : Intrinsic
        return (i32)roundf(Value);
}

inline i32
TruncateR32ToInt32(r32 Value)
{
    // TODO : Intrinsic
    return (i32)Value;
}

inline i32
FloorR32ToInt32(r32 Value)
{
    // TODO : Intrinsic
    return (i32)floor(Value);
}

internal void
SwapU32(u32 *X, u32 *Y)
{
    u32 Temp = *X;
    *X = *Y;
    *Y = Temp;
}

inline u32
FindLeastSignificantSetBit(u32 Value)
{
    u32 Result = 0;

#if 0

    for(u32 Index = 0;
        Index < 32;
        ++Index)
    {
        if(Value >> Index & 1)
        {
            Result = Index;
            break;
        }
    }
#else
    Result = __builtin_ctz(Value);
#endif

    return Result;
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
atan(r32 y, r32 x)
{
    return atan2f(y, x);
}

inline r32
SquareRoot2(r32 Value)
{
    return sqrtf(Value);
}

#endif
