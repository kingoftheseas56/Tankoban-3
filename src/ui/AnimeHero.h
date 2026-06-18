// Tankoban 3 — AnimeHero (Anime hero).
//
// Faithful to harbor-ref src/components/anime-hero.tsx: a full-bleed anime masthead with a
// backdrop, left/bottom gradient melt, a title plate, a year · Subtitled · genres tag row,
// a 3-line synopsis, Start Watching + Save buttons, prev/next arrows, and over-hero dots.
// 14s rotation. This is the Anime hero — NOT CinemaHero (Movies) or PeekHero (Shows).
//
// Start Watching (and a bare click) opens Detail — Harbor's AnimeHero "Start Watching"
// routes through openMeta, not the picker. Drag follows the content block + cross-fades the
// backdrop on release (the smooth CinemaHero approach). Height is driven by the host page.
// v1 trims: no TMDB logo/backdrop, no awards badge, no top-picks panel, no MAL-score chip.

#pragma once

#include <QElapsedTimer>
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
class QMouseEvent;
class QPaintEvent;
class QResizeEvent;
class QEnterEvent;

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
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void enterEvent(QEnterEvent*) override;
    void leaveEvent(QEvent*) override;

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

    QPushButton* m_prevArrow = nullptr;
    QPushButton* m_nextArrow = nullptr;

    QWidget* m_dotsRow = nullptr;
    QHBoxLayout* m_dotsLayout = nullptr;
    QVector<QPushButton*> m_dots;

    QTimer* m_autoTimer = nullptr;

    bool m_dragActive = false;
    bool m_dragMoved = false;
    int m_startX = 0;
    int m_lastX = 0;
    qint64 m_lastT = 0;
    qreal m_vel = 0.0;
    int m_contentBaseX = 0;
    QElapsedTimer m_clock;
};

} // namespace tankoban
