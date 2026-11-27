// physengine: интегратор velocity Verlet (стандарт молекулярной динамики).
// x(t+dt) = x + v·dt + a·dt²/2; v(t+dt) = v + (a + a_new)·dt/2.
// Симплектический, O(dt²), обратим по времени; одно вычисление сил на шаг
// благодаря кэшированию ускорений.
#pragma once
#include "phys/integrator.hpp"
#include <vector>

namespace phys {

class VelocityVerlet : public Integrator {
public:
    const char* name() const override { return "verlet-velocity"; }

    void step(std::vector<Body>& bodies, const ForceFn& forces, double dt) override {
        if (bodies.empty()) return;

        if (acc_.size() != bodies.size()) {
            // Состав системы изменился (или первый шаг): пересчитать a(x(t)).
            acc_.assign(bodies.size(), Vec2{});
            forces(bodies);
            for (size_t i = 0; i < bodies.size(); ++i)
                acc_[i] = bodies[i].force * bodies[i].invMass;
            for (auto& b : bodies) b.force = Vec2{};
        }

        // Позиции и половина шага скорости
        for (size_t i = 0; i < bodies.size(); ++i) {
            Body& b = bodies[i];
            b.pos += b.vel * dt + acc_[i] * (0.5 * dt * dt);
            b.vel += acc_[i] * (0.5 * dt);
        }

        // Силы при новых позициях
        forces(bodies);

        // Вторая половина шага скорости
        for (size_t i = 0; i < bodies.size(); ++i) {
            Vec2 aNew = bodies[i].force * bodies[i].invMass;
            bodies[i].vel += aNew * (0.5 * dt);
            acc_[i] = aNew;
        }

        for (auto& b : bodies) b.force = Vec2{};
    }

private:
    std::vector<Vec2> acc_; // кэш ускорений a(x(t)) с прошлого шага
};

} // namespace phys
