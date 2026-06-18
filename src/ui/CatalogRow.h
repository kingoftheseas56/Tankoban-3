// Tankoban 3 — CatalogRow (Step 3 / lazy + Harbor arrows).
//
// A titled horizontal shelf of PosterCards. Lazy-loads covers by viewport, and
// scrolls via Harbor-style hover edge-arrows (the scrollbar is hidden, 1:1 Harbor).

#pragma once

#include <QVector>
#include <QWidget>

#include "core/MetaItem.h"

class QHBoxLayout;
class QLabel;
class QPushButton;
class QScrollArea;
class QShowEvent;
class QResizeEvent;
class QEnterEvent;

namespace tankoban {

class PosterCard;

class CatalogRow : public QWidget {
    Q_OBJECT
public:
    explicit CatalogRow(const QString& title, QWidget* parent = nullptr);

    void setItems(const QVector<MetaItem>& items);
    void setStatus(const QString& text);
    // Show/hide the Harbor "View all" affordance in the row header. Hidden by default;
    // a page enables it for rows that can open a full-category grid (Harbor: onViewAll).
    void setViewAllVisible(bool visible);

signals:
    void activated(const MetaItem& item);
    void viewAllRequested();

protected:
    void showEvent(QShowEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;
    void enterEvent(QEnterEvent* e) override;
    void leaveEvent(QEvent* e) override;

private:
    void updateVisible();         // lazy-load covers in/near the viewport
    void updateArrows();          // position + show/hide the hover arrows
    void scrollByPage(int dir);   // -1 left / +1 right, smooth glide

    QLabel* m_title = nullptr;
    QPushButton* m_viewAll = nullptr;
    QLabel* m_status = nullptr;
    QScrollArea* m_scroll = nullptr;
    QWidget* m_track = nullptr;
    QHBoxLayout* m_trackLayout = nullptr;
    QPushButton* m_leftArrow = nullptr;
    QPushButton* m_rightArrow = nullptr;
    QVector<PosterCard*> m_cards;
    bool m_hovered = false;
};

} // namespace tankoban
