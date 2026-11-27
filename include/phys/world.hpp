// physengine: мир — контейнер тел, сил и интегратора. Главный API ядра.
#pragma once
#include "phys/body.hpp"
#include "phys/integrator.hpp"
#include "phys/integrator_verlet.hpp"
#include <memory>
#include <vector>

namespace phys {

class World {
public:
    void addBody(Body b) {
        b.id = static_cast<int>(bodies_.size());
        bodies_.push_back(b);
    }
    Body& body(int i) { return bodies_[i]; }
    const std::vector<Body>& bodies() const { return bodies_; }
    size_t count() const { return bodies_.size(); }

    void setIntegrator(std::unique_ptr<Integrator> integ) { integrator_ = std::move(integ); }
    Integrator* integrator() { return integrator_.get(); }

    // Зарегистрированная функция сил вызывается интегратором на каждом шаге.
    void setForces(ForceFn fn) { forces_ = std::move(fn); }

    void step(double dt) {
        if (integrator_) integrator_->step(bodies_, forces_, dt);
    }

private:
    std::vector<Body> bodies_;
    std::unique_ptr<Integrator> integrator_;
    ForceFn forces_;
};

} // namespace phys
