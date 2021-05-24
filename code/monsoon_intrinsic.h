#ifndef MONSOON_INTRINSIC_H
#define MONSOON_INTRINSIC_H

#include <math.h> // TODO : Get rid math.h

inline i32
RoundR32ToInt32(r32 Value)
{
    return (i32)(Value + 0.5f);
}

inline i32
TruncateR32ToInt32(r32 Value)
{
    return (i32)Value;
}

internal i32
FloorR32ToInt32(r32 Value)
{
    return (i32)floor(Value);
}

#endif
