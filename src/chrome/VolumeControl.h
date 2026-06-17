#pragma once

#include <QWidget>

struct PlayerSnapshot;

class VolumeControl : public QWidget {
    Q_OBJECT
public:
    explicit VolumeControl(QWidget* parent = nullptr);

    void setSnapshot(const PlayerSnapshot& snap);

signals:
    void volumeRequested(double value0to6);
    void muteRequested(bool muted);

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void leaveEvent(QEvent*) override;

private:
    enum class HitRegion { None, Mute, Track };

    QRect muteRect() const;
    QRect trackRect() const;
    QRect valueRect() const;
    HitRegion hitRegion(const QPoint& pos) const;
    void setFromTrackX(int x);
    void drawMuteGlyph(QPainter& p, const QRectF& rect, bool muted, const QColor& color) const;

    double m_volume = 1.0;
    bool m_muted = false;
    bool m_dragging = false;
    HitRegion m_pressed = HitRegion::None;
    HitRegion m_hover = HitRegion::None;
};
