// physengine: CLI. Каркас + визуализируемое демо «двойная звезда».
// Полный парсер сцен появляется в блоке A (см. docs/DESIGN.md).
#include "phys/world.hpp"
#include "phys/integrator_verlet.hpp"
#include <cstdio>
#include <memory>
#include <string>

namespace demo_stars {
struct DemoConfig;                 // определён в src/demo_stars.cpp
int run(const DemoConfig& cfg);    // полная конфигурация
int runWithOutdir(const std::string& outDir); // демо с заданным каталогом вывода
}

int main(int argc, char** argv) {
    if (argc > 1) {
        const std::string mode = argv[1];
        if (mode == "demo") {
            // physengine demo [--outdir DIR]
            std::string outDir = "output";
            for (int i = 2; i < argc - 1; ++i)
                if (std::string(argv[i]) == "--outdir") outDir = argv[i + 1];
            return demo_stars::runWithOutdir(outDir);
        }
        std::printf("physengine: неизвестный режим '%s' (поддерживается: demo)\n", argv[1]);
        return 2;
    }

    // Smoke: свободное падение 1 с, сравнение с аналитикой.
    phys::World world;
    phys::Body b;
    b.pos = {0.0, 1.0};
    b.vel = {0.5, 0.0};
    b.setMass(2.0);
    world.addBody(b);
    world.setForces([](std::vector<phys::Body>& bodies) {
        for (auto& body : bodies)
            body.applyForce(phys::Vec2{0.0, -9.81 * body.mass});
    });
    world.setIntegrator(std::make_unique<phys::VelocityVerlet>());

    const double dt = 1.0 / 240.0;
    for (int i = 0; i < 240; ++i) world.step(dt); // 1 секунда
    const auto& after = world.bodies()[0];
    // Свободное падение с высоты 1 м: y(1с) = 1 - g/2 ≈ -3.905.
    std::printf("physengine smoke: y(1s) = %.3f (аналитика -3.905), x(1s) = %.3f\n",
                after.pos.y, after.pos.x);
    return 0;
}
