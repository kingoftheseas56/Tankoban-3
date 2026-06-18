// Tankoban 3 — PosterCard (Step 3 / lazy).
//
// A portrait poster + title. Loads its cover async, FULL-QUALITY (DPR-aware, smooth,
// never upscaled) — but only when asked via ensureLoaded(), so the row can lazy-load
// just the visible cards (Harbor's viewport trick) instead of all at once. Clicking
// the card emits activated() with its full MetaItem (→ Detail).

#pragma once

#include <QImage>
#include <QString>
#include <QWidget>

#include "core/MetaItem.h"

class QLabel;
class QEnterEvent;
class QMouseEvent;
class QPropertyAnimation;
class QGraphicsDropShadowEffect;

namespace tankoban {

class PosterCard : public QWidget {
    Q_OBJECT
public:
    explicit PosterCard(const MetaItem& item, QWidget* parent = nullptr);

    // Fetch the cover the first time the card scrolls into view (idempotent).
    void ensureLoaded();

    // Grid uses this to make cards fill the column (Harbor minmax(150px,1fr)). Rows leave it
    // at the default. Re-crops the retained source so resizing stays crisp.
    void setCardWidth(int width);

    static constexpr int kPosterW = 144;  // Harbor default min card width
    static constexpr int kPosterH = 216;  // 2:3
    static constexpr int kHoverLift = 8;  // Harbor group-hover:-translate-y-2

signals:
    void activated(const MetaItem& item);

protected:
    void enterEvent(QEnterEvent* e) override;
    void leaveEvent(QEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;

private:
    void loadPoster(const QString& url);
    void applyPoster(); // crop the retained source image to the current card size

    QLabel* m_poster = nullptr;
    QLabel* m_title = nullptr;
    QWidget* m_badge = nullptr; // MAL score badge (repositioned on resize)
    QPropertyAnimation* m_liftAnim = nullptr;
    QGraphicsDropShadowEffect* m_shadow = nullptr; // soft drop shadow, enabled only on hover
    QString m_url;
    QImage m_srcImage;          // retained source cover for crisp re-crop on width change
    int m_cardW = kPosterW;
    int m_cardH = kPosterH;
    MetaItem m_item;
    bool m_loadRequested = false;
};

} // namespace tankoban
