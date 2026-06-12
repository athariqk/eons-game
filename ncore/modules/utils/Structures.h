#pragma once

#include <format>
#include <vector>

namespace ncore {

/**
 * @brief A two-dimensional floating-point vector
 */
struct Vec2 {
    float x, y;

    Vec2();
    Vec2(float x, float y);

    Vec2 add(const Vec2 &vec) const;
    Vec2 subtract(const Vec2 &vec) const;
    Vec2 multiply(const Vec2 &vec) const;
    Vec2 divide(const Vec2 &vec) const;

    Vec2 add(const float f) const;
    Vec2 subtract(const float f) const;
    Vec2 multiply(const float f) const;
    Vec2 divide(const float f) const;

    friend Vec2 operator+(const Vec2 &v1, const Vec2 &v2);
    friend Vec2 operator-(const Vec2 &v1, const Vec2 &v2);
    friend Vec2 operator*(const Vec2 &v1, const Vec2 &v2);
    friend Vec2 operator/(const Vec2 &v1, const Vec2 &v2);

    Vec2 &operator+=(const Vec2 &vec);
    Vec2 &operator-=(const Vec2 &vec);
    Vec2 &operator*=(const Vec2 &vec);
    Vec2 &operator/=(const Vec2 &vec);
    Vec2 &zero();

    Vec2 operator+(const float f) const;
    Vec2 operator-(const float f) const;
    Vec2 operator*(const float f) const;
    Vec2 operator/(const float f) const;

    Vec2 &operator+=(const float f);
    Vec2 &operator-=(const float f);
    Vec2 &operator*=(const float f);
    Vec2 &operator/=(const float f);

    Vec2 operator*(const int i) const;

    Vec2 operator-() const;

    float length() const;
    float length_sqr() const;

    static Vec2 lerp(const Vec2 &vec1, const Vec2 &vec2, float amount);

    bool is_zero() const;

    std::string to_string() const { return std::format("Vec2D<x={},y={}>", x, y); }
};

struct Vec4 : Vec2 {
    float w, h;

    Vec4() : Vec2(), w(0), h(0) {}
    Vec4(float x, float y, float w, float h) : Vec2(x, y), w(w), h(h) {}

    bool is_zero() const { return Vec2::is_zero() && w == 0 && h == 0; }
};

using BytesBuffer = std::vector<uint8_t>;

} // namespace ncore
