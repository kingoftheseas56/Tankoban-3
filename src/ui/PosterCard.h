// Tankoban 3 — PosterCard (Step 3 / lazy).
//
// A portrait poster + title. Loads its cover async, FULL-QUALITY (DPR-aware, smooth,
// never upscaled) — but only when asked via ensureLoaded(), so the row can lazy-load
// just the visible cards (Harbor's viewport trick) instead of all at once.

#pragma once

#include <QString>
#include <QWidget>

#include "core/MetaItem.h"

class QLabel;
class QEnterEvent;

namespace tankoban {

class PosterCard : public QWidget {
    Q_OBJECT
public:
    explicit PosterCard(const MetaItem& item, QWidget* parent = nullptr);

    // Fetch the cover the first time the card scrolls into view (idempotent).
    void ensureLoaded();

    static constexpr int kPosterW = 150;
    static constexpr int kPosterH = 225; // 2:3

protected:
    void enterEvent(QEnterEvent* e) override;
    void leaveEvent(QEvent* e) override;

private:
    void loadPoster(const QString& url);

    QLabel* m_poster = nullptr;
    QLabel* m_title = nullptr;
    QString m_url;
    bool m_loadRequested = false;
};

} // namespace tankoban
