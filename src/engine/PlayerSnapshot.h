#pragma once
// PlayerSnapshot — the engine's full observable state, emitted on every change.
// Ports Harbor's bridge.ts PlayerSnapshot/TrackInfo 1:1 (spec §4.3).
#include <QString>
#include <QVector>

struct TrackInfo {
    QString id, label, lang;
    enum Kind { Audio, Subtitle } kind = Audio;
    bool selected = false;
    QString codec, channels;
    int channelCount = 0;
    QString title;
    bool external = false;
    QString externalFilename;
    bool forced = false, isDefault = false, hearingImpaired = false;
};

struct Chapter { QString title; double startSec = 0.0; };

struct PlayerSnapshot {
    enum Status { Idle, Loading, Ready, Playing, Paused, Ended, Error } status = Idle;
    double positionSec = 0, durationSec = 0, bufferedSec = 0; // bufferedSec dead (§0.1 #3)
    bool   buffering = false;                                  // dead (never written)
    double volume = 1.0;        // 0..6 (= mpv volume/100)
    bool   muted = false;
    double rate = 1.0;          // optimistic; mpv `speed` is not observed
    QVector<TrackInfo> audioTracks, subtitleTracks;
    QVector<Chapter>   chapters;
    double subDelaySec = 0, audioDelaySec = 0;
    bool   audioNormalize = false;
    int    videoWidth = 0, videoHeight = 0;
    QString errorMessage;
    enum ErrorCode { None, Decode, Codec, Network, Source, Unknown } errorCode = None;
    bool   noAudio = false;     // dead (never written by the mpv path, §0.2)
};
