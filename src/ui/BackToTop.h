// Tankoban 3 — BackToTop.
//
// A floating round button that appears once a scroll area is scrolled down and glides it
// back to the top on click (Harbor components/back-to-top.tsx). Reused by the Anime route
// and the View-all grid. Created as a child of the page; floats bottom-right, never scrolls.

#pragma once

#include <QPushButton>

class QScrollArea;
class QEvent;

namespace tankoban {

class BackToTop : public QPushButton {
    Q_OBJECT
public:
    BackToTop(QScrollArea* area, QWidget* parent);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void refresh();    // show/hide based on scroll position
    void reposition(); // bottom-right inset of the parent page

    QScrollArea* m_area = nullptr;
};

} // namespace tankoban
