// Tankoban 3 — DetailHero (Detail path).
//
// The full-bleed backdrop hero at the top of the Detail page, recreated from Harbor's
// detail.tsx hero: a wide backdrop with bottom + left art-melt gradients, the Fraunces
// title, a year/rating/runtime/genre pill row, and Play + Add-to-Watchlist buttons.

#pragma once

#include <QImage>
#include <QPixmap>
#include <QString>
#include <QStringList>
#include <QWidget>

#include "core/MetaDetail.h"
#include "core/MetaItem.h"

class QLabel;
class QPushButton;
class QHBoxLayout;

namespace tankoban {

class DetailHero : public QWidget {
    Q_OBJECT
public:
    explicit DetailHero(QWidget* parent = nullptr);

    void setPreview(const MetaItem& preview);   // immediate, from the clicked card
    void applyDetail(const MetaDetail& detail); // when /meta returns (genres, etc.)

    static constexpr int kHeroH = 600;

signals:
    void playClicked();

protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;

private:
    void setBackdropUrl(const QString& url);
    void rebuildPills();

    QString m_year, m_rating, m_runtime;
    QStringList m_genres;

    QLabel* m_title = nullptr;
    QWidget* m_pillsRow = nullptr;
    QHBoxLayout* m_pillsLayout = nullptr;
    QPushButton* m_play = nullptr;
    QPushButton* m_add = nullptr;

    QImage m_bgRaw;
    QPixmap m_bgPix;
    QString m_bgUrl;
};

} // namespace tankoban
