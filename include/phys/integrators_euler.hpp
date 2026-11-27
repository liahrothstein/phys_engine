// physengine: интеграторы Эйлера (header-only).
#include "phys/integrator.hpp"

namespace phys {

/// Явный Эйлер: x += v·dt; v += a(x)·dt. O(dt), энергия дрейфирует вверх.
class ExplicitEuler : public Integrator {
public:
    const char* name() const override { return "euler-explicit"; }
    void step(std::vector<Body>& bodies, const ForceFn& forces, double dt) override {
        forces(bodies);
        for (auto& b : bodies) {
            b.pos += b.vel * dt;
            b.vel += (b.force * b.invMass) * dt;
        }
        for (auto& b : bodies) b.force = Vec2{};
    }
};

/// Симплектический Эйлер: v += a(x)·dt; x += v_new·dt. O(dt), энергия ограничена.
class SymplecticEuler : public Integrator {
public:
    const char* name() const override { return "euler-symplectic"; }
    void step(std::vector<Body>& bodies, const ForceFn& forces, double dt) override {
        forces(bodies);
        for (auto& b : bodies) {
            b.vel += (b.force * b.invMass) * dt;
            b.pos += b.vel * dt;
        }
        for (auto& b : bodies) b.force = Vec2{};
    }
};

} // namespace phys
