#pragma once

#include <format>

namespace Aeon {

struct Vector2D {
    float x, y;

    Vector2D();
    Vector2D(float x, float y);

    Vector2D Add(const Vector2D &vec) const;
    Vector2D Subtract(const Vector2D &vec) const;
    Vector2D Multiply(const Vector2D &vec) const;
    Vector2D Divide(const Vector2D &vec) const;

    Vector2D Add(const float f) const;
    Vector2D Subtract(const float f) const;
    Vector2D Multiply(const float f) const;
    Vector2D Divide(const float f) const;

    friend Vector2D operator+(const Vector2D &v1, const Vector2D &v2);
    friend Vector2D operator-(const Vector2D &v1, const Vector2D &v2);
    friend Vector2D operator*(const Vector2D &v1, const Vector2D &v2);
    friend Vector2D operator/(const Vector2D &v1, const Vector2D &v2);

    Vector2D &operator+=(const Vector2D &vec);
    Vector2D &operator-=(const Vector2D &vec);
    Vector2D &operator*=(const Vector2D &vec);
    Vector2D &operator/=(const Vector2D &vec);

    Vector2D operator+(const float f) const;
    Vector2D operator-(const float f) const;
    Vector2D operator*(const float f) const;
    Vector2D operator/(const float f) const;

    Vector2D operator*(const int i) const;
    Vector2D Zero();

    float Length() const;
    float LengthSqr() const;

    static Vector2D Lerp(const Vector2D &vec1, const Vector2D &vec2, float amount);

    bool IsZero() const;

    std::string ToString() const { return std::format("Vector2D<x={},y={}>", x, y); }
};

} // namespace Aeon
