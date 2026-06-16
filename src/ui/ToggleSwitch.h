// Tankoban 3 — ToggleSwitch.
//
// A small Harbor-style on/off switch (gold track when on, sliding white knob). Used for
// enable/disable in the Addons installed list; reusable wherever a toggle is needed.

#pragma once

#include <QSize>
#include <QWidget>

namespace tankoban {

class ToggleSwitch : public QWidget {
    Q_OBJECT
public:
    explicit ToggleSwitch(QWidget* parent = nullptr);

    bool isOn() const { return m_on; }
    void setOn(bool on);

    QSize sizeHint() const override { return QSize(40, 22); }

signals:
    void toggled(bool on);

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;

private:
    bool m_on = true;
};

} // namespace tankoban
