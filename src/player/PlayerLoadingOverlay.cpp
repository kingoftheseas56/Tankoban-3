// Tankoban 3 — PlayerLoadingOverlay. See PlayerLoadingOverlay.h.

#include "player/PlayerLoadingOverlay.h"

#include <QFont>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QResizeEvent>
#include <QTimer>
#include <QtMath>

namespace tankoban {

namespace {
// Seconds of demuxer cache treated as "warm" for the progress bar. Real playback
// (everPlayed) usually flips before this, so the bar is a faithful fill indicator,
// not a fabricated timer — it reads mpv's actual demuxer-cache-duration.
constexpr double kReadyTargetSec = 8.0;

void drawSpinner(QPainter& p, const QPointF& c, double radius, int step, int dotCount)
{
    p.setPen(Qt::NoPen);
    for (int i = 0; i < dotCount; ++i) {
        const int age = (i - step + dotCount) % dotCount;
        const int alpha = qBound(40, 60 + (dotCount - 1 - age) * (180 / dotCount), 235);
        const double a = (i * (360.0 / dotCount) - 90.0) * M_PI / 180.0;
        p.setBrush(QColor(243, 241, 234, alpha));
        p.drawEllipse(QPointF(c.x() + qCos(a) * radius, c.y() + qSin(a) * radius), 2.1, 2.1);
    }
}
} // namespace

PlayerLoadingOverlay::PlayerLoadingOverlay(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("PlayerLoadingOverlay"));
    setAttribute(Qt::WA_NoSystemBackground);

    m_spinTimer = new QTimer(this);
    m_spinTimer->setInterval(80);
    connect(m_spinTimer, &QTimer::timeout, this, [this]() {
        m_spin = (m_spin + 1) % 12;
        update();
    });

    m_cancel = new QPushButton(QStringLiteral("Cancel"), this);
    m_cancel->setCursor(Qt::PointingHandCursor);
    m_cancel->setFixedHeight(44);
    m_cancel->setMinimumWidth(132);
    m_cancel->setStyleSheet(QStringLiteral(
        "QPushButton { color: rgba(255,255,255,0.78); background: rgba(0,0,0,0.45);"
        " border: 1px solid rgba(255,255,255,0.15); border-radius: 22px;"
        " font-family: 'Inter','Segoe UI',sans-serif; font-size: 13px; font-weight: 600;"
        " padding: 0 22px; }"
        "QPushButton:hover { color: #fff; background: rgba(0,0,0,0.62);"
        " border: 1px solid rgba(255,255,255,0.30); }"));
    connect(m_cancel, &QPushButton::clicked, this, &PlayerLoadingOverlay::cancelRequested);
    m_cancel->hide();

    hide();
}

void PlayerLoadingOverlay::setTitle(const QString& title, const QString& subtitle)
{
    m_title = title;
    m_subtitle = subtitle;
    if (m_mode != Hidden)
        update();
}

void PlayerLoadingOverlay::applySnapshot(bool everPlayed, bool buffering, double bufferedSec,
                                         double durationSec, bool ended, bool error)
{
    Q_UNUSED(durationSec);
    m_bufferedSec = bufferedSec;

    Mode next;
    if (ended || error) {
        next = Hidden;                       // host / chrome owns the ended / error surface
    } else if (!everPlayed) {
        next = WarmUp;                       // warming up — never started yet
        m_status = buffering ? QStringLiteral("Buffering")
                             : (m_local ? QStringLiteral("Preparing stream")
                                        : QStringLiteral("Connecting"));
    } else if (buffering) {
        next = Buffering;                    // real mid-playback cache stall
        m_status = QStringLiteral("Buffering");
    } else {
        next = Hidden;
    }
    setMode(next);
    if (m_mode != Hidden)
        update();
}

void PlayerLoadingOverlay::setMode(Mode mode)
{
    if (mode == m_mode) {
        // Mouse-transparency / spinner stay correct across repeated same-mode calls.
        return;
    }
    m_mode = mode;

    const bool visible = m_mode != Hidden;
    // WarmUp blocks input (true takeover + Cancel). Buffering is a light, non-blocking
    // indication so the transport chrome under it stays usable (Harbor parity).
    setAttribute(Qt::WA_TransparentForMouseEvents, m_mode == Buffering);
    m_cancel->setVisible(m_mode == WarmUp);

    if (visible) {
        if (!m_spinTimer->isActive())
            m_spinTimer->start();
        show();
        raise();
        layoutCancel();
    } else {
        m_spinTimer->stop();
        hide();
    }
}

