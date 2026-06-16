#pragma once
// Shared chrome tokens (spec §10). Colors are computed from Harbor's exact CSS
// values so we paint the identical pixels the browser does.
#include <QColor>
#include <algorithm>
#include <cmath>

namespace theme {

// OKLCH -> sRGB (Björn Ottosson's matrices) so oklch(...) tokens match the
// browser exactly — no hand-converted hex drift.
inline QColor oklch(double L, double C, double H)
{
    constexpr double kPi = 3.14159265358979323846;
    const double h = H * kPi / 180.0;
    const double a = C * std::cos(h), b = C * std::sin(h);
    const double l_ = L + 0.3963377774 * a + 0.2158037573 * b;
    const double m_ = L - 0.1055613458 * a - 0.0638541728 * b;
    const double s_ = L - 0.0894841775 * a - 1.2914855480 * b;
    const double l = l_ * l_ * l_, m = m_ * m_ * m_, s = s_ * s_ * s_;
    double r  =  4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s;
    double g  = -1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s;
    double bl = -0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s;
    auto enc = [](double c) {
        c = std::clamp(c, 0.0, 1.0);
        return c <= 0.0031308 ? 12.92 * c : 1.055 * std::pow(c, 1.0 / 2.4) - 0.055;
    };
    return QColor::fromRgbF(enc(r), enc(g), enc(bl));
}

// Harbor player accent — the player scopes its own --accent to this (player.css).
inline QColor accent()        { return oklch(0.78, 0.13, 60.0); }
inline QColor white(double a) { QColor c(255, 255, 255); c.setAlphaF(a); return c; }
inline QColor black(double a) { QColor c(0, 0, 0);       c.setAlphaF(a); return c; }
}
