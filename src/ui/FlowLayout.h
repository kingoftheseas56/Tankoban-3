// Tankoban 3 — FlowLayout.
//
// A wrapping layout (the canonical Qt FlowLayout): lays children left-to-right and wraps
// to the next line when the row is full. Used by GridPage to recreate Harbor's responsive
// auto-fill poster grid (grid-cols-[repeat(auto-fill,minmax(150px,1fr))]) — fixed-width
// PosterCards that reflow with the window width.

#pragma once

#include <QLayout>
#include <QList>
#include <QRect>
#include <QSize>
#include <QStyle>

namespace tankoban {

class FlowLayout : public QLayout {
public:
    explicit FlowLayout(QWidget* parent, int margin = -1, int hSpacing = -1, int vSpacing = -1);
    explicit FlowLayout(int margin = -1, int hSpacing = -1, int vSpacing = -1);
    ~FlowLayout() override;

    void addItem(QLayoutItem* item) override;
    int horizontalSpacing() const;
    int verticalSpacing() const;
    Qt::Orientations expandingDirections() const override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int width) const override;
    int count() const override;
    QLayoutItem* itemAt(int index) const override;
    QSize minimumSize() const override;
    void setGeometry(const QRect& rect) override;
    QSize sizeHint() const override;
    QLayoutItem* takeAt(int index) override;

private:
    int doLayout(const QRect& rect, bool testOnly) const;
    int smartSpacing(QStyle::PixelMetric pm) const;

    QList<QLayoutItem*> m_items;
    int m_hSpace;
    int m_vSpace;
};

} // namespace tankoban
