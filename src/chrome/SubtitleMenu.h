#pragma once
// SubtitleMenu — Harbor's bottom-right subtitle control, phase 1 (embedded only).
// Ports subtitle-menu.tsx + subtitle-menu/menu-body.tsx + delay-row.tsx +
// variant-row.tsx. Mirrors the AudioMenu Qt pattern (circular button + floating
// Tool popup), extended to Harbor's two-column shell: a left language rail
// (Off/On + language groups) and a right panel (filter tabs + embedded variant
// list + sync row). Provider search / external import / style bar are DEFERRED
// to the later subtitle-ecosystem milestone — only embedded mpv tracks are live.

#include "engine/PlayerSnapshot.h"

#include <QWidget>

class QEnterEvent;
class QMouseEvent;

class SubtitlePopup;

class SubtitleMenu : public QWidget {
    Q_OBJECT
public:
    explicit SubtitleMenu(QWidget* parent = nullptr);
    ~SubtitleMenu() override;

    void setSnapshot(const PlayerSnapshot& snap);
    void syncPopupPosition();

signals:
    void trackRequested(const QString& id);   // empty id -> Off (mpv sid=no)
    void delayRequested(double sec);
    void openChanged(bool open);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void setOpen(bool open);
    void toggleOpen();
    void showPopup();
    void hidePopup();
    QRect globalRect() const;
    bool hasSelectedSub() const;

    SubtitlePopup* m_popup = nullptr;
    PlayerSnapshot m_snapshot;
    bool m_hover = false;
    bool m_pressed = false;
    bool m_open = false;
};
