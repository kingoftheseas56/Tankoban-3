// Tankoban 3 — CatalogRow (Step 3 / lazy).
//
// A titled horizontal shelf of PosterCards. Lazy-loads covers by viewport — only the
// cards in/near view fetch (Harbor's IntersectionObserver trick), more as you scroll.

#pragma once

#include <QVector>
#include <QWidget>

#include "core/MetaItem.h"

class QHBoxLayout;
class QLabel;
class QScrollArea;
class QShowEvent;
class QResizeEvent;

namespace tankoban {

class PosterCard;

class CatalogRow : public QWidget {
    Q_OBJECT
public:
    explicit CatalogRow(const QString& title, QWidget* parent = nullptr);

    void setItems(const QVector<MetaItem>& items);
    void setStatus(const QString& text);

protected:
    void showEvent(QShowEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;

private:
    void updateVisible(); // load covers for cards in/near the viewport

    QLabel* m_title = nullptr;
    QLabel* m_status = nullptr;
    QScrollArea* m_scroll = nullptr;
    QWidget* m_track = nullptr;
    QHBoxLayout* m_trackLayout = nullptr;
    QVector<PosterCard*> m_cards;
};

} // namespace tankoban
