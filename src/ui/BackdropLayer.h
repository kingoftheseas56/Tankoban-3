// Tankoban 3 - Play Picker backdrop layer (M5).
#pragma once

#include <QPixmap>
#include <QWidget>

namespace tankoban {

class BackdropLayer : public QWidget {
    Q_OBJECT
public:
    explicit BackdropLayer(QWidget* parent = nullptr);

    void setSource(const QString& imageUrl);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QString m_source;
    QPixmap m_cachedPixmap;
};

} // namespace tankoban
