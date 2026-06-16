#include "core/StreamScorer.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include <QDate>
#include <QRegularExpression>
#include <QSet>

namespace tankoban {

namespace {

constexpr int trackingMinSeeders = 30;

const QRegularExpression trustedAddonRx(QStringLiteral("mediafusion|comet|easynews|torrentio"),
                                        QRegularExpression::CaseInsensitiveOption);
const QRegularExpression strongAddonRx(QStringLiteral("mediafusion|comet"),
                                       QRegularExpression::CaseInsensitiveOption);
const QRegularExpression camMarkerRx(
    QStringLiteral(R"re(\b(?:cam|hdcam|hd[\s._-]?cam|tsrip|telesync|hdts|hd[\s._-]?ts|telecine|hd[\s._-]?tc|hc[\s._-]?hdrip|hc[\s._-]?cam|new[\s._-]?cam|cleancam|hqcam)\b)re"),
    QRegularExpression::CaseInsensitiveOption);

const QSet<QString> trustedGroups = {
    QStringLiteral("FRDS"), QStringLiteral("FRAMESTOR"), QStringLiteral("FORM"),
    QStringLiteral("EVO"), QStringLiteral("RARBG"), QStringLiteral("ETHEL"),
    QStringLiteral("FLUX"), QStringLiteral("QXR"), QStringLiteral("MEGUSTA"),
    QStringLiteral("ION10"), QStringLiteral("PSA"), QStringLiteral("AMIABLE"),
    QStringLiteral("GALAXYRG"), QStringLiteral("WEBDV"), QStringLiteral("RZEROX"),
    QStringLiteral("SIC"), QStringLiteral("TGX"), QStringLiteral("NTB"),
    QStringLiteral("NTG"), QStringLiteral("TEPES"), QStringLiteral("GECKOS"),
    QStringLiteral("SUCCESSFULCRAB"), QStringLiteral("SUBSPLEASE"), QStringLiteral("ERAI"),
    QStringLiteral("ERAIRAWS"), QStringLiteral("JUDAS"), QStringLiteral("ASW"),
    QStringLiteral("EMBER"), QStringLiteral("ANE"), QStringLiteral("CLEO"),
    QStringLiteral("BEATRICERAWS"), QStringLiteral("AKIHITO"), QStringLiteral("VODES"),
    QStringLiteral("NANDESUKA"), QStringLiteral("SMOL"), QStringLiteral("TENRAISENSEI"),
    QStringLiteral("GST"), QStringLiteral("ANIMEKAIZOKU"), QStringLiteral("REINFORCE"),
    QStringLiteral("RAWS"), QStringLiteral("OZR"), QStringLiteral("PURGATORY"),
    QStringLiteral("SHK"), QStringLiteral("KOTUWA"), QStringLiteral("KIRION"),
    QStringLiteral("COMMIE"), QStringLiteral("DAMEDESUYO"), QStringLiteral("MTBB"),
    QStringLiteral("GJM"), QStringLiteral("SOFCJ"),
};

const QSet<QString> lossyTrustedGroups = {
    QStringLiteral("YTS"),
    QStringLiteral("YIFY"),
    QStringLiteral("YTSAG"),
    QStringLiteral("YTS-AG"),
};

QString resolutionString(Resolution resolution)
{
    switch (resolution) {
    case Resolution::FourK:
        return QStringLiteral("4K");
    case Resolution::FullHD:
        return QStringLiteral("1080p");
    case Resolution::HD:
        return QStringLiteral("720p");
    case Resolution::SD_480:
        return QStringLiteral("480p");
    case Resolution::SD:
    case Resolution::None:
        return QStringLiteral("SD");
    }
    return QStringLiteral("SD");
}

QString hdrFormatString(HdrFormat hdr)
{
    switch (hdr) {
    case HdrFormat::DV:
        return QStringLiteral("DV");
    case HdrFormat::DV_HDR10:
        return QStringLiteral("DV+HDR10");
    case HdrFormat::HDR10Plus:
        return QStringLiteral("HDR10+");
    case HdrFormat::HDR10:
        return QStringLiteral("HDR10");
    case HdrFormat::HLG:
        return QStringLiteral("HLG");
    case HdrFormat::None:
        return QString();
    }
    return QString();
}

bool isTheaterCapture(Source source)
{
    return source == Source::CAM || source == Source::TS || source == Source::HDTS
        || source == Source::TC;
}

bool isCachedOnActive(const ParsedStream&, const QStringList&)
{
    // Debrid slug mapping and cached-key plumbing are deferred for v1.
    return false;
}

bool rankPlayable(const ScoredStream& stream, const QStringList&)
{
    // Harbor treats direct URLs or active-debrid cache as rank-pickable.
    // M4 keeps active debrid deferred, so direct URL is the only active path.
    return !stream.url.isEmpty();
}

bool isTrustedGroup(const QString& normalized)
{
    return !normalized.isEmpty() && trustedGroups.contains(normalized);
}

ScoreReason resolutionPoints(const ParsedStream& stream)
{
    if (stream.resolution == Resolution::FourK)
        return {QStringLiteral("4K"), 25};
    if (stream.resolution == Resolution::FullHD)
        return {QStringLiteral("1080p"), 20};
    if (stream.resolution == Resolution::HD)
        return {QStringLiteral("720p"), 8};
    if (stream.resolution == Resolution::SD_480)
        return {QStringLiteral("480p"), 2};
    return {QStringLiteral("SD"), 0};
}

Tier tierOf(const ParsedStream& stream)
{
    if (isTheaterCapture(stream.source) || stream.source == Source::SCR)
        return Tier::ROUGH;

    if (stream.resolution == Resolution::FourK) {
        if (stream.hdrFormat == HdrFormat::DV || stream.hdrFormat == HdrFormat::DV_HDR10)
            return Tier::FourK_DV;
        if (stream.hdrFormat != HdrFormat::None)
            return Tier::FourK_HDR;
        return Tier::FourK;
    }

    if (stream.resolution == Resolution::FullHD) {
        if (stream.hdrFormat != HdrFormat::None)
            return Tier::FullHD_HDR;
        return Tier::FullHD;
    }

    if (stream.resolution == Resolution::HD)
        return Tier::HD;
    return Tier::SD;
}

ScoreReason audioPoints(const ParsedStream& stream)
{
    if (stream.audio.codec == AudioCodec::Atmos)
        return {QStringLiteral("Atmos"), 3};
    if (stream.audio.codec == AudioCodec::TrueHD)
        return {QStringLiteral("TrueHD"), 2};
    if (stream.audio.codec == AudioCodec::DTS_HD_MA)
        return {QStringLiteral("DTS-HD MA"), 2};
    if (stream.audio.codec == AudioCodec::DDPlus)
        return {QStringLiteral("DD+"), 1};
    return {QStringLiteral("audio"), 0};
}

int playabilityPenalty(const ParsedStream& stream)
{
    int penalty = 0;
    if (stream.audio.codec == AudioCodec::DTS || stream.audio.codec == AudioCodec::DTS_HD_MA)
        penalty -= 6;
    if (stream.audio.codec == AudioCodec::TrueHD)
        penalty -= 4;
    if (stream.container == Container::MKV
        && (stream.audio.codec == AudioCodec::DTS || stream.audio.codec == AudioCodec::TrueHD))
        penalty -= 3;
    if (stream.container == Container::AVI || stream.container == Container::WMV)
        penalty -= 8;
    if (stream.codec == Codec::AV1)
        penalty -= 2;
    return penalty;
}

ScoreReason trustedAddonPoints(const ParsedStream& stream)
{
    if (strongAddonRx.match(stream.addonName).hasMatch())
        return {QStringLiteral("strong-addon"), 8};
    if (trustedAddonRx.match(stream.addonName).hasMatch())
        return {QStringLiteral("trusted-addon"), 4};
    return {QStringLiteral("addon-neutral"), 0};
}

qint64 signedDaysSinceDate(const QString& isoDate)
{
    if (isoDate.isEmpty())
        return std::numeric_limits<qint64>::min();
    const QDate date = QDate::fromString(isoDate.left(10), Qt::ISODate);
    if (!date.isValid())
        return std::numeric_limits<qint64>::min();
    return date.daysTo(QDate::currentDate());
}

qint64 absoluteDaysSinceDate(const QString& isoDate)
{
    const qint64 days = signedDaysSinceDate(isoDate);
    if (days == std::numeric_limits<qint64>::min())
        return -1;
    return qAbs(days);
}

qint64 expectedMinSizeBytes(Resolution resolution, int runtimeMin)
{
    if (runtimeMin <= 0)
        return 0;

    int mbPerMin = 0;
    switch (resolution) {
    case Resolution::FourK:
        mbPerMin = 60;
        break;
    case Resolution::FullHD:
        mbPerMin = 18;
        break;
    case Resolution::HD:
        mbPerMin = 8;
        break;
    case Resolution::SD_480:
        mbPerMin = 3;
        break;
    case Resolution::SD:
        mbPerMin = 2;
        break;
    case Resolution::None:
        return 0;
    }

    return qint64(mbPerMin) * runtimeMin * 1024 * 1024;
}

ScoreReason bitrateBudgetPenalty(const ParsedStream& stream, const ScoreOptions& opts, bool cached)
{
    const int budget = opts.bandwidthMbps;
    if (budget <= 0)
        return {QStringLiteral("bitrate-ok"), 0};

    const double headroom = budget * 0.8;
    if (stream.size > 0 && opts.runtimeMinutes > 0) {
        const double required
            = (double(stream.size) * 8.0) / (double(opts.runtimeMinutes) * 60.0) / 1000000.0;
        if (required > budget * 1.1) {
            const int severity = required > budget * 1.5 ? -120 : -45;
            return {QStringLiteral("bitrate-exceeds-budget:%1>%2Mbps")
                        .arg(QString::number(required, 'f', 0))
                        .arg(QString::number(double(budget), 'f', 0)),
                    cached ? severity + 10 : severity};
        }
        if (required > headroom) {
            return {QStringLiteral("bitrate-tight:%1/%2Mbps")
                        .arg(QString::number(required, 'f', 0))
                        .arg(QString::number(double(budget), 'f', 0)),
                    -12};
        }
    }

    if (stream.resolution == Resolution::FourK && budget < 25)
        return {QStringLiteral("low-bandwidth-4k"), cached ? -30 : -60};
    if (stream.resolution == Resolution::FullHD && budget < 8)
        return {QStringLiteral("low-bandwidth-1080p"), cached ? -20 : -45};
    return {QStringLiteral("bitrate-ok"), 0};
}

int camInFilenamePenalty(const ParsedStream& stream)
{
    if (isTheaterCapture(stream.source))
        return 0;
    if (stream.resolution != Resolution::FullHD && stream.resolution != Resolution::FourK)
        return 0;

    const QString haystack = QStringList{stream.name,
                                         stream.title,
                                         stream.filename,
                                         stream.fileName,
                                         stream.description}
                                 .join(QStringLiteral(" \n "));
    if (!camMarkerRx.match(haystack).hasMatch())
        return 0;
    return stream.resolution == Resolution::FourK ? -200 : -100;
}

int sizeMislabelPenalty(const ParsedStream& stream, qint64 expectedMin)
{
    if (stream.size <= 0 || expectedMin <= 0)
        return 0;
    if (isTheaterCapture(stream.source))
        return 0;
    if (!stream.releaseGroupNormalized.isEmpty()
        && lossyTrustedGroups.contains(stream.releaseGroupNormalized.toUpper()))
        return 0;
    if (stream.size >= expectedMin)
        return 0;

    const double ratio = double(stream.size) / double(expectedMin);
    if (ratio < 0.25)
        return -120;
    if (ratio < 0.5)
        return -60;
    if (ratio < 0.75)
        return -20;
    return 0;
}

ScoreReason impossiblySmallMoviePenalty(const ParsedStream& stream, const ScoreOptions& opts)
{
    if (opts.mediaKind == QStringLiteral("series"))
        return {QStringLiteral("tiny-skip-series"), 0};
    if (stream.size <= 0)
        return {QStringLiteral("tiny-skip-no-size"), 0};
    if (opts.releaseDate.isEmpty())
        return {QStringLiteral("tiny-skip-no-date"), 0};

    const qint64 days = signedDaysSinceDate(opts.releaseDate);
    if (days == std::numeric_limits<qint64>::min())
        return {QStringLiteral("tiny-skip-bad-date"), 0};
    if (days >= 90)
        return {QStringLiteral("tiny-skip-mature"), 0};
    if (isTheaterCapture(stream.source))
        return {QStringLiteral("tiny-skip-theater"), 0};

    const double sizeMB = double(stream.size) / double(1024 * 1024);
    if (sizeMB < 250.0) {
        return {QStringLiteral("new-release-virus-%1mb").arg(qRound(sizeMB)), -250};
    }

    const double runtimeFloor = opts.runtimeMinutes > 0 ? opts.runtimeMinutes * 5.0 : 0.0;
    const double floor = std::max(500.0, runtimeFloor);
    if (sizeMB < floor) {
        return {QStringLiteral("new-release-no-label-%1mb").arg(qRound(sizeMB)), -200};
    }
    return {QStringLiteral("tiny-ok"), 0};
}

ScoreReason undersizedNewReleasePenalty(const ParsedStream& stream, const ScoreOptions& opts)
{
    if (opts.mediaKind == QStringLiteral("series"))
        return {QStringLiteral("undersized-skip-series"), 0};
    if (opts.releaseDate.isEmpty() || stream.size <= 0)
        return {QStringLiteral("undersized-skip-no-data"), 0};

    const qint64 days = signedDaysSinceDate(opts.releaseDate);
    if (days == std::numeric_limits<qint64>::min())
        return {QStringLiteral("undersized-skip-bad-date"), 0};
    if (days >= 90)
        return {QStringLiteral("undersized-skip-mature"), 0};
    if (isTheaterCapture(stream.source))
        return {QStringLiteral("undersized-skip-theater"), 0};

    const double sizeGB = double(stream.size) / std::pow(1024.0, 3);
    if (stream.resolution == Resolution::FourK && sizeGB < 6.0)
        return {QStringLiteral("4k-undersized-%1gb").arg(QString::number(sizeGB, 'f', 1)), -250};
    if (stream.resolution == Resolution::FullHD && sizeGB < 1.5)
        return {QStringLiteral("1080p-undersized-%1gb").arg(QString::number(sizeGB, 'f', 1)), -200};
    if (stream.resolution == Resolution::HD && sizeGB < 0.6)
        return {QStringLiteral("720p-undersized-%1gb").arg(QString::number(sizeGB, 'f', 1)), -80};
    return {QStringLiteral("undersized-ok"), 0};
}

ScoreReason freshTheatricalAdjust(const ParsedStream& stream,
                                  const ScoreOptions& opts,
                                  bool hasValidSize,
                                  const CorpusStats* corpus)
{
    if (opts.mediaKind == QStringLiteral("series"))
        return {QStringLiteral("fresh-skip-series"), 0};
    if (opts.releaseDate.isEmpty())
        return {QStringLiteral("fresh-skip-no-date"), 0};

    const qint64 days = signedDaysSinceDate(opts.releaseDate);
    if (days == std::numeric_limits<qint64>::min())
        return {QStringLiteral("fresh-skip-bad-date"), 0};
    if (days >= 30)
        return {QStringLiteral("fresh-skip-mature"), 0};

    const bool theaterCapture = isTheaterCapture(stream.source);
    const bool remuxOrBluray = stream.source == Source::BluRay || stream.remux;
    const bool claimsHighQuality = stream.source == Source::WEB_DL || stream.source == Source::WEBRip
        || remuxOrBluray || stream.resolution == Resolution::FullHD
        || stream.resolution == Resolution::FourK;
    const bool theaterDominated = corpus && corpus->trustedTrackedCount >= 4
        && corpus->theaterCaptureFraction >= 0.4
        && corpus->theaterCaptureFraction > corpus->webishFraction;

    if (theaterCapture) {
        if (theaterDominated) {
            const int sourceOffset = stream.source == Source::CAM
                ? 95
                : (stream.source == Source::TS || stream.source == Source::HDTS ? 75 : 65);
            return {QStringLiteral("fresh-theater-cinema-window"), sourceOffset};
        }
        if (days < 14)
            return {QStringLiteral("fresh-theater-mild-boost"), 25};
        return {QStringLiteral("fresh-theater-neutral"), 0};
    }

    if (!claimsHighQuality)
        return {QStringLiteral("fresh-low-quality-noise"), 0};

    if (theaterDominated) {
        if (remuxOrBluray)
            return {QStringLiteral("fresh-fake-remux"), -200};
        if (days < 0)
            return {QStringLiteral("fresh-fake-prerelease"), -160};
        if (days < 14)
            return {QStringLiteral("fresh-fake-prebluray"), -90};
        return {QStringLiteral("fresh-fake-soft"), -45};
    }

    if (remuxOrBluray && days < 14)
        return {QStringLiteral("fresh-prebluray-suspect"), -55};
    if (days < 0 && !hasValidSize)
        return {QStringLiteral("fresh-prerelease-soft"), -35};
    return {QStringLiteral("fresh-soft-flag"), -10};
}

bool languageMatches(const QString& language, const QString& preferred)
{
    const QString l = language.toLower();
    const QString p = preferred.toLower();
    return l == p || l.startsWith(p);
}

void addReason(ScoredStream& out, const QString& signal, int delta)
{
    out.score += delta;
    out.reasons.push_back({signal, delta});
}

void addReason(ScoredStream& out, const ScoreReason& reason)
{
    addReason(out, reason.signal, reason.delta);
}

bool isTrackedForCorpus(const ParsedStream& stream, const ScoreOptions& opts)
{
    return isCachedOnActive(stream, opts.activeDebrids) || !stream.url.isEmpty()
        || stream.seeders >= trackingMinSeeders;
}

bool isWebishForCorpus(const ParsedStream& stream)
{
    return stream.source == Source::WEB_DL || stream.source == Source::WEBRip
        || stream.source == Source::BluRay || stream.source == Source::BDRip;
}

} // namespace

ScoredStream StreamScorer::scoreStream(const ParsedStream& stream,
                                       const ScoreOptions& opts,
                                       const CorpusStats* corpus)
{
    ScoredStream out;
    static_cast<ParsedStream&>(out) = stream;
    out.score = 0;
    out.reasons.clear();

    const bool cached = isCachedOnActive(stream, opts.activeDebrids);
    const bool directPlayable = !stream.url.isEmpty();
    const bool isEasynews = stream.addonName.contains(QStringLiteral("easynews"), Qt::CaseInsensitive)
        || stream.parsedTitle.contains(QStringLiteral("easynews"), Qt::CaseInsensitive);
    if (cached || isEasynews)
        addReason(out, cached ? QStringLiteral("cached") : QStringLiteral("easynews-direct"), 60);
    else if (directPlayable)
        addReason(out, QStringLiteral("direct url"), 25);

    const ScoreReason resBoost = resolutionPoints(stream);
    if (resBoost.delta != 0)
        addReason(out, resBoost);

    if (stream.hdrFormat != HdrFormat::None) {
        const int hdrDelta
            = (stream.hdrFormat == HdrFormat::DV_HDR10 || stream.hdrFormat == HdrFormat::DV) ? 6 : 5;
        addReason(out, hdrFormatString(stream.hdrFormat), hdrDelta);
    }

    if (stream.codec == Codec::HEVC)
        addReason(out, QStringLiteral("HEVC"), 1);
    else if (stream.codec == Codec::AV1)
        addReason(out, QStringLiteral("AV1"), 1);

    const ScoreReason audioDelta = audioPoints(stream);
    if (audioDelta.delta != 0)
        addReason(out, audioDelta);

    if (stream.audio.channels >= 6)
        addReason(out, QStringLiteral("%1.0 channels").arg(stream.audio.channels), 2);

    if (!cached && stream.seeders >= 0) {
        const int seedDelta = std::min(stream.seeders / 10, 10);
        if (seedDelta > 0) {
            addReason(out, QStringLiteral("seeders=%1").arg(stream.seeders), seedDelta);
        } else if (stream.url.isEmpty() && !stream.infoHash.isEmpty() && stream.seeders == 0) {
            addReason(out, QStringLiteral("zero-seeders-stale-meta"), -20);
        }
    }
    if (!stream.infoHash.isEmpty() && stream.seeders == 0 && !cached)
        addReason(out, QStringLiteral("zero-seeders-soft"), -8);

    bool expectedYearOk = false;
    const int expectedYear = opts.releaseDate.left(4).toInt(&expectedYearOk);
    if (expectedYearOk && stream.year > 0) {
        const int diff = qAbs(stream.year - expectedYear);
        if (diff != 0) {
            const qint64 daysFromRelease = absoluteDaysSinceDate(opts.releaseDate);
            const bool isRecent = daysFromRelease >= 0 && daysFromRelease < 365;
            if (diff == 1) {
                const int delta = isRecent ? -75 : -18;
                addReason(out,
                          QStringLiteral("year-off-by-1:%1vs%2%3")
                              .arg(stream.year)
                              .arg(expectedYear)
                              .arg(isRecent ? QStringLiteral("-recent") : QString()),
                          delta);
            } else {
                const int delta = isRecent ? -150 : -70;
                addReason(out,
                          QStringLiteral("year-mismatch:%1vs%2%3")
                              .arg(stream.year)
                              .arg(expectedYear)
                              .arg(isRecent ? QStringLiteral("-recent") : QString()),
                          delta);
            }
        }
    }

    if (isTrustedGroup(stream.releaseGroupNormalized))
        addReason(out, QStringLiteral("group:%1").arg(stream.releaseGroupNormalized), 2);

    if (stream.remux)
        addReason(out, QStringLiteral("REMUX"), 3);

    if (stream.source == Source::CAM)
        addReason(out, QStringLiteral("CAM penalty"), -80);
    else if (stream.source == Source::TS || stream.source == Source::HDTS)
        addReason(out, QStringLiteral("Telesync penalty"), -60);
    else if (stream.source == Source::TC)
        addReason(out, QStringLiteral("Telecine penalty"), -50);
    else if (stream.source == Source::SCR)
        addReason(out, QStringLiteral("Screener penalty"), -40);

    if (stream.proper || stream.repackIteration > 0) {
        const int r = std::min(2, stream.repackIteration > 0 ? stream.repackIteration : 1);
        addReason(out,
                  stream.proper ? QStringLiteral("PROPER")
                                : QStringLiteral("REPACK%1").arg(stream.repackIteration),
                  r);
    }

    if (!opts.preferredLanguages.isEmpty()) {
        if (stream.audioLanguages.isEmpty()) {
            addReason(out, QStringLiteral("language-unknown"), -3);
        } else {
            const bool isMulti = stream.audioLanguages.contains(QStringLiteral("Multi"));
            bool match = false;
            for (const QString& language : stream.audioLanguages) {
                for (const QString& preferred : opts.preferredLanguages) {
                    if (languageMatches(language, preferred)) {
                        match = true;
                        break;
                    }
                }
                if (match)
                    break;
            }

            if (match) {
                addReason(out, QStringLiteral("preferred-language"), 12);
            } else if (isMulti) {
                if (opts.preferSingleAudioTrack)
                    addReason(out, QStringLiteral("html5-multi-audio-penalty"), -18);
                else
                    addReason(out, QStringLiteral("multi-language"), 4);
            } else {
                addReason(out, QStringLiteral("language-mismatch"), -14);
            }
        }
    } else if (opts.preferSingleAudioTrack
               && stream.audioLanguages.contains(QStringLiteral("Multi"))) {
        addReason(out, QStringLiteral("html5-multi-audio-penalty"), -12);
    }

    if (stream.scamScore > 0)
        addReason(out, QStringLiteral("scam-penalty"), -stream.scamScore);

    if (!stream.url.isEmpty() && !cached)
        addReason(out, QStringLiteral("prelinked-url"), 4);

    if (!opts.preferAddonId.isEmpty() && stream.addonId == opts.preferAddonId)
        addReason(out, QStringLiteral("origin-addon"), 250);

    const ScoreReason trustedAddonBoost = trustedAddonPoints(stream);
    if (trustedAddonBoost.delta > 0)
        addReason(out, trustedAddonBoost);

    const int playabilityDelta = playabilityPenalty(stream);
    if (playabilityDelta < 0)
        addReason(out, QStringLiteral("webview2-unfriendly"), playabilityDelta);

    const ScoreReason bitratePenalty = bitrateBudgetPenalty(stream, opts, cached);
    if (bitratePenalty.delta < 0)
        addReason(out, bitratePenalty);

    const qint64 expectedMin = opts.runtimeMinutes > 0
        ? expectedMinSizeBytes(stream.resolution, opts.runtimeMinutes)
        : 0;
    const bool hasValidSize = stream.size > 0 && expectedMin > 0 && stream.size >= expectedMin;

    const int sizePenalty = sizeMislabelPenalty(stream, expectedMin);
    if (sizePenalty < 0)
        addReason(out, QStringLiteral("size-mismatch"), sizePenalty);

    const int desyncPenalty = camInFilenamePenalty(stream);
    if (desyncPenalty < 0)
        addReason(out, QStringLiteral("title-says-hires-filename-says-cam"), desyncPenalty);

    const ScoreReason undersizedPenalty = undersizedNewReleasePenalty(stream, opts);
    if (undersizedPenalty.delta < 0)
        addReason(out, undersizedPenalty);

    const ScoreReason tinyPenalty = impossiblySmallMoviePenalty(stream, opts);
    if (tinyPenalty.delta < 0)
        addReason(out, tinyPenalty);

    const ScoreReason recency = freshTheatricalAdjust(stream, opts, hasValidSize, corpus);
    if (recency.delta != 0)
        addReason(out, recency);

    out.tier = tierOf(stream);
    Q_UNUSED(resolutionString(stream.resolution));
    return out;
}

RankedPicker StreamScorer::rankAndPick(const QVector<ScoredStream>& streams,
                                       const QStringList& activeDebrids,
                                       bool preferAac,
                                       bool respectAddonOrder)
{
    RankedPicker picker;
    picker.all = streams;

    std::stable_sort(picker.all.begin(), picker.all.end(), [respectAddonOrder](const ScoredStream& a,
                                                                                const ScoredStream& b) {
        if (respectAddonOrder && a.addonPriority != b.addonPriority)
            return a.addonPriority < b.addonPriority;
        return a.score > b.score;
    });

    QVector<ScoredStream> cachedFirst = picker.all;
    std::stable_sort(cachedFirst.begin(), cachedFirst.end(), [&activeDebrids](const ScoredStream& a,
                                                                              const ScoredStream& b) {
        const int ac = rankPlayable(a, activeDebrids) ? 1 : 0;
        const int bc = rankPlayable(b, activeDebrids) ? 1 : 0;
        return bc < ac;
    });

    for (const ScoredStream& stream : cachedFirst) {
        const int tierKey = static_cast<int>(stream.tier);
        if (!picker.byTier.contains(tierKey))
            picker.byTier.insert(tierKey, stream);
    }

    for (const ScoredStream& stream : picker.all) {
        if (rankPlayable(stream, activeDebrids)) {
            picker.primary = stream;
            break;
        }
    }

    if (preferAac && picker.primary.has_value()) {
        for (const ScoredStream& stream : picker.all) {
            if (rankPlayable(stream, activeDebrids) && stream.audio.codec == AudioCodec::AAC) {
                picker.primary = stream;
                break;
            }
        }
    }

    return picker;
}

CorpusStats StreamScorer::computeCorpusStats(const QVector<ParsedStream>& streams,
                                             const ScoreOptions& opts)
{
    CorpusStats stats;
    const qint64 days = signedDaysSinceDate(opts.releaseDate);
    if (days != std::numeric_limits<qint64>::min())
        stats.daysSinceRelease = double(days);

    QVector<ParsedStream> tracked;
    tracked.reserve(streams.size());
    for (const ParsedStream& stream : streams) {
        if (isTrackedForCorpus(stream, opts))
            tracked.push_back(stream);
    }

    stats.trustedTrackedCount = tracked.size();
    int theater = 0;
    int webish = 0;
    for (const ParsedStream& stream : tracked) {
        if (isTheaterCapture(stream.source))
            ++theater;
        if (isWebishForCorpus(stream))
            ++webish;
    }

    const int trackedTotal = stats.trustedTrackedCount > 0 ? stats.trustedTrackedCount : 1;
    const int streamTotal = std::max(int(streams.size()), 1);
    stats.trustedTrackedFraction = double(stats.trustedTrackedCount) / double(streamTotal);
    stats.theaterCaptureFraction = double(theater) / double(trackedTotal);
    stats.webishFraction = double(webish) / double(trackedTotal);
    return stats;
}

} // namespace tankoban
