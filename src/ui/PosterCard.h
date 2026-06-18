// Tankoban 3 — PosterCard (Step 3 / lazy).
//
// A portrait poster + title. Loads its cover async, FULL-QUALITY (DPR-aware, smooth,
// never upscaled) — but only when asked via ensureLoaded(), so the row can lazy-load
// just the visible cards (Harbor's viewport trick) instead of all at once. Clicking
// the card emits activated() with its full MetaItem (→ Detail).

#pragma once

#include <QString>
#include <QWidget>

#include "core/MetaItem.h"

class QLabel;
class QEnterEvent;
class QMouseEvent;
class QPropertyAnimation;

namespace tankoban {

class PosterCard : public QWidget {
    Q_OBJECT
public:
    explicit PosterCard(const MetaItem& item, QWidget* parent = nullptr);

    // Fetch the cover the first time the card scrolls into view (idempotent).
    void ensureLoaded();

    static constexpr int kPosterW = 150;
    static constexpr int kPosterH = 225; // 2:3
    static constexpr int kHoverLift = 8; // Harbor group-hover:-translate-y-2

signals:
    void activated(const MetaItem& item);

protected:
    void enterEvent(QEnterEvent* e) override;
    void leaveEvent(QEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;

private:
    void loadPoster(const QString& url);

    QLabel* m_poster = nullptr;
    QLabel* m_title = nullptr;
    QPropertyAnimation* m_liftAnim = nullptr;
    QString m_url;
    MetaItem m_item;
    bool m_loadRequested = false;
};

} // namespace tankoban
