// physengine: минимальная векторная математика ядра (2D).
// Полная версия (Vec3, матрицы) появится в блоке A — см. docs/DESIGN.md.
#pragma once
#include <cmath>

namespace phys {

struct Vec2 {
    double x = 0.0;
    double y = 0.0;

    constexpr Vec2() = default;
    constexpr Vec2(double px, double py) : x(px), y(py) {}

    constexpr Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    constexpr Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    constexpr Vec2 operator-() const { return {-x, -y}; }
    constexpr Vec2 operator*(double k) const { return {x * k, y * k}; }
    constexpr Vec2 operator/(double k) const { return {x / k, y / k}; }
    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }

    double dot(const Vec2& o) const { return x * o.x + y * o.y; }
    double cross(const Vec2& o) const { return x * o.y - y * o.x; }
    double len2() const { return x * x + y * y; }
    double len() const { return std::sqrt(len2()); }
    Vec2 normalized() const {
        double l = len();
        return l > 0.0 ? *this / l : Vec2{};
    }
};

inline constexpr Vec2 operator*(double k, const Vec2& v) { return v * k; }

} // namespace phys
