// Tankoban 3 - pure, widget-free source-row logic (badges, confidence, identity,
// quality group, playability). Extracted from StremioRow so SourceRowDelegate and
// the self-test can share it. No QWidget dependency.
#pragma once

#include <QString>
#include <QStringList>

#include "core/StreamModels.h"

namespace tankoban {

enum class RowConfidence { Labeled, Unverified, Unlabeled };

// Stable per-stream identity used for model-row reuse (addon + hash/url/title).
QString streamRowKey(const ScoredStream& s);

// Addon-instance identity for the addon filter (addonUrl else addonId).
QString addonInstanceKey(const ScoredStream& s);

// "4K" / "1080p" / "720p" / "SD" bucket for the quality filter + pills.
QString qualityGroupOf(const ScoredStream& s);

// Harbor format-badge.tsx parity: confidence + the ordered chip labels.
RowConfidence qualityConfidence(const ScoredStream& s);
QStringList streamBadges(const ScoredStream& s);

// Direct URL OR torrent infoHash (torrents stream via the engine) => playable.
bool isPlayable(const ScoredStream& s);

} // namespace tankoban
