#pragma once
// SeekBar — Harbor's seek bar 1:1 (spec §6): 48px hit row, 6px gold bar (+2 on
// hover/scrub), 16px gold dot (+4 scrubbing), click+drag is one gesture that
// commits the seek on RELEASE; an optimistic `pending` value holds the visual
// until the real position catches up. Driven by PlaybackClock.
#include <QWidget>
#include <optional>

class SeekBar : public QWidget {
    Q_OBJECT
public:
    explicit SeekBar(QWidget* parent = nullptr);
    void setDuration(double sec);

signals:
    void seekRequested(double sec);     // commit-on-release
    void hoveringChanged(bool hovering);// chrome stays visible while hovering (§9.2)

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void leaveEvent(QEvent*) override;

private slots:
    void onClock();

private:
    double timeAt(int x) const;
    double m_dur = 1.0;
    std::optional<double> m_hover, m_scrub, m_pending;
    bool m_dragging = false;
};
