#include "util/TimeFormat.h"
#include <cmath>

QString fmtTime(double sec)
{
    if (!std::isfinite(sec) || sec < 0) return QStringLiteral("0:00");
    const long long total = (long long)std::floor(sec);
    const long long h = total / 3600;
    const long long m = (total % 3600) / 60;
    const long long s = total % 60;
    if (h > 0)
        return QStringLiteral("%1:%2:%3")
            .arg(h).arg(m, 2, 10, QLatin1Char('0')).arg(s, 2, 10, QLatin1Char('0'));
    return QStringLiteral("%1:%2").arg(m).arg(s, 2, 10, QLatin1Char('0'));
}
