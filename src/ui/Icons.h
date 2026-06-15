// Tankoban 3 — nav icons (Step 2b).
//
// Lucide-style line icons rendered from inline SVG to a QIcon in a given color, so
// the sidebar reads as Harbor's icon rail (icon + label) rather than text bars.
// Recolored per state (muted default, ink active) by re-rendering.

#pragma once

#include <QColor>
#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QString>
#include <QSvgRenderer>

namespace tankoban {

inline QString navSvgInner(const QString& id)
{
    if (id == QLatin1String("home"))
        return QStringLiteral("<path d='m3 9 9-7 9 7v11a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z'/><path d='M9 22V12h6v10'/>");
    if (id == QLatin1String("discover"))
        return QStringLiteral("<circle cx='12' cy='12' r='10'/><polygon points='16.24 7.76 14.12 14.12 7.76 16.24 9.88 9.88'/>");
    if (id == QLatin1String("movies"))
        return QStringLiteral("<rect width='18' height='18' x='3' y='3' rx='2'/><path d='M7 3v18M17 3v18M3 7.5h4M17 7.5h4M3 12h18M3 16.5h4M17 16.5h4'/>");
    if (id == QLatin1String("shows"))
        return QStringLiteral("<rect width='20' height='15' x='2' y='7' rx='2'/><polyline points='17 2 12 7 7 2'/>");
    if (id == QLatin1String("anime"))
        return QStringLiteral("<polygon points='12 2 15.09 8.26 22 9.27 17 14.14 18.18 21.02 12 17.77 5.82 21.02 7 14.14 2 9.27 8.91 8.26 12 2'/>");
    if (id == QLatin1String("library"))
        return QStringLiteral("<path d='M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2z'/><path d='M4 19.5A2.5 2.5 0 0 1 6.5 17H20'/>");
    if (id == QLatin1String("downloads"))
        return QStringLiteral("<path d='M12 3v12'/><path d='m7 10 5 5 5-5'/><path d='M5 21h14'/>");
    if (id == QLatin1String("addons"))
        return QStringLiteral("<rect x='3' y='3' width='7' height='7' rx='1'/><rect x='14' y='3' width='7' height='7' rx='1'/><rect x='3' y='14' width='7' height='7' rx='1'/><rect x='14' y='14' width='7' height='7' rx='1'/>");
    if (id == QLatin1String("settings"))
        return QStringLiteral("<circle cx='12' cy='12' r='3'/><path d='M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 1 1-2.83 2.83l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-4 0v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 1 1-2.83-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1 0-4h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 1 1 2.83-2.83l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 4 0v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 1 1 2.83 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 0 4h-.09a1.65 1.65 0 0 0-1.51 1z'/>");
    if (id == QLatin1String("chev-left"))
        return QStringLiteral("<path d='m15 18-6-6 6-6'/>");
    if (id == QLatin1String("chev-right"))
        return QStringLiteral("<path d='m9 18 6-6-6-6'/>");
    // Harbor's CollapseToggle glyphs (lucide PanelLeftClose / PanelLeftOpen).
    if (id == QLatin1String("panel-collapse"))
        return QStringLiteral("<rect width='18' height='18' x='3' y='3' rx='2'/>"
                              "<path d='M9 3v18'/><path d='m16 15-3-3 3-3'/>");
    if (id == QLatin1String("panel-expand"))
        return QStringLiteral("<rect width='18' height='18' x='3' y='3' rx='2'/>"
                              "<path d='M9 3v18'/><path d='m14 9 3 3-3 3'/>");
    return QString();
}

inline QIcon navIcon(const QString& id, const QColor& color, int px = 22)
{
    const QString svg = QStringLiteral(
        "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='none' "
        "stroke='%1' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'>%2</svg>")
        .arg(color.name(), navSvgInner(id));

    QSvgRenderer renderer(svg.toUtf8());
    const int hp = px * 2; // render at 2x for crisp HiDPI
    QPixmap pm(hp, hp);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    renderer.render(&p);
    p.end();
    pm.setDevicePixelRatio(2.0);
    return QIcon(pm);
}

} // namespace tankoban
