#pragma once

#include "engine/PlayerSnapshot.h"

#include <QWidget>

class QEnterEvent;
class QMouseEvent;

class AudioPopup;

class AudioMenu : public QWidget {
    Q_OBJECT
public:
    explicit AudioMenu(QWidget* parent = nullptr);
    ~AudioMenu() override;

    void setSnapshot(const PlayerSnapshot& snap);
    void syncPopupPosition();

signals:
    void trackRequested(const QString& id);
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

    AudioPopup* m_popup = nullptr;
    PlayerSnapshot m_snapshot;
    bool m_hover = false;
    bool m_pressed = false;
    bool m_open = false;
};
