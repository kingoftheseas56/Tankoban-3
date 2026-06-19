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
    QPixmap m_cachedPixmap;   // source image at native resolution
    // Cache of the scaled+positioned backdrop so paintEvent doesn't re-run the
    // expensive SmoothTransformation scale on every repaint (scrolling the translucent
    // picker forces this layer to repaint each frame -> per-frame full-image rescale =
    // the rough-scroll / hang). Rebuilt only when the source or widget size changes.
    QPixmap m_scaledCache;
    QSize m_scaledForSize;
};

} // namespace tankoban
