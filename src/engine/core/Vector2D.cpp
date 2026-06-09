#include "Vector2D.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Aeon {

Vector2D::Vector2D() : x(0.0f), y(0.0f) {}

Vector2D::Vector2D(float x, float y) : x(x), y(y) {}

Vector2D Vector2D::Add(const Vector2D &vec) const { return Vector2D(this->x + vec.x, this->y + vec.y); }

Vector2D Vector2D::Subtract(const Vector2D &vec) const { return Vector2D(this->x - vec.x, this->y - vec.y); }

Vector2D Vector2D::Multiply(const Vector2D &vec) const { return Vector2D(this->x * vec.x, this->y * vec.y); }

Vector2D Vector2D::Divide(const Vector2D &vec) const { return Vector2D(this->x / vec.x, this->y / vec.y); }

Vector2D Vector2D::Add(const float f) const { return Vector2D(this->x + f, this->y + f); }

Vector2D Vector2D::Subtract(const float f) const { return Vector2D(this->x - f, this->y - f); }

Vector2D Vector2D::Multiply(const float f) const { return Vector2D(this->x * f, this->y * f); }

Vector2D Vector2D::Divide(const float f) const { return Vector2D(this->x / f, this->y / f); }

Vector2D operator+(const Vector2D &v1, const Vector2D &v2) { return v1.Add(v2); }

Vector2D operator-(const Vector2D &v1, const Vector2D &v2) { return v1.Subtract(v2); }

Vector2D operator*(const Vector2D &v1, const Vector2D &v2) { return v1.Multiply(v2); }

Vector2D operator/(const Vector2D &v1, const Vector2D &v2) { return v1.Divide(v2); }

Vector2D &Vector2D::operator+=(const Vector2D &vec) {
    this->x += vec.x;
    this->y += vec.y;
    return *this;
}

Vector2D &Vector2D::operator-=(const Vector2D &vec) {
    this->x -= vec.x;
    this->y -= vec.y;
    return *this;
}

Vector2D &Vector2D::operator*=(const Vector2D &vec) {
    this->x *= vec.x;
    this->y *= vec.y;
    return *this;
}

Vector2D &Vector2D::operator/=(const Vector2D &vec) {
    this->x /= vec.x;
    this->y /= vec.y;
    return *this;
}

Vector2D &Vector2D::Zero() {
    this->x = 0;
    this->y = 0;
    return *this;
}

Vector2D Vector2D::operator+(const float f) const { return this->Add(f); }

Vector2D Vector2D::operator-(const float f) const { return this->Subtract(f); }

Vector2D Vector2D::operator*(const float f) const { return this->Multiply(f); }

Vector2D Vector2D::operator/(const float f) const { return this->Divide(f); }

Vector2D &Vector2D::operator+=(const float f) {
    x += f;
    y += f;
    return *this;
}
Vector2D &Vector2D::operator-=(const float f) {
    x -= f;
    y -= f;
    return *this;
}
Vector2D &Vector2D::operator*=(const float f) {
    x *= f;
    y *= f;
    return *this;
}
Vector2D &Vector2D::operator/=(const float f) {
    x /= f;
    y /= f;
    return *this;
}

Vector2D Vector2D::operator*(const int i) const {
    return Vector2D(this->x * static_cast<float>(i), this->y * static_cast<float>(i));
}

Vector2D Vector2D::operator-() const { return Vector2D(-x, -y); }

float Vector2D::Length() const { return std::sqrt(x * x + y * y); }

float Vector2D::LengthSqr() const { return x * x + y * y; }

Vector2D Vector2D::Lerp(const Vector2D &vec1, const Vector2D &vec2, float amount) {
    amount = std::clamp(amount, 0.0f, 1.0f);
    return Vector2D(vec1.x + (vec2.x - vec1.x) * amount, vec1.y + (vec2.y - vec1.y) * amount);
}

bool Vector2D::IsZero() const {
    return LengthSqr() < (std::numeric_limits<float>::epsilon() * std::numeric_limits<float>::epsilon());
}

} // namespace Aeon
