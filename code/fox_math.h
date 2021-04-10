/* 
 
Author : Gyuhyeon, Lee 
Email : weanother@gmail.com 
Copyright by GyuHyeon, Lee. All Rights Reserved. 
 
*/

#ifndef FOX_MATH_H
#include <math.h>

#define Minimum(a, b) ((a < b) ? a : b)
#define Maximum(a, b) ((a > b)? a : b)

inline i32
Abs(i32 value) 
{
	i32 result = value;
	if(result < 0)
	{
		result *= -1;
	}
	return result;
}

inline r32
Abs(r32 value) 
{
	r32 result = value;
	if(result < 0)
	{
		result *= -1.0f;
	}
	return result;
}

inline r32
Modular(r32 value, r32 devider)
{
	r32 result = value - (i32)(value/devider)*devider;

	if(result < 0.0f)
	{
		result += devider;
	}

	return result;
}

inline r32
ModularMinus(r32 minusValue, r32 devider)
{
	return Modular(minusValue, devider) - minusValue;
}

inline b32
IsBetweenExclude(r32 a, r32 value, r32 b)
{
	b32 result = false;
	if(value > a && value < b)
	{
		result = true;
	}
	return result;
}

inline b32
IsBetweenInclude(r32 a, r32 value, r32 b)
{
	b32 result = false;
	if(value >= a && value <= b)
	{
		result = true;
	}
	return result;
}

inline b32
IsBetween(r32 a, r32 value, r32 b)
{
	b32 result = false;
	if(value >= a && value < b)
	{
		result = true;
	}
	return result;
}

inline r32
Lerp(r32 min, r32 t, r32 max)
{
	return (1.0f-t)*min + t*max;
}




/*
	V2
*/

inline v2
V2(r32 x, r32 y)
{
	v2 result = {};
	result.x = x;
	result.y = y;

	return result;
}

inline v2
V2(void)
{
	v2 result = {};
	return result;
}

inline v2
V2(r32 value)
{
	v2 result = {};
	result.x = value;
	result.y = value;
	return result;
}

inline v2
operator+(v2 a, v2 b)
{
	v2 result = {};
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	return result;
}

inline v2
operator-(v2 a, v2 b)
{
	v2 result = {};
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	return result;
}

inline v2
operator*(r32 value, v2 a)
{
	v2 result = a;
	result.x *= value;
	result.y *= value;

	return result;
}

inline v2
operator/(v2 a, r32 value)
{
	v2 result = a;
	result.x /= value;
	result.y /= value;
	return result;
}

inline v2
operator/(v2 a, v2 b)
{
	v2 result = a;
	result.x /= b.x;
	result.y /= b.y;
	return result;
}

inline v2 &
operator+=(v2 &a, v2 b)
{
    a = a + b;

    return a;
}

inline v2 &
operator-=(v2 &a, v2 b)
{
    a = a - b;

    return a;
}

inline v2 &
operator/=(v2 &a, r32 value)
{
	a.x /= value;
	a.y /= value;

	return a;
}
inline v2
Lerp(v2 min, r32 t, v2 max)
{
	return V2(Lerp(min.x, t, max.x), Lerp(min.y, t, max.y));
}

inline r32
LengthSq(v2 a)
{
	r32 result = a.x*a.x + a.y*a.y;
	return result;
}

inline r32
Length(v2 a)
{
	return sqrtf(LengthSq(a));
}

inline r32
Inner(v2 a, v2 b)
{
	r32 result = a.x*b.x + a.y*b.y;
	return result;
}

/*
	V2i
*/

inline v2i
V2i(i32 x, i32 y)
{
	v2i result = {};
	result.x = x;
	result.y = y;

	return result;
}

inline v2i
operator+(v2i a, v2i b)
{
	v2i result = {};
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	return result;
}

inline v2i
operator-(v2i a, v2i b)
{
	v2i result = {};
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	return result;
}

inline v2i &
operator+=(v2i &a, v2i b)
{
    a = a + b;
    return a;
}

inline v2i &
operator-=(v2i &a, v2i b)
{
    a = a - b;
    return a;
}

