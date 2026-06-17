#pragma once

#include <QWidget>

class TitleInfo : public QWidget {
    Q_OBJECT
public:
    explicit TitleInfo(QWidget* parent = nullptr);

    void setText(const QString& title, const QString& subtitle = QString());
    void setClickable(bool clickable);
    QSize sizeHint() const override;

signals:
    void clicked();

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void enterEvent(QEnterEvent*) override;
    void leaveEvent(QEvent*) override;

private:
    void drawInfoGlyph(QPainter& p, const QRectF& rect) const;

    QString m_title;
    QString m_subtitle;
    bool m_clickable = false;
    bool m_pressed = false;
    bool m_hover = false;
};
