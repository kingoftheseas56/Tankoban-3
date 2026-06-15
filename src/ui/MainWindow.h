// Tankoban 3 — MainWindow (Step 2).
//
// The app shell: [ Sidebar | content QStackedWidget ]. Each nav id maps to a page in
// the stack. For Step 2 the pages are placeholders ("Home", "Discover", …) so we can
// see navigation working; real pages (home rows, detail, player) slot in here in later
// steps. Frameless custom window chrome is the next sub-step; for now this is a normal
// top-level window.

#pragma once

#include <QHash>
#include <QWidget>

class QStackedWidget;

namespace tankoban {

class Sidebar;

class MainWindow : public QWidget {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    QWidget* makePlaceholder(const QString& title);

    Sidebar* m_sidebar = nullptr;
    QStackedWidget* m_content = nullptr;
    QHash<QString, int> m_pageIndex;
};

} // namespace tankoban
