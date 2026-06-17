#include "util/VolumeCurve.h"

#include <algorithm>
#include <cmath>

namespace volume_curve {

double fractionFromValue(double value)
{
    const double clamped = std::clamp(value, 0.0, kMax);
    if (clamped <= 1.0) return clamped * kNormalFraction;
    return kNormalFraction + ((clamped - 1.0) / (kMax - 1.0)) * (1.0 - kNormalFraction);
}

double valueFromFraction(double fraction)
{
    const double clamped = std::clamp(fraction, 0.0, 1.0);
    if (clamped <= kNormalFraction) return (clamped / kNormalFraction);
    return 1.0 + ((clamped - kNormalFraction) / (1.0 - kNormalFraction)) * (kMax - 1.0);
}

QColor boostColor(double value)
{
    if (value <= 1.0) return QColor(255, 255, 255);
    const double t = std::min(1.0, (value - 1.0) / (kMax - 1.0));
    const int r = int(std::lround(249.0 - t * (249.0 - 220.0)));
    const int g = int(std::lround(115.0 - t * (115.0 - 38.0)));
    const int b = int(std::lround(22.0 - t * (22.0 - 38.0)));
    return QColor(r, g, b);
}

} // namespace volume_curve
