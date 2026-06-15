// Tankoban 3 — Sidebar (Step 2).
//
// The left navigation rail, recreated faithfully from Harbor's src/chrome/sidebar.tsx:
// a brand wordmark header, a PRIMARY nav group and a COLLECTIONS group separated by a
// hairline divider. Each item emits viewActivated(id); the active item gets the gold/
// elevated treatment via the QSS [active="true"] rule. Collapse + icons come in a
// later sub-step (Harbor's 72/240 collapse); this is the expanded 240px rail.

#pragma once

#include <QHash>
#include <QWidget>

class QPushButton;
class QVBoxLayout;

namespace tankoban {

class Sidebar : public QWidget {
    Q_OBJECT
public:
    explicit Sidebar(QWidget* parent = nullptr);

    // Highlight the nav item with this id (and clear the previous one).
    void setActive(const QString& id);

signals:
    void viewActivated(const QString& id);

private:
    void addNavItem(QVBoxLayout* group, const QString& id, const QString& label);

    QHash<QString, QPushButton*> m_items;
    QString m_active;
};

} // namespace tankoban
