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
    font-size: 17px;
    font-weight: 500;
}
QLabel#RowStatus { color: #8b909b; font-size: 13px; }

QLabel#Poster {
    background: #1a1d24;
    border: 1px solid rgba(255,255,255,0.06);
    border-radius: 12px;
}
/* Harbor PickCard hover = lift + soft shadow + subtle ring (NOT a gold accent). The
   blurred shadow is a QGraphicsDropShadowEffect on the poster (PosterCard); here we just
   firm up the inset ring on hover. Gold is reserved for true Harbor accents, not cards. */
QWidget#PosterCard[hover="true"] QLabel#Poster { border: 1px solid rgba(255,255,255,0.22); }
QLabel#PosterTitle { color: #f3f1ea; font-size: 13px; font-weight: 500; }

/* ── Row hover edge-arrows (Harbor: hidden scrollbar, arrows on hover) ── */
QPushButton#RowArrow {
    background: rgba(18,19,23,0.90);
    border: 1px solid rgba(255,255,255,0.14);
    border-radius: 22px;
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

/* ── Addons store (Addons path — Harbor addons.tsx) ── */
QPushButton#AddonTab {
    background: transparent;
    border: none;
    color: #8b909b;
    font-size: 15px;
    font-weight: 600;
    padding: 6px 2px;
    margin-right: 16px;
}
QPushButton#AddonTab:hover            { color: #aab1bd; }
QPushButton#AddonTab[active="true"]   { color: #f3f1ea; border-bottom: 2px solid #e8b923; }

QLineEdit#AddonUrlInput {
    background: rgba(255,255,255,0.04);
    border: 1px solid rgba(255,255,255,0.10);
    border-radius: 12px;
    padding: 0 14px;
    color: #f3f1ea;
    font-size: 14px;
    selection-background-color: #e8b923;
    selection-color: #121317;
}
QLineEdit#AddonUrlInput:focus { border-color: #8b909b; }

QPushButton#AddonInstallBtn {
    background: #f3f1ea;
    color: #121317;
    border: none;
    border-radius: 12px;
    padding: 0 22px;
    font-size: 14px;
    font-weight: 600;
}
QPushButton#AddonInstallBtn:hover { background: #ffffff; }

QLabel#AddonStatus              { color: #7ddc8f; font-size: 13px; }
QLabel#AddonStatus[err="true"]  { color: #e8736b; }

QScrollArea#AddonScroll { background: #121317; border: none; }
QScrollArea#AddonScroll > QWidget { background: #121317; }

QLabel#AddonPanePlaceholder { color: #8b909b; font-size: 15px; }

QWidget#AddonInstalledRow {
    background: #1a1d24;
    border: 1px solid rgba(255,255,255,0.06);
    border-radius: 14px;
}
QWidget#AddonInstalledRow:hover { border: 1px solid rgba(255,255,255,0.14); }
QLabel#AddonAvatar {
    background: #232833;
    border-radius: 10px;
    color: #e8b923;
    font-family: "Fraunces", "Georgia", serif;
    font-size: 22px;
    font-weight: 600;
}
QLabel#AddonName { color: #f3f1ea; font-size: 15px; font-weight: 600; }
QLabel#AddonSub  { color: #8b909b; font-size: 12px; }
QPushButton#AddonRemoveBtn {
    background: rgba(255,255,255,0.05);
    border: 1px solid rgba(255,255,255,0.10);
    border-radius: 14px;
    padding: 6px 14px;
    color: #aab1bd;
    font-size: 12px;
    font-weight: 600;
}
QPushButton#AddonRemoveBtn:hover {
    background: rgba(232,115,107,0.15);
    color: #e8736b;
    border: 1px solid rgba(232,115,107,0.40);
}

QWidget#AddonEmpty {
    background: rgba(255,255,255,0.02);
    border: 1px solid rgba(255,255,255,0.06);
    border-radius: 18px;
}
QLabel#AddonEmptyTitle {
    color: #f3f1ea;
    font-family: "Fraunces", "Georgia", serif;
    font-size: 22px;
    font-weight: 600;
}
QLabel#AddonEmptyText { color: #8b909b; font-size: 13px; }

/* ── Addons marketplace (Browse/Discover — Harbor store) ── */
QLineEdit#AddonSearchInput {
    background: rgba(255,255,255,0.04);
    border: 1px solid rgba(255,255,255,0.10);
    border-radius: 20px;
    padding: 0 16px;
    color: #f3f1ea;
    font-size: 14px;
    selection-background-color: #e8b923;
    selection-color: #121317;
}
QLineEdit#AddonSearchInput:focus { border-color: #8b909b; }

QPushButton#AddonAdultBtn {
    background: transparent;
    border: 1px solid rgba(255,255,255,0.12);
    border-radius: 20px;
    padding: 0 16px;
    color: #8b909b;
    font-size: 12px;
    font-weight: 700;
}
QPushButton#AddonAdultBtn:checked {
    background: rgba(255,255,255,0.10);
    color: #f3f1ea;
    border-color: #f3f1ea;
}

QScrollArea#StoreCatScroll { background: transparent; border: none; }
QScrollArea#StoreCatScroll > QWidget { background: transparent; }

QPushButton#StoreChip {
    background: rgba(255,255,255,0.04);
    border: 1px solid rgba(255,255,255,0.08);
    border-radius: 16px;
    padding: 6px 14px;
    color: #aab1bd;
    font-size: 13px;
    font-weight: 600;
}
QPushButton#StoreChip:hover   { background: rgba(255,255,255,0.08); color: #f3f1ea; }
QPushButton#StoreChip:checked { background: #f3f1ea; color: #121317; border-color: #f3f1ea; }

QPushButton#StoreLoadMore {
    background: rgba(255,255,255,0.05);
    border: 1px solid rgba(255,255,255,0.10);
    border-radius: 20px;
    padding: 0 28px;
    color: #f3f1ea;
    font-size: 13px;
    font-weight: 600;
}
QPushButton#StoreLoadMore:hover { background: rgba(255,255,255,0.10); }

QWidget#StoreCard {
    background: #1a1d24;
    border: 1px solid rgba(255,255,255,0.06);
    border-radius: 14px;
}
QWidget#StoreCard[hover="true"] { border: 1px solid rgba(232,185,35,0.5); }
QLabel#StoreCardLogo {
    background: #232833;
    border-radius: 10px;
    color: #e8b923;
    font-family: "Fraunces", "Georgia", serif;
    font-size: 20px;
    font-weight: 600;
}
QLabel#StoreCardName  { color: #f3f1ea; font-size: 15px; font-weight: 600; }
QLabel#StoreCardStars { color: #e8b923; font-size: 12px; font-weight: 600; }
QLabel#StoreCardDesc  { color: #8b909b; font-size: 12px; }
QLabel#StoreCardCat   { color: #8b909b; font-size: 11px; }
QPushButton#StoreCardInstall {
    background: #f3f1ea;
    color: #121317;
    border: none;
    border-radius: 15px;
    padding: 0 16px;
    font-size: 12px;
    font-weight: 600;
}
QPushButton#StoreCardInstall:hover { background: #ffffff; }
QPushButton#StoreCardInstall[installed="true"] {
    background: transparent;
    color: #7ddc8f;
    border: 1px solid rgba(125,220,143,0.40);
}
)qss");
}

} // namespace tankoban
