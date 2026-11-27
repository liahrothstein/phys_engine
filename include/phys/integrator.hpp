// physengine: интерфейс интегратора (паттерн «стратегия»).
// Реализации: явный Эйлер, симплектический Эйлер, velocity Verlet, RK4.
#pragma once
#include "phys/body.hpp"
#include <vector>
#include <functional>

namespace phys {

// Вычисление ускорений всех тел: a(body) = force/mass после applyForces().
using ForceFn = std::function<void(std::vector<Body>&)>;

class Integrator {
public:
    virtual ~Integrator() = default;
    virtual const char* name() const = 0;

    // Продвинуть систему на dt. forces() обязан заполнить body.force.
    virtual void step(std::vector<Body>& bodies, const ForceFn& forces, double dt) = 0;
};

} // namespace phys
