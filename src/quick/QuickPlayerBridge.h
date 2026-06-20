#pragma once

#include "engine/PlayerSnapshot.h"

#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariant>
#include <QVariantList>
#include <QtQmlIntegration/qqmlintegration.h>

class MpvController;
struct mpv_handle;

class QuickPlayerBridge : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString status READ status NOTIFY snapshotChanged)
    Q_PROPERTY(bool playing READ playing NOTIFY snapshotChanged)
    Q_PROPERTY(double positionSec READ positionSec NOTIFY positionChanged)
    Q_PROPERTY(double durationSec READ durationSec NOTIFY snapshotChanged)
    Q_PROPERTY(double volume READ volume NOTIFY snapshotChanged)
    Q_PROPERTY(bool muted READ muted NOTIFY snapshotChanged)
    Q_PROPERTY(double rate READ rate NOTIFY snapshotChanged)
    Q_PROPERTY(bool buffering READ buffering NOTIFY snapshotChanged)
    Q_PROPERTY(QVariantList audioTracks READ audioTracks NOTIFY snapshotChanged)
    Q_PROPERTY(QVariantList subtitleTracks READ subtitleTracks NOTIFY snapshotChanged)
    Q_PROPERTY(double audioDelaySec READ audioDelaySec NOTIFY snapshotChanged)
    Q_PROPERTY(double subDelaySec READ subDelaySec NOTIFY snapshotChanged)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString subtitle READ subtitle WRITE setSubtitle NOTIFY titleChanged)

public:
    explicit QuickPlayerBridge(QObject* parent = nullptr);

    mpv_handle* handle() const;

    QString status() const;
    bool playing() const;
    double positionSec() const { return m_displayPositionSec; }
    double durationSec() const { return m_snapshot.durationSec; }
    double volume() const { return m_snapshot.volume; }
    bool muted() const { return m_snapshot.muted; }
    double rate() const { return m_snapshot.rate; }
    bool buffering() const { return m_snapshot.buffering; }
    QVariantList audioTracks() const;
    QVariantList subtitleTracks() const;
    double audioDelaySec() const { return m_snapshot.audioDelaySec; }
    double subDelaySec() const { return m_snapshot.subDelaySec; }
    QString title() const { return m_title; }
    QString subtitle() const { return m_subtitle; }

    void setTitle(const QString& title);
    void setSubtitle(const QString& subtitle);

    Q_INVOKABLE void load(const QString& url, double startSec = 0.0);
    Q_INVOKABLE void togglePlayPause();
    Q_INVOKABLE void seek(double sec);
    Q_INVOKABLE void seekStep(double delta);
    Q_INVOKABLE void setVolume(double value);
    Q_INVOKABLE void toggleMuted();
    Q_INVOKABLE void setRate(double value);
    Q_INVOKABLE void setAudioTrack(const QString& id);
    Q_INVOKABLE void setSubtitleTrack(const QString& id);   // empty id -> Off (mpv sid=no)
    Q_INVOKABLE void setAudioDelay(double sec);
    Q_INVOKABLE void setSubDelay(double sec);
    Q_INVOKABLE void setRenderReady();

signals:
    void snapshotChanged();
    void positionChanged();
    void titleChanged();

private:
    void applySnapshot(const PlayerSnapshot& snapshot);
    void maybeLoadPending();
    void tickPosition();

    MpvController* m_controller = nullptr;
    PlayerSnapshot m_snapshot;
    QTimer m_positionTimer;
    QString m_pendingUrl;
    double m_pendingStartSec = 0.0;
    double m_displayPositionSec = 0.0;
    QString m_title;
    QString m_subtitle;
    bool m_renderReady = false;
};
