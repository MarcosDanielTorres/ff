#pragma once
#include "math.h"

union Vec2
{
    struct
    {
        f32 x, y;
    };
    f32 c[2];
};

Vec2 operator+(Vec2 a, Vec2 b)
{
    Vec2 result = {a.x + b.x, a.y + b.y};
    return result;
}

Vec2 &operator+=(Vec2& a, Vec2 b)
{
    a = a + b;
    return a;
}

Vec2 operator-(Vec2 a, Vec2 b)
{
    Vec2 result = {a.x - b.x, a.y - b.y};
    return result;
}

Vec2 operator-(Vec2 a)
{
    Vec2 result;
    result.x = -a.x;
    result.y = -a.y;
    return result;
}

Vec2 &operator-=(Vec2& a, Vec2 b)
{
    a = a - b;
    return a;
}

Vec2 operator*(Vec2 a, f32 s)
{
    Vec2 result = {a.x * s, a.y * s};
    return result;
}

Vec2 operator*(f32 s, Vec2 a)
{
    Vec2 result = a * s;
    return result;
}

Vec2 &operator*=(Vec2& a, f32 s)
{
    a = a * s;
    return a;
}

Vec2 &operator*=(f32 s, Vec2& a)
{
    a = a * s;
    return a;
}

f32 dot(Vec2 a, Vec2 b)
{
    f32 result = a.x * b.x + a.y + b.y;
    return result;
}

f32 len(Vec2 a)
{
    f32 result = {0};
    f32 len_sq = dot(a, a);
    if (len_sq < 0.000001f)
    {
        result = 0.0f;
    }
    else
    {
        result = sqrtf(len_sq);
    }
    return result;
}


Vec2 norm(Vec2 &a)
{
    Vec2 result = {0};
    f32 v_len = len(a);
    if (v_len != 0.0f){
        result = a * (1 / v_len);
    }
    return result;
}

Vec2 lerp(Vec2 a, Vec2 b, f32 t)
{
    Vec2 result = a + (b - a) * t;
    return result;
}

union Vec3
{
    struct
    {
        f32 x, y, z;
    };
    f32 c[3];
};

//Vec3 cross(Vec3 a, Vec3 b)
//{
//    Vec3 result;
//    result.x = a.y * b.z - a.z * b.y;
//    result.y = a.z * b.x - a.x * b.z;
//    result.z = a.x * b.y - a.y * b.x;
//    return result;
//}


union Vec4
{
    struct
    {
        f32 x, y, z, w;
    };
    f32 c[4];
};

struct Mat4
{
    union
    {
        f32 m[4][4];
        f32 v[16];
        struct
        {
            f32 right[4];
            f32 up[4];
            f32 forward[4];
            f32 position[4];
        };

    };

    inline Mat4() : m 
    {
          {1.0f, 0.0f, 0.0f, 0.0f},   // column 0 = right
          {0.0f, 1.0f, 0.0f, 0.0f},   // column 1 = up
          {0.0f, 0.0f, 1.0f, 0.0f},   // column 2 = forward
          {0.0f, 0.0f, 0.0f, 1.0f}    // column 3 = position
    } {}

    inline Mat4(
        f32 m00, f32 m01, f32 m02, f32 m03,
        f32 m10, f32 m11, f32 m12, f32 m13,
        f32 m20, f32 m21, f32 m22, f32 m23,
        f32 m30, f32 m31, f32 m32, f32 m33
    ) : m
    {
        { m00, m10, m20, m30 },  // col 0
        { m01, m11, m21, m31 },  // col 1
        { m02, m12, m22, m32 },  // col 2
        { m03, m13, m23, m33 }   // col 3
    } {}

    inline Mat4(f32 *c) : m{
        {c[0], c[1], c[2], c[3]},
        {c[4], c[5], c[6], c[7]},
        {c[8], c[9], c[10], c[11]},
        {c[12], c[13], c[14], c[15]}
    } {}
};