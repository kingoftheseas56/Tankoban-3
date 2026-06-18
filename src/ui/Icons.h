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
    if (id == QLatin1String("anime")) // Harbor's anime mark is a cat (lucide cat)
        return QStringLiteral(
            "<path d='M12 5c.67 0 1.35.09 2 .26 1.78-2 5.03-2.84 6.42-2.26 1.4.58-.42 7-.42 7 "
            ".57 1.07 1 2.24 1 3.44C21 17.9 16.97 21 12 21s-9-3-9-7.56c0-1.25.5-2.4 1-3.44 0 0"
            "-1.89-6.42-.5-7 1.39-.58 4.72.23 6.5 2.23A9.04 9.04 0 0 1 12 5Z'/>"
            "<path d='M8 14v.5'/><path d='M16 14v.5'/>"
            "<path d='M11.25 16.25h1.5L12 17l-.75-.75Z'/>");
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
    if (id == QLatin1String("bookmark"))
        return QStringLiteral("<path d='m19 21-7-4-7 4V5a2 2 0 0 1 2-2h10a2 2 0 0 1 2 2z'/>");
    if (id == QLatin1String("play")) // lucide Play (filled via navIcon's `filled` flag)
        return QStringLiteral("<polygon points='6 3 20 12 6 21 6 3'/>");
    // Harbor's CollapseToggle glyphs (lucide PanelLeftClose / PanelLeftOpen).
    if (id == QLatin1String("panel-collapse"))
        return QStringLiteral("<rect width='18' height='18' x='3' y='3' rx='2'/>"
                              "<path d='M9 3v18'/><path d='m16 15-3-3 3-3'/>");
    if (id == QLatin1String("panel-expand"))
        return QStringLiteral("<rect width='18' height='18' x='3' y='3' rx='2'/>"
                              "<path d='M9 3v18'/><path d='m14 9 3 3-3 3'/>");
    return QString();
}

inline QIcon navIcon(const QString& id, const QColor& color, int px = 22, bool filled = false)
{
    // filled = solid fill (e.g. the Play triangle); default = lucide line style (stroked).
    const QString svg = QStringLiteral(
        "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='%1' "
        "stroke='%2' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'>%3</svg>")
        .arg(filled ? color.name() : QStringLiteral("none"),
             filled ? QStringLiteral("none") : color.name(),
             navSvgInner(id));

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

// Harbor's MalLogo (components/icons/mal-logo.tsx) ported 1:1 — the MyAnimeList wordmark
// rendered to a pixmap at the given ink color + pixel height (2x for HiDPI). Shared by the
// anime hero score chip and the poster-card score badge.
inline QPixmap malLogoPixmap(const QColor& color, int heightPx)
{
    const QString svg = QStringLiteral(
        "<svg xmlns='http://www.w3.org/2000/svg' viewBox='256 409 512 182' fill='%1'>"
        "<path d='M432.49 410.61V590.3l-44.86-.06V479l-43.31 51.29-42.43-52.44-.43 112.75H256"
        "V410.65h47l39.79 54.29 43-54.31zm184.06 44.14.53 135.15h-50.45l-.17-61.25h-59.73c1.49"
        " 10.65 4.48 27 8.9 38 3.31 8.13 6.36 16 12.44 24.06l-36.37 24c-7.45-13.57-13.27-28.52"
        "-18.73-44.42a198.31 198.31 0 0 1-10.82-46.49c-1.81-16-2.07-31.38 2.28-47.19a83.37 83.37"
        " 0 0 1 24.77-39.81c6.68-6.25 16-10.67 23.47-14.66s15.85-5.63 23.62-7.66a158 158 0 0 1"
        " 25.41-3.9c8.49-.73 23.62-1.41 51-.6l11.63 37.31h-58.78c-12.65.17-18.73 0-28.61 4.46a47.7"
        " 47.7 0 0 0-27.26 41l56.81.7.81-38.61h49.26zM701.72 410v141.35L768 552l-9.17 37.87H656.28"
        "V409.33z'/></svg>")
        .arg(color.name());
    QSvgRenderer r(svg.toUtf8());
    const QSizeF vb = r.viewBoxF().size();
    const qreal aspect = vb.height() > 0 ? vb.width() / vb.height() : 2.8;
    const int hp = heightPx * 2;
    QPixmap pm(int(hp * aspect), hp);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    r.render(&p);
    p.end();
    pm.setDevicePixelRatio(2.0);
    return pm;
}

} // namespace tankoban
