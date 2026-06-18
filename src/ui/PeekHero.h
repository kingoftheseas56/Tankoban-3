// Tankoban 3 — PeekHero (Shows hero).
//
// Faithful recreation of Harbor's src/components/peek-hero.tsx: a centered wide LANDSCAPE
// card carousel with side-peek neighbours (scaled 0.86 / dimmed 0.45), mouse drag/swipe
// (9.5s auto-rotate, pause on hover), and wide-bar dot pills. This is the Shows hero —
// deliberately NOT the Movies billboard (FeaturedHero/CinemaHero).
//
// Single painted widget: the track region paints the backdrop cards + bottom gradients; a
// small bottom-left overlay (title + meta + Play/Episodes) sits over the active card, and
// a dot row sits below the track. Backdrops load lazily for the active card and its two
// neighbours. With one slide it degrades to a single static card (no dots/auto/peek).

#pragma once

#include <QHash>
#include <QImage>
#include <QPixmap>
#include <QSet>
#include <QVector>
#include <QWidget>
#include <QElapsedTimer>

#include "core/MetaItem.h"

class QHBoxLayout;
class QLabel;
class QNetworkAccessManager;
class QPushButton;
class QTimer;
class QMouseEvent;
class QEnterEvent;
class QPaintEvent;
class QResizeEvent;

namespace tankoban {

class PeekHero : public QWidget {
    Q_OBJECT
public:
    explicit PeekHero(QWidget* parent = nullptr);

    void setSlides(const QVector<MetaItem>& slides);

    static constexpr int kTrackH = 440; // Harbor h-[440px]
    static constexpr int kCardH = 420;  // Harbor card h-[420px]
    static constexpr int kCardMaxW = 920;

signals:
    void openDetailRequested(const MetaItem& meta);
    void playRequested(const MetaItem& meta);

protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void enterEvent(QEnterEvent*) override;
    void leaveEvent(QEvent*) override;

private:
    int wrap(int i) const;
    int cardWidth() const;
    QRect cardRect(int offset, qreal scale, qreal dragDx) const;
    void drawCard(class QPainter& p, const MetaItem& m, const QRect& r, qreal opacity);
    void requestVisibleBgs();
    void loadBg(int index);
    void goTo(int idx);
    void next();
    void rebuildDots();
    void syncDots();
    void updateActiveContent();
    void positionChildren();

    QVector<MetaItem> m_slides;
    int m_active = 0;

    // Drag gesture (Harbor DRAG_THRESHOLD 8 / SNAP_FRACTION 0.15 / FLICK 0.45).
    bool m_dragActive = false;
    bool m_dragMoved = false;
    qreal m_dragDx = 0.0;
    int m_startX = 0;
    int m_lastX = 0;
    qint64 m_lastT = 0;
    qreal m_vel = 0.0;
    QElapsedTimer m_clock;

    QHash<QString, QImage> m_bg;       // meta id -> loaded backdrop (full-res)
    QHash<QString, QPixmap> m_pmCache; // "id|WxH" -> cover-scaled pixmap (drag-smooth blits)
    QSet<QString> m_bgRequested;
    QNetworkAccessManager* m_nam = nullptr;

    QWidget* m_overlay = nullptr; // bottom-left active content (title + meta + buttons)
    QLabel* m_title = nullptr;
    QLabel* m_meta = nullptr;
    QPushButton* m_play = nullptr;
    QPushButton* m_episodes = nullptr;

    QWidget* m_dotsRow = nullptr;
    QHBoxLayout* m_dotsLayout = nullptr;
    QVector<QPushButton*> m_dots;

    QTimer* m_autoTimer = nullptr;
};

} // namespace tankoban
