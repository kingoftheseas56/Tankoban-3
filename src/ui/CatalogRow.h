// Tankoban 3 — CatalogRow (Step 3).
//
// A titled horizontal shelf of PosterCards — Harbor's catalogue "row". Step 3 cut:
// plain horizontal scroll (the drag-physics + edge arrows are polish later).

#pragma once

#include <QVector>
#include <QWidget>

#include "core/MetaItem.h"

class QHBoxLayout;
class QLabel;

namespace tankoban {

class CatalogRow : public QWidget {
    Q_OBJECT
public:
    explicit CatalogRow(const QString& title, QWidget* parent = nullptr);

    void setItems(const QVector<MetaItem>& items);
    void setStatus(const QString& text);

private:
    QLabel* m_title = nullptr;
    QLabel* m_status = nullptr;
    QWidget* m_track = nullptr;
    QHBoxLayout* m_trackLayout = nullptr;
};

} // namespace tankoban
