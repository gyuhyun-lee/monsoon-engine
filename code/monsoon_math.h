#ifndef MONSOON_MATH_H
#define MONSOON_MATH_H


inline r32
Clamp(r32 value, r32 min, r32 max)
{
    if(value < min)
    {
        value = min;
    }
    else if(value > max)
    {
        value = max;
    }

    return value;
}

inline r32
Clamp01(r32 value)
{
    if(value < 0.0f)
    {
        value = 0.0f;
    }
    else if(value > 1.0f)
    {
        value = 1.0f;
    }

    return value;
}

inline r32 Square(r32 Value)
{
    return Value*Value;
}

inline r32 
LinearInterpolation(r32 Min, r32 t, r32 Max)
{
    return Min*(1.0f-t) + Max*t;
}

union v2
{
    struct 
    {
        r32 x, y;
    };

    r32 e[2];
};

inline v2 V2(r32 x, r32 y)
{
    v2 result;

    result.x = x;
    result.y = y;

    return result;
}

inline v2 operator+(v2 A, v2 B)
{
    v2 result;

    result.x = A.x + B.x;
    result.y = A.y + B.y;

    return result;
}

inline v2 operator-(v2 A, v2 B)
{
    v2 result;

    result.x = A.x - B.x;
    result.y = A.y - B.y;

    return result;
}

inline v2 operator*(r32 Value, v2 A)
{
    v2 result;

    result.x = Value * A.x;
    result.y = Value * A.y;

    return result;
}

inline v2 &operator+=(v2 &A, v2 B)
{
    A.x += B.x;
    A.y += B.y;

    return A;
}

inline v2 &operator-=(v2 &A, v2 B)
{
    A.x -= B.x;
    A.y -= B.y;

    return A;
}

inline v2 operator-(v2 &A)
{
    v2 result = {};

    result.x = -A.x;
    result.y = -A.y;

    return result;
}

inline v2 &operator*=(v2 &A, r32 Value)
{
    A.x *= Value;
    A.y *= Value;

    return A;
}

inline v2 Hadamard(v2 A, v2 B)
{
    v2 result;

    result.x = A.x * B.x;
    result.y = A.y * B.y;

    return result;
}

inline r32 Dot(v2 A, v2 B)
{
    r32 result = A.x*B.x + A.y*B.y;
    return result;
}

inline r32 LengthSquare(v2 A)
{
    return A.x * A.x + A.y * A.y;
}

inline r32 Length(v2 A)
{
    return sqrt(LengthSquare(A));
}

inline v2 
Normalize(v2 A)
{
    v2 result = A;
    result *= 1.0f/Length(A);

    return result;
}

inline v2 
V2i(i32 x, i32 y)
{
    v2 result;

    result.x = x;
    result.y = y;

    return result;
}
inline v2
V2u(u32 x, u32 y)
{
    v2 result;

    result.x = x;
    result.y = y;

    return result;
}

union v3
{
    struct 
    {
        r32 x, y, z;
    };

    struct
    {
        v2 xy;
        r32 ignored0;
    };
    struct
    {
        r32 ignored1;
        v2 yz;
    };

    r32 E[3];
};

inline v3
V3(r32 x, r32 y, r32 z)
{
    v3 result = {};

    result.x = x;
    result.y = y;
    result.z = z;

    return result;
}

inline v3 operator+(v3 A, v3 B)
{
    v3 result;

    result.x = A.x + B.x;
    result.y = A.y + B.y;
    result.z = A.z + B.z;

    return result;
}

inline v3 operator-(v3 A, v3 B)
{
    v3 result;

    result.x = A.x - B.x;
    result.y = A.y - B.y;
    result.z = A.z - B.z;

    return result;
}

inline v3 operator*(r32 Value, v3 A)
{
    v3 result;

    result.x = Value * A.x;
    result.y = Value * A.y;
    result.z = Value * A.z;

    return result;
}

inline v3 &operator+=(v3 &A, v3 B)
{
    A.x += B.x;
    A.y += B.y;
    A.z += B.z;

    return A;
}

inline v3 
operator/(v3 a, r32 value)
{
    v3 result;

    r32 oneOverValue = 1.0f/value;
    result.x = a.x*oneOverValue;
    result.y = a.y*oneOverValue;
    result.z = a.z*oneOverValue;

    return result;
}

inline v3 &operator-=(v3 &A, v3 B)
{
    A.x -= B.x;
    A.y -= B.y;
    A.z -= B.z;

    return A;
}

inline v3 operator-(v3 &A)
{
    v3 result = {};

    result.x = -A.x;
    result.y = -A.y;
    result.z = -A.z;

    return result;
}

