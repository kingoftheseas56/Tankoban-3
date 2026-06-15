// Tankoban 3 — Theme (Step 2).
//
// Harbor's dark design tokens as a Qt stylesheet. Faithful to Harbor's OKLCH
// hue-260 ladder + the gold accent, baked to sRGB (same values Tankoban 2 used).
// For now this is a single QSS string; a proper token system can come later if we
// want runtime theme presets. Step 2 = the shell, so this covers the shell only.

#pragma once

#include <QString>

namespace tankoban {

inline QString appStyleSheet()
{
    return QStringLiteral(R"qss(
* { font-family: "Inter", "Segoe UI", sans-serif; color: #f3f1ea; }

QWidget#Root           { background: #121317; }
QStackedWidget#Content { background: #121317; }
QWidget#ContentPage    { background: #121317; }

/* ── Sidebar (Harbor: bg-canvas, right hairline, 240px) ── */
QWidget#Sidebar { background: #121317; border-right: 1px solid rgba(255,255,255,0.06); }

QLabel#Brand {
    color: #e8b923;
    font-family: "Fraunces", "Iowan Old Style", "Georgia", serif;
    font-size: 30px;
    font-weight: 600;
    padding-left: 6px;
}

/* ── Nav item (Harbor: h-14, rounded-xl, gold/elevated active) ── */
QPushButton#NavItem {
    color: #aab1bd;
    background: transparent;
    border: none;
    text-align: left;
    padding-left: 16px;
    border-radius: 12px;
    font-size: 15px;
}
QPushButton#NavItem:hover            { background: rgba(255,255,255,0.05); color: #f3f1ea; }
QPushButton#NavItem[active="true"]   { background: #232833; color: #f3f1ea; font-weight: 600; }

QFrame#NavDivider { background: rgba(255,255,255,0.07); border: none; }

/* ── Placeholder content pages ── */
QLabel#PlaceholderTitle {
    color: #f3f1ea;
    font-family: "Fraunces", "Georgia", serif;
    font-size: 40px;
    font-weight: 600;
}
QLabel#PlaceholderSub { color: #8b909b; font-size: 13px; letter-spacing: 1px; }

/* ── Catalogue rows + poster cards (Step 3) ── */
QScrollArea#HomeScroll, QScrollArea#RowScroll { background: transparent; border: none; }
QScrollArea#HomeScroll > QWidget, QScrollArea#RowScroll > QWidget { background: transparent; }
QWidget#RowTrack { background: transparent; }

QLabel#RowTitle {
    color: #f3f1ea;
    font-family: "Fraunces", "Georgia", serif;
    font-size: 20px;
    font-weight: 600;
}
QLabel#RowStatus { color: #8b909b; font-size: 13px; }

QLabel#Poster {
    background: #1a1d24;
    border: 1px solid rgba(255,255,255,0.06);
    border-radius: 6px;
}
QWidget#PosterCard[hover="true"] QLabel#Poster { border: 2px solid #e8b923; }
QLabel#PosterTitle { color: #aab1bd; font-size: 12px; }
)qss");
}

} // namespace tankoban
