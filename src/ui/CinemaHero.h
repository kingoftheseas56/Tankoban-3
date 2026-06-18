// Tankoban 3 — CinemaHero (Movies hero).
//
// Faithful recreation of Harbor's src/components/cinema-hero.tsx: a FULL-BLEED cinematic
// masthead (78vh / min 640) with a backdrop, a left + bottom gradient melt, a "Featured
// tonight" eyebrow, a large title plate, a year · runtime · genres meta row, a 2-line
// synopsis, Play + More Info buttons, and wide-bar dots OVER the hero. This is the Movies
// hero — deliberately NOT the Home billboard (FeaturedHero) nor the Shows PeekHero.
//
// Play opens the Picker; More Info (and a bare click) opens Detail — Harbor's split actions.
// Drag follows the content block + cross-fades the backdrop on release (the smooth Home-hero
// approach, not the heavier full-image repaint). Height is driven by the host page.

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

class CinemaHero : public QWidget {
    Q_OBJECT
public:
    explicit CinemaHero(QWidget* parent = nullptr);

    void setSlides(const QVector<MetaItem>& slides);

signals:
    void playRequested(const MetaItem& meta);
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

    // Backdrop cross-fade (prev -> cur).
    QImage m_rawCur, m_rawPrev;
    QPixmap m_pxCur, m_pxPrev;
    qreal m_fade = 1.0;
    QString m_url;
    QNetworkAccessManager* m_nam = nullptr;

    // Bottom-left content block.
    QWidget* m_content = nullptr;
    QGraphicsOpacityEffect* m_contentFx = nullptr;
    QLabel* m_eyebrow = nullptr;
    QLabel* m_title = nullptr;
    QLabel* m_meta = nullptr;
    QLabel* m_desc = nullptr;
    QPushButton* m_play = nullptr;
    QPushButton* m_moreInfo = nullptr;

    // Over-hero dots.
    QWidget* m_dotsRow = nullptr;
    QHBoxLayout* m_dotsLayout = nullptr;
    QVector<QPushButton*> m_dots;

    QTimer* m_autoTimer = nullptr;

    // Drag (content-follow + cross-fade on release).
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
