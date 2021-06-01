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

#endif
