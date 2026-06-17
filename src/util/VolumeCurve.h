#pragma once

#include <QColor>

namespace volume_curve {

inline constexpr double kMax = 6.0;
inline constexpr double kNormalFraction = 0.6;
inline constexpr int kTrackWidth = 120;

double fractionFromValue(double value);
double valueFromFraction(double fraction);
QColor boostColor(double value);

} // namespace volume_curve
