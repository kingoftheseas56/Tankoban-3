// Tankoban 3 — Sidebar (Step 2 + collapse).
//
// The left navigation rail, recreated faithfully from Harbor's src/chrome/sidebar.tsx:
// a brand header, a PRIMARY group and a COLLECTIONS group split by a hairline. Each item
// emits viewActivated(id); the active item gets the gold/elevated treatment.
//
// Collapse (Harbor's 72 ⇄ 240px): a bottom toggle flips between the full icon+label rail
// and an icon-only rail, animated with Harbor's 320ms "pull" easing. The choice persists
// across launches (QSettings), matching Harbor's sidebarCollapsed setting.

#pragma once

#include <QHash>
#include <QWidget>

class QLabel;
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
    void setCollapsed(bool collapsed, bool animate);
    void applyCollapsedVisual(bool collapsed);

    QHash<QString, QPushButton*> m_items;
    QString m_active;

    QLabel* m_brand = nullptr;
    QPushButton* m_toggle = nullptr;
    bool m_collapsed = false;

    static constexpr int kExpandedW = 240; // Harbor lg:w-60
    static constexpr int kCollapsedW = 72; // Harbor w-[72px]
};

} // namespace tankoban
