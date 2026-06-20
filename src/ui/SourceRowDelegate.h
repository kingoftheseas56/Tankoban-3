// Tankoban 3 - paints one source row 1:1 with the old StremioRow, using QPainter.
// Variable height: the row grows to fit word-wrapped (multi-line) headline + filename
// (>= kMinRowHeight). Icons cached in QPixmapCache. Exposes play/copy sub-rects so
// SourceListView can hit-test clicks + drive hover.
#pragma once

#include <QStyledItemDelegate>
#include <QRect>
#include <QString>

namespace tankoban {

class SourceRowDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    static constexpr int kMinRowHeight = 96;   // StremioRow's floor; rows grow past it to fit content

    explicit SourceRowDelegate(QObject* parent = nullptr);

    void paint(QPainter* p, const QStyleOptionViewItem& opt, const QModelIndex& idx) const override;
    QSize sizeHint(const QStyleOptionViewItem& opt, const QModelIndex& idx) const override;

    // Geometry helpers (pure, given the row rect) for SourceListView hit-testing.
    static QRect playRect(const QRect& row);
    static QRect copyRect(const QRect& row);

    void setHoveredKey(const QString& key) { m_hoveredKey = key; }
    void setCopiedKey(const QString& key)  { m_copiedKey = key;  }  // shows the check glyph on that row

private:
    QString m_hoveredKey;
    QString m_copiedKey;
};

} // namespace tankoban
