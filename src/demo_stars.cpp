// physengine: демо «двойная звезда» (блок D1) — визуализируемый прогон.
// Два тела на круговых орбитах вокруг общего центра масс; гравитация F = G·M·m/r².
// Результат: PNG-кадры (output/frames), CSV (output/binary_stars.csv),
// MP4 (собирается ffmpeg'ом по цели Make demo_video), контроль энергии в stdout.
#include "phys/world.hpp"
#include "phys/integrator_verlet.hpp"
#include "phys/render.hpp"
#include "phys/recorder.hpp"
#include <cmath>
#include <cstdio>
#include <sys/stat.h>
#include <string>
#include <vector>

namespace demo_stars {

using namespace phys;

struct DemoConfig {
    double m1 = 2.0;        // массы звёзд, кг (условные единицы масштаба)
    double m2 = 1.0;
    double separation = 4.0; // расстояние между звёздами, м
    double duration = 30.0;  // чуть больше полного периода (T = 29.0 c)
    double dt = 1.0 / 480.0;
    int everyNth = 8;        // кадр каждые N шагов → 60 к/с
    int width = 1280, height = 720;
    std::string outDir = "output";
};

// Гравитационная постоянная в условных единицах (масштаб задачи).
inline constexpr double G = 1.0;

// Потенциальная энергия пары (для записи в CSV).
inline double pairPotential(const std::vector<Body>& bodies) {
    if (bodies.size() < 2) return 0.0;
    Vec2 r = bodies[1].pos - bodies[0].pos;
    return -G * bodies[0].mass * bodies[1].mass / r.len();
}

inline int run(const DemoConfig& cfg = {}) {
    // Каталог вывода создаём сами (mkdir -p для одного уровня).
    ::mkdir(cfg.outDir.c_str(), 0755);

    World world;

    // Круговые орбиты: центр масс в начале координат.
    // r1 = a·m2/(m1+m2), r2 = a·m1/(m1+m2); v = sqrt(G·m_other / a_pair)... для
    // относительного движения omega² = G(m1+m2)/a³, v_i = omega·r_i.
    const double mSum = cfg.m1 + cfg.m2;
    const double r1 = cfg.separation * cfg.m2 / mSum;
    const double r2 = cfg.separation * cfg.m1 / mSum;
    const double omega = std::sqrt(G * mSum / (cfg.separation * cfg.separation * cfg.separation));

    Body a, b;
    a.setMass(cfg.m1);
    a.pos = {-r1, 0.0};
    a.vel = {0.0, -omega * r1};
    a.radius = 0.28;
    b.setMass(cfg.m2);
    b.pos = {r2, 0.0};
    b.vel = {0.0, omega * r2};
    b.radius = 0.20;
    world.addBody(a);
    world.addBody(b);

    auto gravity = [](std::vector<Body>& bodies) {
        if (bodies.size() < 2) return;
        Vec2 r = bodies[1].pos - bodies[0].pos;
        double d = r.len();
        if (d == 0.0) return;
        Vec2 f = r.normalized() * (G * bodies[0].mass * bodies[1].mass / (d * d));
        bodies[0].applyForce(f);
        bodies[1].applyForce(-f);
    };
    world.setForces(gravity);
    world.setIntegrator(std::make_unique<VelocityVerlet>());

    Recorder rec(cfg.outDir + "/binary_stars.csv", 0.02);
    rec.begin(world.bodies());

    // Следы (в мировых координатах, проецируются каждый кадр).
    std::vector<Vec2> trailA{a.pos}, trailB{b.pos};

    const render::Color bg{12, 16, 28};
    const render::Color gridC{34, 42, 66};
    const render::Color starA{255, 214, 120};
    const render::Color haloA{255, 150, 40};
    const render::Color starB{140, 200, 255};
    const render::Color haloB{60, 120, 255};

    render::Camera cam;
    cam.center = {0.0, 0.0};
    cam.halfW = 3.4; // звёзды на расстоянии 4 м: орбиты радиусом 1.3 и 2.7 м
    cam.pxW = cfg.width;
    cam.pxH = cfg.height;

    int frame = 0;
    int framesTotal = 0;
    double E0 = 0.0;
    double maxDrift = 0.0;

    auto totalEnergy = [&](const std::vector<Body>& bs) {
        double e = 0.0;
        for (const auto& body : bs) e += 0.5 * body.mass * body.vel.len2();
        return e + pairPotential(bs);
    };

    for (double t = 0.0; t <= cfg.duration + 1e-9; t += cfg.dt) {
        world.step(cfg.dt);
        // На тело относим половину потенциальной энергии пары: сумма по телам
        // даёт корректную полную энергию системы.
        rec.sample(t, world.bodies(), [&world](const Body&) {
            return 0.5 * pairPotential(world.bodies());
        });
        trailA.push_back(world.bodies()[0].pos);
        trailB.push_back(world.bodies()[1].pos);

        double E = totalEnergy(world.bodies());
        if (t == 0.0 || E0 == 0.0) E0 = E;
        maxDrift = std::max(maxDrift, std::abs(E - E0) / std::abs(E0));

        if (frame % cfg.everyNth == 0) {
            render::Canvas canvas(cfg.width, cfg.height);
            canvas.fill(bg);
            canvas.drawGrid(64, gridC);
            canvas.drawTrail(render::project(trailA, cam), render::Color{120, 90, 40}, 1.5, true);
            canvas.drawTrail(render::project(trailB, cam), render::Color{50, 80, 140}, 1.5, true);
            const auto& bA = world.bodies()[0];
            const auto& bB = world.bodies()[1];
            Vec2 pa = cam.toPixels(bA.pos);
            Vec2 pb = cam.toPixels(bB.pos);
            canvas.drawGlow(pa.x, pa.y, std::max(3.0, bA.radius * cam.scale()), starA, haloA);
            canvas.drawGlow(pb.x, pb.y, std::max(2.5, bB.radius * cam.scale()), starB, haloB);
            char path[256];
            std::snprintf(path, sizeof(path), "%s/frame_%05d.png",
                          cfg.outDir.c_str(), framesTotal);
            canvas.savePng(path);
            ++framesTotal;
        }
        ++frame;
    }
    rec.end();

    std::printf("demo binary_stars: кадров=%d, CSV=1, макс. дрейф энергии=%.2e (Verlet)\n",
                framesTotal, maxDrift);
    return framesTotal > 0 && maxDrift < 1e-3 ? 0 : 1;
}

int runWithOutdir(const std::string& outDir) {
    DemoConfig cfg;
    cfg.outDir = outDir;
    return run(cfg);
}

} // namespace demo_stars
