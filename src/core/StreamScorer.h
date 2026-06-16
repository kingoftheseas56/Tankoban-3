// Tankoban 3 - StreamScorer (Slice 5).
// Scores and ranks parsed streams for the Play Picker.
// Recreated from Harbor's src/lib/streams/scoring/ subsystem.
#pragma once

#include "core/StreamModels.h"

namespace tankoban {

struct ScoreOptions {
    QStringList activeDebrids;       // DEFERRED - pass empty in v1.
    QStringList preferredLanguages;  // Empty means language scoring no-ops.
    int bandwidthMbps = 0;           // 0 means bitrate budget no-ops.
    QString releaseDate;             // ISO date, empty when unavailable.
    QString mediaKind;               // "movie" or "series".
    int runtimeMinutes = 0;          // 0 when unavailable.
    bool inTheaters = false;
    bool preferSingleAudioTrack = false;
    QString preferAddonId;
    bool respectAddonOrder = false;
};

struct CorpusStats {
    double daysSinceRelease = -1.0; // -1 = null/unknown.
    double trustedTrackedFraction = 0.0;
    double theaterCaptureFraction = 0.0;
    double webishFraction = 0.0;
    int trustedTrackedCount = 0;
};

class StreamScorer {
public:
    static ScoredStream scoreStream(const ParsedStream& stream,
                                    const ScoreOptions& opts = ScoreOptions(),
                                    const CorpusStats* corpus = nullptr);

    static RankedPicker rankAndPick(const QVector<ScoredStream>& streams,
                                    const QStringList& activeDebrids = QStringList(),
                                    bool preferAac = false,
                                    bool respectAddonOrder = false);

    static CorpusStats computeCorpusStats(const QVector<ParsedStream>& streams,
                                          const ScoreOptions& opts = ScoreOptions());

    StreamScorer() = delete;
};

} // namespace tankoban
