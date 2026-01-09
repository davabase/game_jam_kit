#pragma once

#include <box2d/box2d.h>
#include <raylib.h>

// Unary add one vector to another
inline void operator+=(Vector2& a, Vector2 b)
{
    a.x += b.x;
    a.y += b.y;
}

/// Unary subtract one vector from another
inline void operator-=(Vector2& a, Vector2 b)
{
    a.x -= b.x;
    a.y -= b.y;
}

/// Unary multiply a vector by a scalar
inline void operator*=(Vector2& a, float b)
{
    a.x *= b;
    a.y *= b;
}

/// Unary negate a vector
inline Vector2 operator-(Vector2 a)
{
    return {-a.x, -a.y};
}

/// Binary vector addition
inline Vector2 operator+(Vector2 a, Vector2 b)
{
    return {a.x + b.x, a.y + b.y};
}

/// Binary vector subtraction
inline Vector2 operator-(Vector2 a, Vector2 b)
{
    return {a.x - b.x, a.y - b.y};
}

/// Binary scalar and vector multiplication
inline Vector2 operator*(float a, Vector2 b)
{
    return {a * b.x, a * b.y};
}

/// Binary scalar and vector multiplication
inline Vector2 operator*(Vector2 a, float b)
{
    return {a.x * b, a.y * b};
}

/// Binary vector equality
inline bool operator==(Vector2 a, Vector2 b)
{
    return a.x == b.x && a.y == b.y;
}

/// Binary vector inequality
inline bool operator!=(Vector2 a, Vector2 b)
{
    return a.x != b.x || a.y != b.y;
}

/// Body equality.
inline bool operator==(b2BodyId a, b2BodyId b)
{
    return B2_ID_EQUALS(a, b);
}

/// Body less than.
inline bool operator<(b2BodyId a, b2BodyId b)
{
    return a.index1 < b.index1;
}
