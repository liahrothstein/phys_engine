// physengine: точечное тело — базовая сущность ядра.
// Масса, состояние (позиция/скорость) и аккумулятор силы.
#pragma once
#include "phys/math_vec2.hpp"

namespace phys {

struct Body {
    Vec2 pos;        // позиция, м
    Vec2 vel;        // скорость, м/с
    Vec2 force;      // аккумулятор силы, Н (обнуляется каждый шаг)
    double mass = 1.0;   // кг
    double invMass = 1.0; // 1/m (для скорости и неподвижных тел = 0)
    double radius = 0.0; // радиус для столкновений и рендера, м
    int id = 0;          // стабильный идентификатор

    void setMass(double m) {
        mass = m;
        invMass = m > 0.0 ? 1.0 / m : 0.0;
    }
    void applyForce(const Vec2& f) { force += f; }
};

} // namespace phys
