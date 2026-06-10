#pragma once

#include <format>

namespace ncore {

/**
 * @brief A two-dimensional floating-point vector
 */
struct Vec2D {
    float x, y;

    Vec2D();
    Vec2D(float x, float y);

    Vec2D add(const Vec2D &vec) const;
    Vec2D subtract(const Vec2D &vec) const;
    Vec2D multiply(const Vec2D &vec) const;
    Vec2D divide(const Vec2D &vec) const;

    Vec2D add(const float f) const;
    Vec2D subtract(const float f) const;
    Vec2D multiply(const float f) const;
    Vec2D divide(const float f) const;

    friend Vec2D operator+(const Vec2D &v1, const Vec2D &v2);
    friend Vec2D operator-(const Vec2D &v1, const Vec2D &v2);
    friend Vec2D operator*(const Vec2D &v1, const Vec2D &v2);
    friend Vec2D operator/(const Vec2D &v1, const Vec2D &v2);

    Vec2D &operator+=(const Vec2D &vec);
    Vec2D &operator-=(const Vec2D &vec);
    Vec2D &operator*=(const Vec2D &vec);
    Vec2D &operator/=(const Vec2D &vec);
    Vec2D &zero();

    Vec2D operator+(const float f) const;
    Vec2D operator-(const float f) const;
    Vec2D operator*(const float f) const;
    Vec2D operator/(const float f) const;

    Vec2D &operator+=(const float f);
    Vec2D &operator-=(const float f);
    Vec2D &operator*=(const float f);
    Vec2D &operator/=(const float f);

    Vec2D operator*(const int i) const;

    Vec2D operator-() const;

    float length() const;
    float length_sqr() const;

    static Vec2D lerp(const Vec2D &vec1, const Vec2D &vec2, float amount);

    bool is_zero() const;

    std::string to_string() const { return std::format("Vec2D<x={},y={}>", x, y); }
};

} // namespace ncore
