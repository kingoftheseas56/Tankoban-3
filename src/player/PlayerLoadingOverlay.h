#pragma once
// PlayerLoadingOverlay — Qt port of Harbor's views/player/cinematic-player-loader.tsx
// (+ loader-layer.tsx). A full-stage cover over the video that:
//   * WarmUp   — shows until playback genuinely starts (real duration + progress),
//                with title/subtitle, a status line ("Preparing stream"/"Buffering"/
//                "Connecting"), a real cache-fill progress bar, a spinner, and Cancel.
//   * Buffering — after playback started, a light non-blocking "Buffering" pill when
//                mpv reports a real cache stall (paused-for-cache). Chrome stays usable.
//   * Hidden    — once playing and not buffering.
// Driven entirely by real PlayerSnapshot state (no fake timers); cache fill comes from
// mpv demuxer-cache-duration. Mirrors Harbor's everPlayed gate (use-ever-played.ts).
#include <QString>
#include <QWidget>

class QPushButton;
class QTimer;

namespace tankoban {

class PlayerLoadingOverlay : public QWidget {
    Q_OBJECT
public:
    explicit PlayerLoadingOverlay(QWidget* parent = nullptr);

    void setTitle(const QString& title, const QString& subtitle);
    // local: stream served by our in-process engine / local proxy (torrent) — picks
    // "Preparing stream"; otherwise a direct remote URL — picks "Connecting".
    void setLocalEngine(bool local) { m_local = local; }

    // Recompute the mode from the latest snapshot. everPlayed is owned by PlayerView
    // (sticky once real progress is seen). Safe to call on every snapshot.
    void applySnapshot(bool everPlayed, bool buffering, double bufferedSec,
                       double durationSec, bool ended, bool error);

signals:
    void cancelRequested();

protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;

private:
    enum Mode { Hidden, WarmUp, Buffering };
    void setMode(Mode mode);
    void layoutCancel();

    Mode m_mode = Hidden;
    QString m_title, m_subtitle, m_status;
    double m_bufferedSec = 0.0;
    bool m_local = true;
    int m_spin = 0;
    QTimer* m_spinTimer = nullptr;
    QPushButton* m_cancel = nullptr;
};

} // namespace tankoban
