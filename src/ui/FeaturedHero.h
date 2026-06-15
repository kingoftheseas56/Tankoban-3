// Tankoban 3 — FeaturedHero (Step 3b).
//
// Harbor's featured hero carousel: the large rotating billboard at the top of Home.
// Faithful to harbor-ref src/components/hero-carousel.tsx + hero.tsx:
//   - a wide backdrop with the art-melt scrims (left + bottom),
//   - a "#N in Movies/TV Today" rank pill,
//   - the Fraunces title, a 3-line synopsis, a Year / IMDb / Runtime stat line,
//   - Play + Add-to-Watchlist buttons,
//   - 13s auto-advance that pauses on hover, and the wide-pill dots underneath.
// The slide change is an opacity cross-fade (Harbor's dominant transition).

#pragma once

#include <QString>
#include <QVector>
#include <QWidget>

#include "core/MetaItem.h"

class QHBoxLayout;
class QPushButton;
class QTimer;

namespace tankoban {

// One hero slide: a featured title plus its rank ("#1 in Movies Today").
struct HeroSlide {
    MetaItem meta;
    int position = 1;   // 1-based, for "#N …"
    QString rankLabel;  // "Movies" | "TV"
};

class HeroPanel; // painted backdrop + scrims + content (internal, defined in .cpp)

class FeaturedHero : public QWidget {
    Q_OBJECT
public:
    explicit FeaturedHero(QWidget* parent = nullptr);

    // Build the carousel from a (already deduped, capped) list of slides.
    void setSlides(const QVector<HeroSlide>& slides);

    static constexpr int kHeroH = 560; // Harbor: h-[560px]

signals:
    void openDetailRequested(const MetaItem& meta);

private:
    void showSlide(int index, bool animate);
    void goTo(int index);
    void next();
    void rebuildDots();
    void syncDots();

    QVector<HeroSlide> m_slides;
    int m_active = 0;

    HeroPanel* m_panel = nullptr;
    QWidget* m_dotsRow = nullptr;
    QHBoxLayout* m_dotsLayout = nullptr;
    QVector<QPushButton*> m_dots;
    QTimer* m_autoTimer = nullptr;
};

} // namespace tankoban
