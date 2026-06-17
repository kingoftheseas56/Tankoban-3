#pragma once

#include <QLabel>

class TimeLabel : public QLabel {
    Q_OBJECT
public:
    enum class Mode { Start, End, Remaining };

    explicit TimeLabel(Mode mode, QWidget* parent = nullptr);
    void setDuration(double sec);

private slots:
    void refresh();

private:
    Mode m_mode = Mode::Start;
    double m_duration = 0.0;
};