/*
	V3
*/

inline v3
V3(r32 x, r32 y, r32 z)
{
	v3 result = {};
	result.x = x;
	result.y = y;
	result.z = z;

	return result;
}

inline v3
V3(v2 xy, r32 z)
{
	v3 result = {};
	result.xy = xy;
	result.z = z;

	return result;
}

inline v3
V3(void)
{
	v3 result = {};
	return result;
}

inline v3
V3(r32 value)
{
	v3 result = {};
	result.x = value;
	result.y = value;
	result.z = value;
	return result;
}

inline v3
operator+(v3 a, v3 b)
{
	v3 result = {};
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	result.z = a.z + b.z;
	return result;
}

inline v3
operator-(v3 a, v3 b)
{
	v3 result = {};
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	result.z = a.z - b.z;
	return result;
}

inline v3
operator*(r32 value, v3 a)
{
	v3 result = a;
	result.x *= value;
	result.y *= value;
	result.z *= value;

	return result;
}

inline v3
operator*(v3 a, r32 value)
{
	v3 result = a;
	result.x *= value;
	result.y *= value;
	result.z *= value;

	return result;
}

inline v3
operator/(v3 a, r32 value)
{
	v3 result = a;
	result.x /= value;
	result.y /= value;
	result.z /= value;
	return result;
}

inline v3
operator-(v3 a)
{
	v3 result = {};

	result.x = -a.x;
	result.y = -a.y;
	result.z = -a.z;
	return result;
}

inline v3 &
operator+=(v3 &a, v3 b)
{
    a = a + b;
    return a;
}

inline v3 &
operator-=(v3 &a, v3 b)
{
    a = a - b;
    return a;
}

inline v3 &
operator*=(v3 &a, r32 value)
{
	a.x *= value;
	a.y *= value;
	a.z *= value;

	return a;
}

inline v3 &
operator/=(v3 &a, r32 value)
{
	a.x /= value;
	a.y /= value;
	a.z /= value;

	return a;
}

inline r32
LengthSq(v3 a)
{
	r32 result = a.x*a.x + a.y*a.y + a.z*a.z;
	return result;
}

inline r32
Length(v3 a)
{
	return sqrtf(LengthSq(a));
}

inline r32
Inner(v3 a, v3 b)
{
	r32 result = a.x*b.x + a.y*b.y + a.z*b.z;
	return result;
}

inline v2
Normalize(v2 a)
{
	a /= Length(a);
	return a;
}

inline v3
Normalize(v3 a)
{
	a /= Length(a);
	return a;
}

/*
	V3i
*/

inline v3i
V3i(i32 x, i32 y, i32 z)
{
	v3i result = {};
	result.x = x;
	result.y = y;
	result.z = z;

	return result;
}

inline v3i
V3iMax()
{
	return V3i(INT_MAX, INT_MAX, INT_MAX);
}

inline v3i
operator+(v3i a, v3i b)
{
	v3i result = {};
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	result.z = a.z + b.z;
	return result;
}

inline v3i
operator-(v3i a, v3i b)
{
	v3i result = {};
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	result.z = a.z - b.z;
	return result;
}

inline v3i &
operator+=(v3i &a, v3i b)
{
    a = a + b;
    return a;
}

inline v3i &
operator-=(v3i &a, v3i b)
{
    a = a - b;
    return a;
}

inline b32
operator==(v3i a, v3i b)
{
	b32 result = false;
	if(a.x == b.x &&
		a.y == b.y &&
		a.z == b.z)
	{
		result = true;
	}

	return result;
}


/*
	V3u
*/

inline v3u
V3u(u32 x, u32 y, u32 z)
{
	v3u result = {};
	result.x = x;
	result.y = y;
	result.z = z;

	return result;
}

inline v3u
V3uMax()
{
	return V3u(UINT_MAX, UINT_MAX, UINT_MAX);
}

inline v3u
operator+(v3u a, v3u b)
{
	v3u result = {};
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	result.z = a.z + b.z;
	return result;
}

