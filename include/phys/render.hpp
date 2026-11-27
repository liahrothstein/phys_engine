// physengine: программный рендер кадров (headless-визуализация).
// Рисование в RGB-холст и сохранение в PNG через stb_image_write.
// Никакой внешней графики: работает в CI и на машине без дисплея.
#pragma once
#include "phys/math_vec2.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace phys::render {

struct Color {
    std::uint8_t r = 0, g = 0, b = 0;
    constexpr Color() = default;
    constexpr Color(std::uint8_t rr, std::uint8_t gg, std::uint8_t bb)
        : r(rr), g(gg), b(bb) {}
};

/// Холст: ось Y пикселей направлена ВНИЗ (как в растровых форматах).
class Canvas {
public:
    Canvas(int width, int height);

    int width() const { return w_; }
    int height() const { return h_; }

    void fill(Color c);
    /// Фоновая сетка с шагом step пикселей (полупрозрачная).
    void drawGrid(int step, Color c);

    /// Заливной круг со сглаженным краем (координаты в пикселях).
    void fillCircle(double px, double py, double radius, Color c);
    /// Светящееся тело: яркое ядро радиусом r + мягкий ореол до 3r.
    void drawGlow(double px, double py, double radius, Color core, Color halo);
    /// Отрезок толщиной width; alpha — для следов и вспомогательных кривых.
    void drawLine(double x1, double y1, double x2, double y2, Color c,
                  double width = 1.0, double alpha = 1.0);
    /// След траектории (пиксельные точки); при fade=true яркость нарастает
    /// от хвоста к голове.
    void drawTrail(const std::vector<Vec2>& pointsPx, Color c,
                   double width = 1.5, bool fade = true);

    Color pixel(int x, int y) const;
    bool savePng(const std::string& path) const;

private:
    int w_ = 0, h_ = 0;
    std::vector<std::uint8_t> rgb_; // 3 байта на пиксель
    void blend(int x, int y, Color c, double a);
};

/// Камера: мировые координаты (м, ось Y вверх) → пиксели холста.
struct Camera {
    Vec2 center{0.0, 0.0};  // центр обзора, м
    double halfW = 10.0;    // половина ширины обзора, м
    int pxW = 1280, pxH = 720;

    double scale() const { return static_cast<double>(pxW) / (2.0 * halfW); }
    Vec2 toPixels(const Vec2& world) const;
};

/// Перевод массива мировых точек в пиксельные (для следов).
std::vector<Vec2> project(const std::vector<Vec2>& worldPts, const Camera& cam);

} // namespace phys::render
