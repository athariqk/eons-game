#include "Vec2D.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace ncore {

Vec2D::Vec2D() : x(0.0f), y(0.0f) {}

Vec2D::Vec2D(float x, float y) : x(x), y(y) {}

Vec2D Vec2D::add(const Vec2D &vec) const { return Vec2D(this->x + vec.x, this->y + vec.y); }

Vec2D Vec2D::subtract(const Vec2D &vec) const { return Vec2D(this->x - vec.x, this->y - vec.y); }

Vec2D Vec2D::multiply(const Vec2D &vec) const { return Vec2D(this->x * vec.x, this->y * vec.y); }

Vec2D Vec2D::divide(const Vec2D &vec) const { return Vec2D(this->x / vec.x, this->y / vec.y); }

Vec2D Vec2D::add(const float f) const { return Vec2D(this->x + f, this->y + f); }

Vec2D Vec2D::subtract(const float f) const { return Vec2D(this->x - f, this->y - f); }

Vec2D Vec2D::multiply(const float f) const { return Vec2D(this->x * f, this->y * f); }

Vec2D Vec2D::divide(const float f) const { return Vec2D(this->x / f, this->y / f); }

Vec2D operator+(const Vec2D &v1, const Vec2D &v2) { return v1.add(v2); }

Vec2D operator-(const Vec2D &v1, const Vec2D &v2) { return v1.subtract(v2); }

Vec2D operator*(const Vec2D &v1, const Vec2D &v2) { return v1.multiply(v2); }

Vec2D operator/(const Vec2D &v1, const Vec2D &v2) { return v1.divide(v2); }

Vec2D &Vec2D::operator+=(const Vec2D &vec) {
    this->x += vec.x;
    this->y += vec.y;
    return *this;
}

Vec2D &Vec2D::operator-=(const Vec2D &vec) {
    this->x -= vec.x;
    this->y -= vec.y;
    return *this;
}

Vec2D &Vec2D::operator*=(const Vec2D &vec) {
    this->x *= vec.x;
    this->y *= vec.y;
    return *this;
}

Vec2D &Vec2D::operator/=(const Vec2D &vec) {
    this->x /= vec.x;
    this->y /= vec.y;
    return *this;
}

Vec2D &Vec2D::zero() {
    this->x = 0;
    this->y = 0;
    return *this;
}

Vec2D Vec2D::operator+(const float f) const { return this->add(f); }

Vec2D Vec2D::operator-(const float f) const { return this->subtract(f); }

Vec2D Vec2D::operator*(const float f) const { return this->multiply(f); }

Vec2D Vec2D::operator/(const float f) const { return this->divide(f); }

Vec2D &Vec2D::operator+=(const float f) {
    x += f;
    y += f;
    return *this;
}
Vec2D &Vec2D::operator-=(const float f) {
    x -= f;
    y -= f;
    return *this;
}
Vec2D &Vec2D::operator*=(const float f) {
    x *= f;
    y *= f;
    return *this;
}
Vec2D &Vec2D::operator/=(const float f) {
    x /= f;
    y /= f;
    return *this;
}

Vec2D Vec2D::operator*(const int i) const {
    return Vec2D(this->x * static_cast<float>(i), this->y * static_cast<float>(i));
}

Vec2D Vec2D::operator-() const { return Vec2D(-x, -y); }

float Vec2D::length() const { return std::sqrt(x * x + y * y); }

float Vec2D::length_sqr() const { return x * x + y * y; }

Vec2D Vec2D::lerp(const Vec2D &vec1, const Vec2D &vec2, float amount) {
    amount = std::clamp(amount, 0.0f, 1.0f);
    return Vec2D(vec1.x + (vec2.x - vec1.x) * amount, vec1.y + (vec2.y - vec1.y) * amount);
}

bool Vec2D::is_zero() const {
    return length_sqr() < (std::numeric_limits<float>::epsilon() * std::numeric_limits<float>::epsilon());
}

} // namespace ncore
