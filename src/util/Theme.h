#pragma once
// Shared chrome tokens (spec §10). Harbor's seek/accent gold is oklch(0.78 0.13 60);
// converted to sRGB = #F4A25C. (Matches our OKLCH gold ladder — swap to the project
// token when the theme system lands.)
#include <QColor>

namespace theme {
// Harbor accent (default theme `--color-accent`) = #e8b923 — the canonical gold
// the Electron port hardcodes. (Theme switching is EXTRA; CORE uses the default.)
inline QColor accent()        { return QColor(0xE8, 0xB9, 0x23); }
inline QColor white(double a) { QColor c(255, 255, 255); c.setAlphaF(a); return c; }
inline QColor black(double a) { QColor c(0, 0, 0);       c.setAlphaF(a); return c; }
}
