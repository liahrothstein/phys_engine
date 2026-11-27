// physengine: реализация программного рендера (canvas.cpp).
#include "phys/render.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_WRITE_NO_PSD 1 // собираем только PNG-путь
#include "stb_image_write.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace phys::render {

Canvas::Canvas(int width, int height)
    : w_(std::max(1, width)), h_(std::max(1, height)),
      rgb_(static_cast<size_t>(w_) * h_ * 3, 20) {}

void Canvas::fill(Color c) {
    for (size_t i = 0; i < rgb_.size(); i += 3) {
        rgb_[i] = c.r;
        rgb_[i + 1] = c.g;
        rgb_[i + 2] = c.b;
    }
}

void Canvas::drawGrid(int step, Color c) {
    if (step <= 0) return;
    for (int x = 0; x < w_; x += step)
        for (int y = 0; y < h_; ++y) blend(x, y, c, 0.35);
    for (int y = 0; y < h_; y += step)
        for (int x = 0; x < w_; ++x) blend(x, y, c, 0.35);
}

void Canvas::blend(int x, int y, Color c, double a) {
    if (x < 0 || y < 0 || x >= w_ || y >= h_ || a <= 0.0) return;
    if (a > 1.0) a = 1.0;
    size_t i = (static_cast<size_t>(y) * w_ + x) * 3;
    double ia = 1.0 - a;
    rgb_[i] = static_cast<std::uint8_t>(c.r * a + rgb_[i] * ia);
    rgb_[i + 1] = static_cast<std::uint8_t>(c.g * a + rgb_[i + 1] * ia);
    rgb_[i + 2] = static_cast<std::uint8_t>(c.b * a + rgb_[i + 2] * ia);
}

Color Canvas::pixel(int x, int y) const {
    if (x < 0 || y < 0 || x >= w_ || y >= h_) return {};
    size_t i = (static_cast<size_t>(y) * w_ + x) * 3;
    return {rgb_[i], rgb_[i + 1], rgb_[i + 2]};
}

void Canvas::fillCircle(double px, double py, double radius, Color c) {
    if (radius <= 0.0) radius = 0.5; // хотя бы полпикселя
    const double r2 = radius * radius;
    // Антиалиасинг: пиксель с центром внутри r-0.5 — полный, в кольце — по расстоянию.
    const double inner = std::max(0.0, radius - 0.7);
    const double inner2 = inner * inner;
    int x0 = static_cast<int>(std::floor(px - radius)) - 1;
    int x1 = static_cast<int>(std::ceil(px + radius)) + 1;
    int y0 = static_cast<int>(std::floor(py - radius)) - 1;
    int y1 = static_cast<int>(std::ceil(py + radius)) + 1;
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            double dx = x + 0.5 - px;
            double dy = y + 0.5 - py;
            double d2 = dx * dx + dy * dy;
            if (d2 >= r2) continue;
            double a = d2 <= inner2 ? 1.0
                                    : 1.0 - (std::sqrt(d2) - inner) / (radius - inner);
            blend(x, y, c, a);
        }
    }
}

void Canvas::drawGlow(double px, double py, double radius, Color core, Color halo) {
    const double haloR = 3.0 * std::max(radius, 1.0);
    const double halo2 = haloR * haloR;
    int x0 = static_cast<int>(std::floor(px - haloR)) - 1;
    int x1 = static_cast<int>(std::ceil(px + haloR)) + 1;
    int y0 = static_cast<int>(std::floor(py - haloR)) - 1;
    int y1 = static_cast<int>(std::ceil(py + haloR)) + 1;
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            double dx = x + 0.5 - px;
            double dy = y + 0.5 - py;
            double d2 = dx * dx + dy * dy;
            if (d2 >= halo2) continue;
            double t = std::sqrt(d2) / haloR;      // 0 в центре → 1 на краю
            double a = 0.55 * (1.0 - t) * (1.0 - t); // квадратичное затухание
            blend(x, y, halo, a);
        }
    }
    fillCircle(px, py, radius, core);
}

void Canvas::drawLine(double x1, double y1, double x2, double y2, Color c,
                      double width, double alpha) {
    // Растеризация Брезенхэма с «толщиной» через несколько смещённых проходов.
    double dx = x2 - x1, dy = y2 - y1;
    double len = std::sqrt(dx * dx + dy * dy);
    if (len < 1e-9) return;
    int steps = static_cast<int>(std::ceil(len * 2.0));
    int passes = std::max(1, static_cast<int>(std::ceil(width)));
    for (int p = 0; p < passes; ++p) {
        double off = (passes == 1) ? 0.0 : (p - (passes - 1) * 0.5) * 0.6;
        // смещение перпендикулярно линии
        double nx = -dy / len * off;
        double ny = dx / len * off;
        for (int s = 0; s <= steps; ++s) {
            double t = static_cast<double>(s) / steps;
            blend(static_cast<int>(std::lround(x1 + dx * t + nx)),
                  static_cast<int>(std::lround(y1 + dy * t + ny)),
                  c, alpha / passes);
        }
    }
}

void Canvas::drawTrail(const std::vector<Vec2>& pointsPx, Color c,
                       double width, bool fade) {
    if (pointsPx.size() < 2) return;
    for (size_t i = 1; i < pointsPx.size(); ++i) {
        double a = fade ? static_cast<double>(i) / pointsPx.size() : 1.0;
        drawLine(pointsPx[i - 1].x, pointsPx[i - 1].y,
                 pointsPx[i].x, pointsPx[i].y, c, width, 0.85 * a);
    }
}

bool Canvas::savePng(const std::string& path) const {
    return stbi_write_png(path.c_str(), w_, h_, 3, rgb_.data(), w_ * 3) != 0;
}

Vec2 Camera::toPixels(const Vec2& world) const {
    double s = scale();
    // мировая Y вверх → пиксельная Y вниз; центр кадра = center.
    double px = static_cast<double>(pxW) * 0.5 + (world.x - center.x) * s;
    double py = static_cast<double>(pxH) * 0.5 - (world.y - center.y) * s;
    return {px, py};
}

std::vector<Vec2> project(const std::vector<Vec2>& worldPts, const Camera& cam) {
    std::vector<Vec2> out;
    out.reserve(worldPts.size());
    for (const auto& p : worldPts) out.push_back(cam.toPixels(p));
    return out;
}

} // namespace phys::render
