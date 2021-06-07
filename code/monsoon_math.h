#ifndef MONSOON_MATH_H
#define MONSOON_MATH_H

struct v2
{
    union
    {
        struct 
        {
            r32 X, Y;
        };

        r32 E[2];
    };
};

v2 V2(r32 X, r32 Y)
{
    v2 Result;

    Result.X = X;
    Result.Y = Y;

    return Result;
}

v2 operator+(v2 A, v2 B)
{
    v2 Result;

    Result.X = A.X + B.X;
    Result.Y = A.Y + B.Y;

    return Result;
}

v2 operator-(v2 A, v2 B)
{
    v2 Result;

    Result.X = A.X - B.X;
    Result.Y = A.Y - B.Y;

    return Result;
}

v2 operator*(r32 Value, v2 A)
{
    v2 Result;

    Result.X = Value * A.X;
    Result.Y = Value * A.Y;

    return Result;
}

v2 &operator+=(v2 &A, v2 B)
{
    A.X += B.X;
    A.Y += B.Y;

    return A;
}

v2 &operator-=(v2 &A, v2 B)
{
    A.X -= B.X;
    A.Y -= B.Y;

    return A;
}

v2 operator-(v2 &A)
{
    v2 Result = {};

    Result.X = -A.X;
    Result.Y = -A.Y;

    return Result;
}

v2 &operator*=(v2 &A, r32 Value)
{
    A.X *= Value;
    A.Y *= Value;

    return A;
}

v2 Hadamard(v2 A, v2 B)
{
    v2 Result;

    Result.X = A.X * B.X;
    Result.Y = A.Y * B.Y;

    return Result;
}

r32 Square(r32 Value)
{
    return Value*Value;
}

r32 Dot(v2 A, v2 B)
{
    r32 Result = A.X*B.X + A.Y*B.Y;
    return Result;
}

r32 LengthSquare(v2 A)
{
    return A.X * A.X + A.Y * A.Y;
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
            u32 X, Y;
        };

        u32 E[2];
    };
};

v2u V2u(u32 X, u32 Y)
{
    v2u Result;

    Result.X = X;
    Result.Y = Y;

    return Result;
}

v2u operator+(v2u A, v2u B)
{
    v2u Result;

    Result.X = A.X + B.X;
    Result.Y = A.Y + B.Y;

    return Result;
}

v2u operator-(v2u A, v2u B)
{
    v2u Result;

    Result.X = A.X - B.X;
    Result.Y = A.Y - B.Y;

    return Result;
}

v2u operator*(u32 Value, v2u A)
{
    v2u Result;

    Result.X = Value * A.X;
    Result.Y = Value * A.Y;

    return Result;
}

v2u &operator+=(v2u &A, v2u B)
{
    A.X += B.X;
    A.Y += B.Y;

    return A;
}

v2u &operator-=(v2u &A, v2u B)
{
    A.X -= B.X;
    A.Y -= B.Y;

    return A;
}

v2u operator-(v2u &A)
{
    v2u Result = {};

    Result.X = -A.X;
    Result.Y = -A.Y;

    return Result;
}

v2u &operator*=(v2u &A, u32 Value)
{
    A.X *= Value;
    A.Y *= Value;

    return A;
}

struct v2i
{
    union
    {
        struct 
        {
            i32 X, Y;
        };

        i32 E[2];
    };
};

v2i V2i(i32 X, i32 Y)
{
    v2i Result;

    Result.X = X;
    Result.Y = Y;

    return Result;
}

v2i operator+(v2i A, v2i B)
{
    v2i Result;

    Result.X = A.X + B.X;
    Result.Y = A.Y + B.Y;

    return Result;
}

v2i operator-(v2i A, v2i B)
{
    v2i Result;

    Result.X = A.X - B.X;
    Result.Y = A.Y - B.Y;

    return Result;
}

v2i operator*(i32 Value, v2i A)
{
    v2i Result;

    Result.X = Value * A.X;
    Result.Y = Value * A.Y;

    return Result;
}

v2i &operator+=(v2i &A, v2i B)
{
    A.X += B.X;
    A.Y += B.Y;

    return A;
}

v2i &operator-=(v2i &A, v2i B)
{
    A.X -= B.X;
    A.Y -= B.Y;

    return A;
}

v2i operator-(v2i &A)
{
    v2i Result = {};

    Result.X = -A.X;
    Result.Y = -A.Y;

    return Result;
}

v2i &operator*=(v2i &A, i32 Value)
{
    A.X *= Value;
    A.Y *= Value;

    return A;
}

struct v3u
{
    union
    {
        struct 
        {
            u32 X, Y, Z;
        };

        u32 E[3];
    };
};

#endif
