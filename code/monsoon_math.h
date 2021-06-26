#ifndef MONSOON_MATH_H
#define MONSOON_MATH_H

inline r32 Square(r32 Value)
{
    return Value*Value;
}

inline r32 LinearInterpolation(r32 Min, r32 t, r32 Max)
{
    return Min*(1.0f-t) + Max*t;
}

struct v2
{
    union
    {
        struct 
        {
            r32 x, y;
        };

        r32 e[2];
    };
};

v2 V2(r32 x, r32 y)
{
    v2 Result;

    Result.x = x;
    Result.y = y;

    return Result;
}

v2 operator+(v2 A, v2 B)
{
    v2 Result;

    Result.x = A.x + B.x;
    Result.y = A.y + B.y;

    return Result;
}

v2 operator-(v2 A, v2 B)
{
    v2 Result;

    Result.x = A.x - B.x;
    Result.y = A.y - B.y;

    return Result;
}

v2 operator*(r32 Value, v2 A)
{
    v2 Result;

    Result.x = Value * A.x;
    Result.y = Value * A.y;

    return Result;
}

v2 &operator+=(v2 &A, v2 B)
{
    A.x += B.x;
    A.y += B.y;

    return A;
}

v2 &operator-=(v2 &A, v2 B)
{
    A.x -= B.x;
    A.y -= B.y;

    return A;
}

v2 operator-(v2 &A)
{
    v2 Result = {};

    Result.x = -A.x;
    Result.y = -A.y;

    return Result;
}

v2 &operator*=(v2 &A, r32 Value)
{
    A.x *= Value;
    A.y *= Value;

    return A;
}

v2 Hadamard(v2 A, v2 B)
{
    v2 Result;

    Result.x = A.x * B.x;
    Result.y = A.y * B.y;

    return Result;
}

r32 Dot(v2 A, v2 B)
{
    r32 Result = A.x*B.x + A.y*B.y;
    return Result;
}

r32 LengthSquare(v2 A)
{
    return A.x * A.x + A.y * A.y;
}

r32 Length(v2 A)
{
    return sqrt(LengthSquare(A));
}

v2 UnitLength(v2 A)
{
    v2 Result = A;
    Result *= 1.0f/Length(A);

    return Result;
}

struct v2u
{
    union
    {
        struct 
        {
            u32 x, y;
        };

        u32 E[2];
    };
};

v2u V2u(u32 x, u32 y)
{
    v2u Result;

    Result.x = x;
    Result.y = y;

    return Result;
}

v2u operator+(v2u A, v2u B)
{
    v2u Result;

    Result.x = A.x + B.x;
    Result.y = A.y + B.y;

    return Result;
}

v2u operator-(v2u A, v2u B)
{
    v2u Result;

    Result.x = A.x - B.x;
    Result.y = A.y - B.y;

    return Result;
}

v2u operator*(u32 Value, v2u A)
{
    v2u Result;

    Result.x = Value * A.x;
    Result.y = Value * A.y;

    return Result;
}

v2u &operator+=(v2u &A, v2u B)
{
    A.x += B.x;
    A.y += B.y;

    return A;
}

v2u &operator-=(v2u &A, v2u B)
{
    A.x -= B.x;
    A.y -= B.y;

    return A;
}

v2u operator-(v2u &A)
{
    v2u Result = {};

    Result.x = -A.x;
    Result.y = -A.y;

    return Result;
}

v2u &operator*=(v2u &A, u32 Value)
{
    A.x *= Value;
    A.y *= Value;

    return A;
}

struct v2i
{
    union
    {
        struct 
        {
            i32 x, y;
        };

        i32 E[2];
    };
};

v2i V2i(i32 x, i32 y)
{
    v2i Result;

    Result.x = x;
    Result.y = y;

    return Result;
}

v2i operator+(v2i A, v2i B)
{
    v2i Result;

    Result.x = A.x + B.x;
    Result.y = A.y + B.y;

    return Result;
}

v2i operator-(v2i A, v2i B)
{
    v2i Result;

    Result.x = A.x - B.x;
    Result.y = A.y - B.y;

    return Result;
}

v2i operator*(i32 Value, v2i A)
{
    v2i Result;

    Result.x = Value * A.x;
    Result.y = Value * A.y;

    return Result;
}

v2i &operator+=(v2i &A, v2i B)
{
    A.x += B.x;
    A.y += B.y;

    return A;
}

v2i &operator-=(v2i &A, v2i B)
{
    A.x -= B.x;
    A.y -= B.y;

    return A;
}

v2i operator-(v2i &A)
{
    v2i Result = {};

    Result.x = -A.x;
    Result.y = -A.y;

    return Result;
}

v2i &operator*=(v2i &A, i32 Value)
{
    A.x *= Value;
    A.y *= Value;

    return A;
}

struct v3
{
    union
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
};

inline v3
V3(r32 x, r32 y, r32 z)
{
    v3 Result = {};

    Result.x = x;
    Result.y = y;
    Result.z = z;

    return Result;
}

v3 operator+(v3 A, v3 B)
{
    v3 Result;

    Result.x = A.x + B.x;
    Result.y = A.y + B.y;
    Result.z = A.z + B.z;

    return Result;
}

v3 operator-(v3 A, v3 B)
{
    v3 Result;

    Result.x = A.x - B.x;
    Result.y = A.y - B.y;
    Result.z = A.z - B.z;

    return Result;
}

v3 operator*(r32 Value, v3 A)
{
    v3 Result;

    Result.x = Value * A.x;
    Result.y = Value * A.y;
    Result.z = Value * A.z;

    return Result;
}

v3 &operator+=(v3 &A, v3 B)
{
    A.x += B.x;
    A.y += B.y;
    A.z += B.z;

    return A;
}

v3 &operator-=(v3 &A, v3 B)
{
    A.x -= B.x;
    A.y -= B.y;
    A.z -= B.z;

    return A;
}

v3 operator-(v3 &A)
{
    v3 Result = {};

    Result.x = -A.x;
    Result.y = -A.y;
    Result.z = -A.z;

    return Result;
}

v3 &operator*=(v3 &A, r32 Value)
{
    A.x *= Value;
    A.y *= Value;
    A.z *= Value;

    return A;
}

v3 Hadamard(v3 A, v3 B)
{
    v3 Result;

    Result.x = A.x * B.x;
    Result.y = A.y * B.y;
    Result.z = A.z * B.z;

    return Result;
}

r32 Dot(v3 A, v3 B)
{
    r32 Result = A.x*B.x + A.y*B.y + A.z*B.z;
    return Result;
}

r32 LengthSquare(v3 A)
{
    return A.x * A.x + A.y * A.y + A.z*A.z;
}

r32 Length(v3 A)
{
    return sqrt(LengthSquare(A));
}

v3 UnitLength(v3 A)
{
    v3 Result = A;
    Result *= 1.0f/Length(A);

    return Result;
}

struct v3u
{
    union
    {
        struct 
        {
            u32 x, y, z;
        };

        u32 E[3];
    };
};


#endif
