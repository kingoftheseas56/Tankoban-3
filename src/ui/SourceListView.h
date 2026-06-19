// Tankoban 3 - QListView that drives delegate hover + maps clicks to the play/copy
// sub-rects (the rows are painted, not widgets). Emits stream-level signals.
#pragma once

#include <QListView>

namespace tankoban {

class SourceRowDelegate;

class SourceListView : public QListView {
    Q_OBJECT
public:
    explicit SourceListView(QWidget* parent = nullptr);

signals:
    void playActivated(const QModelIndex& index);
    void copyActivated(const QModelIndex& index);

protected:
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void leaveEvent(QEvent* e) override;

private:
    SourceRowDelegate* rowDelegate() const;
};

} // namespace tankoban
