// physengine: библиотека сил. Каждая сила — функция (std::vector<Body>&) -> void,
// складывается в общий лямбда-конвейер World::setForces().
// Гравитация и пружина нужны уже в блоке A; потенциалы (Леннард-Джонс,
// Кулон) добавятся в блоках E и далее.
#pragma once
#include "phys/body.hpp"
#include <vector>

namespace phys::forces {

/// Однородное поле тяжести: F = m·g.
inline void gravity(std::vector<Body>& bodies, double gx, double gy) {
    for (auto& b : bodies) b.applyForce({gx * b.mass, gy * b.mass});
}

/// Пружина между телами i и j: F = -k·(|r|-L)·r̂ + демпфирование c·v_rel.
inline void spring(std::vector<Body>& bodies, int i, int j,
                   double k, double length, double damping) {
    Body& a = bodies[i];
    Body& b = bodies[j];
    Vec2 r = b.pos - a.pos;
    double dist = r.len();
    if (dist == 0.0) return;
    Vec2 dir = r / dist;
    Vec2 vRel = b.vel - a.vel;
    Vec2 f = dir * ((dist - length) * k + vRel.dot(dir) * damping);
    a.applyForce(f);
    b.applyForce(-f);
}

/// Линейное сопротивление среды: F = -k1·v (закон Стокса).
inline void dragLinear(std::vector<Body>& bodies, double k1) {
    for (auto& b : bodies) b.applyForce(-b.vel * k1);
}

/// Квадратичное сопротивление: F = -k2·|v|·v.
inline void dragQuadratic(std::vector<Body>& bodies, double k2) {
    for (auto& b : bodies) {
        double v = b.vel.len();
        if (v > 0.0) b.applyForce(b.vel * (-k2 * v));
    }
}

} // namespace phys::forces
