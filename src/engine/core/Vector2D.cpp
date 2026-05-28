#include "Vector2D.h"

#include <cmath>

Vector2D::Vector2D() {
    x = 0.0f;
    y = 0.0f;
}

Vector2D::Vector2D(float x, float y) {
    this->x = x;
    this->y = y;
}

Vector2D &Vector2D::Add(const Vector2D &vec) {
    this->x += vec.x;
    this->y += vec.y;

    return *this;
}

Vector2D &Vector2D::Subtract(const Vector2D &vec) {
    this->x -= vec.x;
    this->y -= vec.y;

    return *this;
}

Vector2D &Vector2D::Multiply(const Vector2D &vec) {
    this->x *= vec.x;
    this->y *= vec.y;

    return *this;
}

Vector2D &Vector2D::Divide(const Vector2D &vec) {
    this->x /= vec.x;
    this->y /= vec.y;

    return *this;
}

Vector2D &Vector2D::Add(const float f) {
    this->x += f;
    this->y += f;

    return *this;
}
Vector2D &Vector2D::Subtract(const float f) {
    this->x -= f;
    this->y -= f;

    return *this;
}

Vector2D &Vector2D::Multiply(const float f) {
    this->x *= f;
    this->y *= f;

    return *this;
}

Vector2D &Vector2D::Divide(const float f) {
    this->x /= f;
    this->y /= f;

    return *this;
}

Vector2D &operator+(Vector2D &v1, const Vector2D &v2) { return v1.Add(v2); }

Vector2D &operator-(Vector2D &v1, const Vector2D &v2) { return v1.Subtract(v2); }

Vector2D &operator*(Vector2D &v1, const Vector2D &v2) { return v1.Multiply(v2); }

Vector2D &operator/(Vector2D &v1, const Vector2D &v2) { return v1.Divide(v2); }

Vector2D &Vector2D::operator+=(const Vector2D &vec) { return this->Add(vec); }

Vector2D &Vector2D::operator-=(const Vector2D &vec) { return this->Subtract(vec); }

Vector2D &Vector2D::operator*=(const Vector2D &vec) { return this->Multiply(vec); }

Vector2D &Vector2D::operator/=(const Vector2D &vec) { return this->Divide(vec); }

Vector2D &Vector2D::operator+(const float &f) { return this->Add(f); }

Vector2D &Vector2D::operator-(const float &f) { return this->Subtract(f); }

Vector2D &Vector2D::operator*(const float &f) { return this->Multiply(f); }

Vector2D &Vector2D::operator/(const float &f) { return this->Divide(f); }

Vector2D &Vector2D::operator*(const int &i) {
    this->x *= i;
    this->y *= i;

    return *this;
}

Vector2D &Vector2D::Zero() {
    this->x = 0;
    this->y = 0;

    return *this;
}

float Vector2D::Length() const { return (float) std::sqrt(x * x + y * y); }

float Vector2D::LengthSqr() const { return x * x + y * y; }

Vector2D Vector2D::Lerp(const Vector2D &vec1, const Vector2D &vec2, float amount) {
    Vector2D temp;

    if (amount > 1)
        amount = 1;
    if (amount < 0)
        amount = 0;

    temp.x = vec1.x + (vec2.x - vec1.x) * amount;
    temp.y = vec1.y + (vec2.y - vec1.y) * amount;

    return temp;
}

bool Vector2D::IsZero() const {
	return Length() < std::numeric_limits<float>::epsilon();
}
