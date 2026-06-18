#pragma once

#include <format>
#include <vector>

#include <ncore/kernel/types.h>

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

    NC_BIND(Vec2, NC_F(Vec2, x) NC_F(Vec2, y));
};

struct Vec4 : Vec2 {
    float w, h;

    Vec4() : Vec2(), w(0), h(0) {}
    Vec4(float x, float y, float w, float h) : Vec2(x, y), w(w), h(h) {}

    bool is_zero() const { return Vec2::is_zero() && w == 0 && h == 0; }

    NC_BIND(Vec4, NC_F(Vec4, w) NC_F(Vec4, h));
};

struct Color {
    Color() {}
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) : r(r), g(g), b(b), a(a) {}

    static Color unpack(uint32_t hex, uint8_t alpha = 200) {
        return {
            static_cast<uint8_t>((hex >> 16) & 0xFF),
            static_cast<uint8_t>((hex >> 8) & 0xFF),
            static_cast<uint8_t>(hex & 0xFF),
            alpha,
        };
    }

    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 0;

    NC_BIND(Color, NC_F(Color, r) NC_F(Color, g) NC_F(Color, b) NC_F(Color, a));
};

using BytesBuffer = std::vector<uint8_t>;

} // namespace ncore
