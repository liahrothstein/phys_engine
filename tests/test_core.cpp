// Юнит-тесты ядра на doctest: математика, интеграторы на аналитических задачах.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "phys/math_vec2.hpp"
#include "phys/world.hpp"
#include "phys/integrators_euler.hpp"
#include "phys/integrator_verlet.hpp"
#include "phys/forces.hpp"
#include "phys/render.hpp"
#include <cmath>
#include <memory>

using phys::Vec2;
using phys::Body;
using phys::World;

TEST_CASE("Vec2: арифметика и скалярное произведение") {
    Vec2 a{3.0, 4.0};
    CHECK(a.len() == doctest::Approx(5.0));
    CHECK(a.dot({1.0, 1.0}) == doctest::Approx(7.0));
    CHECK(a.cross({1.0, 0.0}) == doctest::Approx(-4.0));
    CHECK((a * 2).x == doctest::Approx(6.0));
}

namespace {
// Гармонический осциллятор: x'' = -ω²x, ω = 1, x(0)=1, v(0)=0.
// Аналитика: x(t) = cos t. Проверяем точность интеграторов.
template <typename IntegratorT>
double oscillatorError(double dt, double tEnd) {
    World w;
    Body b;
    b.pos = {1.0, 0.0};
    w.addBody(b);
    w.setForces([](std::vector<Body>& bodies) {
        for (auto& body : bodies) body.applyForce(-body.pos);
    });
    w.setIntegrator(std::make_unique<IntegratorT>());

    for (double t = 0.0; t < tEnd; t += dt) w.step(dt);
    return std::abs(w.bodies()[0].pos.x - std::cos(tEnd));
}
} // namespace

TEST_CASE("Интеграторы: сходимость к cos(t) на осцилляторе") {
    const double dt = 1e-3, tEnd = 1.0;
    CHECK(oscillatorError<phys::ExplicitEuler>(dt, tEnd) < 5e-3);   // O(dt)
    CHECK(oscillatorError<phys::SymplecticEuler>(dt, tEnd) < 5e-3); // O(dt)
    CHECK(oscillatorError<phys::VelocityVerlet>(dt, tEnd) < 1e-5);  // O(dt²)
}

TEST_CASE("Verlet: сохранение энергии на осцилляторе") {
    World w;
    Body b;
    b.pos = {1.0, 0.0};
    w.addBody(b);
    w.setForces([](std::vector<Body>& bodies) {
        for (auto& body : bodies) body.applyForce(-body.pos);
    });
    phys::VelocityVerlet vv;
    w.setIntegrator(std::make_unique<phys::VelocityVerlet>(std::move(vv)));

    const double dt = 1e-3;
    for (int i = 0; i < 10000; ++i) w.step(dt); // 10 периодов

    const auto& body = w.bodies()[0];
    // E = ½x² + ½v²; при x(0)=1, v(0)=0 полная энергия E = 0.5.
    double energy = 0.5 * body.vel.len2() + 0.5 * body.pos.len2();
    CHECK(energy == doctest::Approx(0.5).epsilon(1e-3));
}

TEST_CASE("Рендер: камера, круги, сохранение PNG") {
    using namespace phys::render;
    Camera cam;
    cam.center = {0.0, 0.0};
    cam.halfW = 10.0;
    cam.pxW = 200;
    cam.pxH = 100;

    // Ось Y мира направлена вверх: точка выше центра — пиксель с меньшим y.
    Vec2 up = cam.toPixels({0.0, 5.0});
    Vec2 down = cam.toPixels({0.0, -5.0});
    CHECK(up.y < down.y);
    CHECK(up.x == doctest::Approx(100.0));

    Canvas canvas(200, 100);
    canvas.fill(Color{10, 10, 30});
    canvas.fillCircle(100.0, 50.0, 20.0, Color{255, 0, 0});
    // Центр закрашен, угол — фон.
    CHECK(canvas.pixel(100, 50).r == 255);
    CHECK(canvas.pixel(2, 2).b == 30);
    CHECK(canvas.pixel(2, 2).r == 10);
    // Сохранение PNG реально пишет файл.
    CHECK(canvas.savePng("build/test_render.png"));
}

TEST_CASE("Двойная звезда: кинематика круговых орбит") {
    // Аналитика блока D1: omega² = G(m1+m2)/a³, звёзды обращаются вокруг ЦМ.
    World w;
    const double m1 = 2.0, m2 = 1.0, a = 4.0, G = 1.0;
    const double mSum = m1 + m2;
    const double r1 = a * m2 / mSum, r2 = a * m1 / mSum;
    const double omega = std::sqrt(G * mSum / (a * a * a));

    Body A, B;
    A.setMass(m1); A.pos = {-r1, 0.0}; A.vel = {0.0, -omega * r1};
    B.setMass(m2); B.pos = { r2, 0.0}; B.vel = {0.0,  omega * r2};
    w.addBody(A); w.addBody(B);
    w.setForces([](std::vector<Body>& bodies) {
        Vec2 r = bodies[1].pos - bodies[0].pos;
        double d = r.len();
        Vec2 f = r.normalized() * (1.0 * bodies[0].mass * bodies[1].mass / (d * d));
        bodies[0].applyForce(f);
        bodies[1].applyForce(-f);
    });
    w.setIntegrator(std::make_unique<phys::VelocityVerlet>());

    const double dt = 1.0 / 2000.0;
    const double T = 2.0 * M_PI / omega; // период
    for (double t = 0.0; t < T; t += dt) w.step(dt);

    // После полного периода тела возвращаются в исходные точки (погрешность O(dt²)).
    CHECK(w.bodies()[0].pos.x == doctest::Approx(-r1).epsilon(1e-4));
    CHECK(w.bodies()[0].pos.y == doctest::Approx(0.0).epsilon(1e-4));
    CHECK(w.bodies()[1].pos.x == doctest::Approx(r2).epsilon(1e-4));

    // Расстояние между телами постоянно и равно a.
    double dist = (w.bodies()[1].pos - w.bodies()[0].pos).len();
    CHECK(dist == doctest::Approx(a).epsilon(1e-3));
}
