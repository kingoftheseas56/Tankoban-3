// Tankoban 3 — PosterCard (Step 3).
//
// A portrait poster + title, the atomic catalogue unit. Loads its cover async and
// renders it FULL-QUALITY: DPR-aware (source >= card px * devicePixelRatio), smooth
// scaling, never upscaled — the Tankoban 2 blur fix, built in from day one.

#pragma once

#include <QWidget>

#include "core/MetaItem.h"

class QLabel;
class QEnterEvent;

namespace tankoban {

class PosterCard : public QWidget {
    Q_OBJECT
public:
    explicit PosterCard(const MetaItem& item, QWidget* parent = nullptr);

    static constexpr int kPosterW = 150;
    static constexpr int kPosterH = 225; // 2:3

protected:
    void enterEvent(QEnterEvent* e) override;
    void leaveEvent(QEvent* e) override;

private:
    void loadPoster(const QString& url);

    QLabel* m_poster = nullptr;
    QLabel* m_title = nullptr;
};

} // namespace tankoban
