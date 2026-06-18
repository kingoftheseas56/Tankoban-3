// Tankoban 3 — AnimeHero (Anime hero).
//
// Faithful to harbor-ref src/components/anime-hero.tsx: a full-bleed anime masthead with a
// backdrop, left/bottom gradient melt, a title plate, a year · Subtitled · genres tag row,
// a 3-line synopsis, Start Watching + Save buttons, prev/next arrows, and over-hero dots.
// 14s rotation. This is the Anime hero — NOT CinemaHero (Movies) or PeekHero (Shows).
//
// Start Watching opens Detail (Harbor's AnimeHero routes through openMeta, not the picker).
// Harbor has NO hero drag/flick and NO bare-click-to-open, so neither does this (1:1 until
// recreation). Navigation is arrows + dots + 14s auto-rotate, paused on hover AND when the
// route is hidden. Height is driven by the host page.
// v1 trims: no TMDB logo/backdrop, no awards badge, no top-picks panel; MAL-score chip +
// Harbor button styling land in parity increment 2b.

#pragma once

#include <QImage>
#include <QPixmap>
#include <QString>
#include <QVector>
#include <QWidget>

#include "core/MetaItem.h"

class QHBoxLayout;
class QLabel;
class QPushButton;
class QTimer;
class QNetworkAccessManager;
class QGraphicsOpacityEffect;
class QPaintEvent;
class QResizeEvent;
class QEnterEvent;
class QShowEvent;
class QHideEvent;

namespace tankoban {

class AnimeHero : public QWidget {
    Q_OBJECT
public:
    explicit AnimeHero(QWidget* parent = nullptr);

    void setSlides(const QVector<MetaItem>& slides);

signals:
    void openDetailRequested(const MetaItem& meta);

protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void enterEvent(QEnterEvent*) override;
    void leaveEvent(QEvent*) override;
    void showEvent(QShowEvent*) override;
    void hideEvent(QHideEvent*) override;

private:
    void showSlide(int i, bool animate);
    void goTo(int i);
    void next();
    void rebuildDots();
    void syncDots();
    void positionChildren();
    void setBackdrop(const QString& url, bool animate);
    QPixmap coverScaled(const QImage& src) const;
    int activeIndex() const;

    QVector<MetaItem> m_slides;
    int m_active = 0;

    QImage m_rawCur, m_rawPrev;
    QPixmap m_pxCur, m_pxPrev;
    qreal m_fade = 1.0;
    QString m_url;
    QNetworkAccessManager* m_nam = nullptr;

    QWidget* m_content = nullptr;
    QGraphicsOpacityEffect* m_contentFx = nullptr;
    QLabel* m_title = nullptr;
    QLabel* m_meta = nullptr;
    QLabel* m_desc = nullptr;
    QPushButton* m_start = nullptr;
    QPushButton* m_save = nullptr;
    QWidget* m_malChip = nullptr; // MAL logo + score, shown when the slide has a score
    QLabel* m_malScore = nullptr;

    QPushButton* m_prevArrow = nullptr;
    QPushButton* m_nextArrow = nullptr;

    QWidget* m_dotsRow = nullptr;
    QHBoxLayout* m_dotsLayout = nullptr;
    QVector<QPushButton*> m_dots;

    QTimer* m_autoTimer = nullptr;
};

} // namespace tankoban
