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

/* Collapsed rail (Harbor: icon-only, centered, active = gold icon with NO pill). */
QPushButton#NavItem[collapsed="true"]                { text-align: center; padding-left: 0; padding-right: 0; background: transparent; }
QPushButton#NavItem[collapsed="true"]:hover          { background: rgba(255,255,255,0.05); }
QPushButton#NavItem[collapsed="true"][active="true"] { background: transparent; }

/* Collapse toggle (Harbor's CollapseToggle, bottom of the rail). */
QPushButton#CollapseToggle {
    color: #8b909b;
    background: transparent;
    border: none;
    text-align: left;
    padding-left: 12px;
    border-radius: 8px;
    font-size: 13px;
    font-weight: 500;
}
QPushButton#CollapseToggle:hover                { background: rgba(255,255,255,0.05); color: #aab1bd; }
QPushButton#CollapseToggle[collapsed="true"]    { text-align: center; padding-left: 0; }

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

/* ── Row hover edge-arrows (Harbor: hidden scrollbar, arrows on hover) ── */
QPushButton#RowArrow {
    background: rgba(18,19,23,0.86);
    border: 1px solid rgba(255,255,255,0.14);
    border-radius: 18px;
}
QPushButton#RowArrow:hover { background: rgba(45,51,63,0.96); }

/* ── Featured hero (Step 3b — Harbor hero-carousel + hero) ── */
QWidget#FeaturedHero, QWidget#HeroContent { background: transparent; }

QLabel#HeroRank {
    background: rgba(18,19,23,0.85);
    border-radius: 6px;
    padding: 4px 10px;
    color: #f3f1ea;
    font-size: 12px;
    font-weight: 600;
}
QLabel#HeroTitle {
    color: #f3f1ea;
    font-family: "Fraunces", "Iowan Old Style", "Georgia", serif;
    font-size: 60px;
    font-weight: 500;
}
QLabel#HeroDesc  { color: #aab1bd; font-size: 16px; }
QLabel#HeroStats { color: #f3f1ea; font-size: 14px; }

QPushButton#HeroPlayBtn {
    background: #f3f1ea;
    color: #121317;
    border: none;
    border-radius: 24px;
    padding: 0 28px;
    font-size: 15px;
    font-weight: 600;
}
QPushButton#HeroPlayBtn:hover { background: #ffffff; }

QPushButton#HeroAddBtn {
    background: rgba(18,19,23,0.55);
    color: #f3f1ea;
    border: 1px solid rgba(255,255,255,0.16);
    border-radius: 24px;
    padding: 0 22px;
    font-size: 15px;
    font-weight: 500;
}
QPushButton#HeroAddBtn:hover {
    background: rgba(18,19,23,0.78);
    border-color: rgba(255,255,255,0.28);
}
QPushButton#HeroAddBtn[inwatch="true"] {
    color: #e8b923;
    border-color: rgba(232,185,35,0.5);
}

QPushButton#HeroDot {
    border: none;
    border-radius: 3px;
    background: rgba(170,177,189,0.7);
}
QPushButton#HeroDot[active="true"] { background: #f3f1ea; }

/* ── Detail page (Detail path — Harbor detail.tsx) ── */
QWidget#DetailPage, QScrollArea#DetailScroll, QWidget#DetailPageBody { background: #121317; }
QScrollArea#DetailScroll { border: none; }
QScrollArea#DetailScroll > QWidget { background: #121317; }

QLabel#DetailTitle {
    color: #f3f1ea;
    font-family: "Fraunces", "Iowan Old Style", "Georgia", serif;
    font-size: 56px;
    font-weight: 500;
}
QLabel#DetailPill {
    background: rgba(18,19,23,0.85);
    border: 1px solid rgba(255,255,255,0.10);
    border-radius: 13px;
    padding: 4px 12px;
    color: #aab1bd;
    font-size: 13px;
}
QLabel#DetailSynopsis { color: #aab1bd; font-size: 16px; }

QLabel#DetailSectionTitle {
    color: #f3f1ea;
    font-family: "Fraunces", "Georgia", serif;
    font-size: 22px;
    font-weight: 600;
}
QLabel#DetailEpCount { color: #8b909b; font-size: 13px; }

QPushButton#SeasonTrigger {
    background: rgba(18,19,23,0.85);
    border: 1px solid rgba(255,255,255,0.12);
    border-radius: 18px;
    padding: 7px 16px;
    color: #f3f1ea;
    font-size: 13px;
    font-weight: 500;
}
QPushButton#SeasonTrigger:hover { border-color: #e8b923; background: #1a1d24; }

QMenu#SeasonMenu {
    background: #1a1d24;
    border: 1px solid rgba(255,255,255,0.10);
    border-radius: 14px;
    padding: 6px;
}
QMenu#SeasonMenu::item {
    padding: 9px 30px 9px 16px;
    margin: 1px 0;
    border-radius: 8px;
    color: #aab1bd;
    font-size: 13px;
}
QMenu#SeasonMenu::indicator { width: 0px; height: 0px; }
QMenu#SeasonMenu::item:selected { background: rgba(255,255,255,0.07); color: #f3f1ea; }
QMenu#SeasonMenu::item:checked  { color: #e8b923; }

QWidget#EpisodeRow { background: transparent; border-radius: 14px; }
QWidget#EpisodeRow:hover { background: rgba(255,255,255,0.04); }
QLabel#EpThumb { background: #1a1d24; border-radius: 8px; }
QLabel#EpTitle { color: #f3f1ea; font-size: 15px; font-weight: 600; }
QLabel#EpMeta  { color: #8b909b; font-size: 12px; }

QPushButton#DetailBack {
    background: rgba(18,19,23,0.7);
    border: 1px solid rgba(255,255,255,0.14);
    border-radius: 20px;
    color: #f3f1ea;
    font-size: 18px;
}
QPushButton#DetailBack:hover { background: rgba(45,51,63,0.95); }
)qss");
}

} // namespace tankoban