inline v3 &operator*=(v3 &A, r32 Value)
{
    A.x *= Value;
    A.y *= Value;
    A.z *= Value;

    return A;
}

inline v3 Hadamard(v3 A, v3 B)
{
    v3 result;

    result.x = A.x * B.x;
    result.y = A.y * B.y;
    result.z = A.z * B.z;

    return result;
}

inline r32 Dot(v3 A, v3 B)
{
    r32 result = A.x*B.x + A.y*B.y + A.z*B.z;
    return result;
}

inline r32 LengthSquare(v3 A)
{
    return A.x * A.x + A.y * A.y + A.z*A.z;
}

inline r32 Length(v3 A)
{
    return sqrt(LengthSquare(A));
}

inline v3 
Normalize(v3 A)
{
    v3 result = A;
    result *= 1.0f/Length(A);

    return result;
}

inline v3
V3u(u32 x, u32 y, u32 z)
{
    v3 result;

    result.x = x;
    result.y = y;
    result.z = z;

    return result;
}

inline v3
V3i(i32 x, i32 y, i32 z)
{
    v3 result;

    result.x = x;
    result.y = y;
    result.z = z;

    return result;
}

union v4
{
    struct 
    {
        r32 x, y, z, w;
    };

    struct 
    {
        v2 xy;
        r32 ignored1;
        r32 ignored2;
    };

    struct
    {
        v3 xyz;
        r32 ignored0;
    };

    struct
    {
        v3 rgb;
        r32 ignored3;
    };

    struct 
    {
        r32 r, g, b, a;
    };

    r32 E[4];
};

inline v4
V4(r32 x, r32 y, r32 z, r32 w)
{
    v4 result;
    
    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;

    return result;
}

inline v4
V4i(i32 x, i32 y, i32 z, i32 w)
{
    v4 result;

    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;
    
    return result;
}

inline v4
V4u(u32 x, u32 y, u32 z, u32 w)
{
    v4 result;

    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;

    return result;
}

inline v4
V4(v3 xyz, r32 w)
{
    v4 result;

    result.xyz = xyz;
    result.w = w;

    return result;
}

inline v4
operator+(v4 A, v4 B)
{
    v4 result;

    result.x = A.x + B.x;
    result.y = A.y + B.y;
    result.z = A.z + B.z;
    result.w = A.w + B.w;

    return result;
}

inline v4 operator-(v4 A, v4 B)
{
    v4 result;

    result.x = A.x - B.x;
    result.y = A.y - B.y;
    result.z = A.z - B.z;
    result.w = A.w - B.w;

    return result;
}

inline v4 operator*(r32 Value, v4 A)
{
    v4 result;

    result.x = Value * A.x;
    result.y = Value * A.y;
    result.z = Value * A.z;
    result.w = Value * A.w;

    return result;
}


inline v4 &operator+=(v4 &A, v4 B)
{
    A.x += B.x;
    A.y += B.y;
    A.z += B.z;
    A.w += B.w;

    return A;
}

inline v4 
operator/(v4 a, r32 value)
{
    v4 result;

    r32 oneOverValue = 1.0f/value;
    result.x = a.x*oneOverValue;
    result.y = a.y*oneOverValue;
    result.z = a.z*oneOverValue;
    result.w = a.w*oneOverValue;

    return result;
}

inline v4 &operator-=(v4 &A, v4 B)
{
    A.x -= B.x;
    A.y -= B.y;
    A.z -= B.z;
    A.w -= B.w;

    return A;
}

inline v4 operator-(v4 &A)
{
    v4 result = {};

    result.x = -A.x;
    result.y = -A.y;
    result.z = -A.z;
    result.w = -A.w;

    return result;
}

inline v4 &operator*=(v4 &A, r32 Value)
{
    A.x *= Value;
    A.y *= Value;
    A.z *= Value;
    A.w *= Value;

    return A;
}

inline v4 
Hadamard(v4 A, v4 B)
{
    v4 result;

    result.x = A.x * B.x;
    result.y = A.y * B.y;
    result.z = A.z * B.z;
    result.w = A.w * B.w;

    return result;
}

inline r32 
Dot(v4 A, v4 B)
{
    r32 result = A.x*B.x + A.y*B.y + A.z*B.z + A.w*B.w;
    return result;
}

inline r32 
LengthSquare(v4 A)
{
    return Dot(A, A);
}

inline r32 
Length(v4 A)
{
    return sqrt(Dot(A, A));
}

inline v4
Normalize(v4 A)
{
    v4 result = A;
    result *= 1.0f/Length(A);

    return result;
}

#endif
