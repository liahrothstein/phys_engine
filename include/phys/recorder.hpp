// physengine: запись временных рядов в CSV для последующих графиков.
// Колонки: t и построчные величины тел; формат фиксирован экспериментальной
// главой: t, затем x,y,vx,vy,E для каждого отслеживаемого тела.
#pragma once
#include "phys/body.hpp"
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace phys {

class Recorder {
public:
    // path — файл CSV; если пуст, рекордер неактивен.
    explicit Recorder(const std::string& path, double every = 0.02)
        : path_(path), every_(every > 0.0 ? every : 0.02) {}

    bool active() const { return !path_.empty(); }

    void begin(const std::vector<Body>& bodies) {
        if (!active()) return;
        file_ = std::fopen(path_.c_str(), "w");
        if (!file_) return;
        std::fprintf(file_, "t");
        for (size_t i = 0; i < bodies.size(); ++i)
            std::fprintf(file_, ",x%d,y%d,vx%d,vy%d,E%d", (int)i, (int)i, (int)i, (int)i, (int)i);
        std::fprintf(file_, "\n");
        nextT_ = 0.0;
    }

    // Пишет строку, когда t достиг очередного кратного every.
    void sample(double t, const std::vector<Body>& bodies,
                const std::function<double(const Body&)>& extraEnergy) {
        if (!file_ || t + 1e-12 < nextT_) return;
        nextT_ += every_;
        std::fprintf(file_, "%.6f", t);
        for (const auto& b : bodies) {
            double e = 0.5 * b.mass * b.vel.len2() + extraEnergy(b);
            std::fprintf(file_, ",%.8f,%.8f,%.8f,%.8f,%.8f",
                         b.pos.x, b.pos.y, b.vel.x, b.vel.y, e);
        }
        std::fprintf(file_, "\n");
    }

    void end() {
        if (file_) {
            std::fclose(file_);
            file_ = nullptr;
        }
    }

    ~Recorder() { end(); }

private:
    std::string path_;
    double every_ = 0.02;
    double nextT_ = 0.0;
    std::FILE* file_ = nullptr;
};

} // namespace phys