inline v3u
operator-(v3u a, v3u b)
{
	v3u result = {};
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	result.z = a.z - b.z;
	return result;
}

inline v3u
operator*(u32 value, v3u a)
{
	v3u result = a;
	result.x *= value;
	result.y *= value;
	result.z *= value;

	return result;
}

inline v3u
operator/(v3u a, u32 value)
{
	v3u result = a;
	result.x /= value;
	result.y /= value;
	result.z /= value;
	return result;
}

inline v3u &
operator+=(v3u &a, v3u b)
{
    a = a + b;
    return a;
}

inline v3u &
operator-=(v3u &a, v3u b)
{
    a = a - b;
    return a;
}

inline v3u &
operator*=(v3u &a, u32 value)
{
	a.x *= value;
	a.y *= value;
	a.z *= value;

	return a;
}

inline v3u &
operator/=(v3u &a, u32 value)
{
	a.x /= value;
	a.y /= value;
	a.z /= value;

	return a;
}

inline b32
operator==(v3u a, v3u b)
{
	b32 result = false;
	if(a.x == b.x &&
		a.y == b.y &&
		a.z == b.z)
	{
		result = true;
	}

	return result;
}

/*
	V4
*/

inline v4
V4(r32 x, r32 y, r32 z, r32 w)
{
	v4 result = {};
	result.x = x;
	result.y = y;
	result.z = z;
	result.w = w;

	return result;
}

inline v4
operator+(v4 a, v4 b)
{
	v4 result = {};
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	result.z = a.z + b.z;
	result.w = a.w + b.w;
	return result;
}

inline v4
operator-(v4 a, v4 b)
{
	v4 result = {};
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	result.z = a.z - b.z;
	result.w = a.w - b.w;
	return result;
}

inline v4
operator/(v4 a, r32 value)
{
	v4 result = a;

	result.x /= value;
	result.y /= value;
	result.z /= value;
	result.w /= value;

	return result;
}

inline v4
operator*(r32 value, v4 a)
{
	v4 result = a;

	result.x *= value;
	result.y *= value;
	result.z *= value;
	result.w *= value;

	return result;
}

inline v4 &
operator+=(v4 &a, v4 b)
{
    a = a + b;
    return a;
}

inline v4 &
operator-=(v4 &a, v4 b)
{
    a = a - b;
    return a;
}




struct rt2
{
	v2 min;
	v2 max;
};

inline b32
IsPointInsideRectangle(v2 point, rt2 rectangle)
{
	b32 result = false;
	if(point.x >= rectangle.min.x && 
		point.y >= rectangle.min.y && 
		
		point.x < rectangle.max.x &&
		point.y < rectangle.max.y)
	{
		result = true;
	}
	return result;
};

struct rt2i
{
	v2i min;
	v2i max;
};

inline b32
IsPointInsideRectangle(v2i point, rt2i rectangle)
{
	b32 result = false;
	if(point.x >= rectangle.min.x && 
		point.y >= rectangle.min.y && 

		point.x < rectangle.max.x &&
		point.y < rectangle.max.y)
	{
		result = true;
	}
	return result;
};

struct rt3
{
	v3 min;
	v3 max;
};

inline b32
IsPointInsideRectangle(v3 point, rt3 rectangle)
{
	b32 result = false;

	if(point.x >= rectangle.min.x && 
		point.y >= rectangle.min.y && 
		point.z >= rectangle.min.z &&

		point.x < rectangle.max.x &&
		point.y < rectangle.max.y &&
		point.z < rectangle.max.z)
	{
		result = true;
	}

	return result;
};

struct rt3i
{
	v3i min;
	v3i max;
};

inline b32
IsPointInsideRectangle(v3i point, rt3i rectangle)
{
	b32 result = false;
	if(point.x >= rectangle.min.x && 
		point.y >= rectangle.min.y && 
		point.z >= rectangle.min.z &&

		point.x < rectangle.max.x &&
		point.y < rectangle.max.y &&
		point.z < rectangle.max.z)
	{
		result = true;
	}

	return result;
};

#define FOX_MATH_H
#endif