void PlayerLoadingOverlay::resizeEvent(QResizeEvent*)
{
    layoutCancel();
}

void PlayerLoadingOverlay::layoutCancel()
{
    if (!m_cancel)
        return;
    m_cancel->move((width() - m_cancel->width()) / 2, height() - m_cancel->height() - 40);
    m_cancel->raise();
}

void PlayerLoadingOverlay::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QRectF r = rect();

    if (m_mode == Buffering) {
        // Light, unobtrusive pill near the lower third — chrome stays visible/usable.
        const QString text = m_status.isEmpty() ? QStringLiteral("Buffering") : m_status;
        QFont f(QStringLiteral("Inter"));
        f.setPixelSize(13);
        f.setWeight(QFont::DemiBold);
        p.setFont(f);
        const int textW = p.fontMetrics().horizontalAdvance(text);
        const double pillW = 28 + 14 + textW + 18;
        const double pillH = 38;
        const QRectF pill((r.width() - pillW) / 2.0, r.height() * 0.72, pillW, pillH);
        QPainterPath path;
        path.addRoundedRect(pill, pillH / 2.0, pillH / 2.0);
        p.fillPath(path, QColor(10, 11, 13, 220));
        p.setPen(QPen(QColor(255, 255, 255, 26), 1));
        p.drawPath(path);
        drawSpinner(p, QPointF(pill.left() + 22, pill.center().y()), 7.0, m_spin, 12);
        p.setPen(QColor(255, 255, 255, 210));
        p.drawText(QRectF(pill.left() + 40, pill.top(), textW + 8, pillH),
                   Qt::AlignVCenter | Qt::AlignLeft, text);
        return;
    }

    // WarmUp: full dark backdrop (Harbor's blurred-still is deferred — see limitations).
    QLinearGradient g(r.topLeft(), r.bottomLeft());
    g.setColorAt(0.0, QColor(11, 11, 13));
    g.setColorAt(1.0, QColor(5, 5, 6));
    p.fillRect(r, g);

    const double cx = r.width() / 2.0;
    double y = r.height() / 2.0 - 70.0;

    // Spinner.
    drawSpinner(p, QPointF(cx, y), 11.0, m_spin, 12);
    y += 40.0;

    // Title.
    if (!m_title.isEmpty()) {
        QFont tf(QStringLiteral("Inter"));
        tf.setPixelSize(26);
        tf.setWeight(QFont::DemiBold);
        p.setFont(tf);
        p.setPen(QColor(0xf3, 0xf1, 0xea));
        p.drawText(QRectF(0, y, r.width(), 34), Qt::AlignHCenter | Qt::AlignVCenter, m_title);
        y += 38.0;
    }

    // Subtitle (uppercase, tracked) — e.g. "S1 · E02 · Title".
    if (!m_subtitle.isEmpty()) {
        QFont sf(QStringLiteral("Inter"));
        sf.setPixelSize(12);
        sf.setWeight(QFont::DemiBold);
        sf.setLetterSpacing(QFont::AbsoluteSpacing, 3.0);
        p.setFont(sf);
        p.setPen(QColor(255, 255, 255, 178));
        p.drawText(QRectF(0, y, r.width(), 20), Qt::AlignHCenter | Qt::AlignVCenter,
                   m_subtitle.toUpper());
        y += 28.0;
    }

    // Status line.
    {
        QFont stf(QStringLiteral("Inter"));
        stf.setPixelSize(13);
        stf.setWeight(QFont::Medium);
        p.setFont(stf);
        p.setPen(QColor(255, 255, 255, 140));
        p.drawText(QRectF(0, y, r.width(), 18), Qt::AlignHCenter | Qt::AlignVCenter, m_status);
        y += 28.0;
    }

    // Cache-fill progress (real mpv demuxer-cache-duration, not a timer).
    const double pct = qBound(0.0, m_bufferedSec / kReadyTargetSec, 1.0);
    const double barW = qMin(360.0, r.width() - 96.0);
    if (barW > 40.0) {
        const QRectF track(cx - barW / 2.0, y, barW, 6.0);
        QPainterPath tp;
        tp.addRoundedRect(track, 3.0, 3.0);
        p.fillPath(tp, QColor(255, 255, 255, 30));
        if (pct > 0.0) {
            QPainterPath fp;
            fp.addRoundedRect(QRectF(track.left(), track.top(), barW * pct, 6.0), 3.0, 3.0);
            p.fillPath(fp, QColor(255, 255, 255, 216));
        }
    }
}

} // namespace tankoban
