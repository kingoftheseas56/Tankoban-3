// Tankoban 3 - see StreamRowPaint.h. Logic moved verbatim from StremioRow.cpp.
#include "ui/StreamRowPaint.h"

namespace tankoban {

QString streamRowKey(const ScoredStream& s)
{
    const QString id = !s.infoHash.isEmpty()
        ? QStringLiteral("h:%1:%2").arg(s.infoHash).arg(s.fileIdx)
        : (!s.url.isEmpty() ? QStringLiteral("u:%1").arg(s.url)
                            : QStringLiteral("t:%1").arg(s.title.isEmpty() ? s.name : s.title));
    return s.addonId + QLatin1Char('|') + id;
}

QString addonInstanceKey(const ScoredStream& s)
{
    return s.addonUrl.isEmpty() ? s.addonId : s.addonUrl.toString();
}

QString qualityGroupOf(const ScoredStream& s)
{
    switch (s.resolution) {
    case Resolution::FourK:  return QStringLiteral("4K");
    case Resolution::FullHD: return QStringLiteral("1080p");
    case Resolution::HD:     return QStringLiteral("720p");
    default:                 return QStringLiteral("SD");
    }
}

RowConfidence qualityConfidence(const ScoredStream& s)
{
    const bool nothingDetected =
        (s.resolution == Resolution::SD || s.resolution == Resolution::None)
        && s.source == Source::Other && s.codec == Codec::Other
        && s.hdrFormat == HdrFormat::None && s.audio.codec == AudioCodec::Other;
    if (nothingDetected)
        return RowConfidence::Unlabeled;

    for (const ScoreReason& r : s.reasons) {
        const QString& sig = r.signal;
        if (sig.startsWith(QStringLiteral("fresh-fake-"))
            || sig == QStringLiteral("fresh-soft-flag")
            || sig == QStringLiteral("fresh-prerelease-soft")
            || sig == QStringLiteral("fresh-prebluray-suspect")
            || sig == QStringLiteral("size-mismatch")
            || sig == QStringLiteral("title-says-hires-filename-says-cam")) {
            return RowConfidence::Unverified;
        }
    }

    const bool claimsHighRes =
        s.resolution == Resolution::FourK || s.resolution == Resolution::FullHD;
    const bool directStream = !s.url.isEmpty() && s.infoHash.isEmpty();
    if (claimsHighRes && s.size <= 0 && s.source == Source::Other && !directStream)
        return RowConfidence::Unverified;
    return RowConfidence::Labeled;
}

QStringList streamBadges(const ScoredStream& s)
{
    QStringList out;

    // 1) capture source overrides resolution.
    QString capture;
    if (s.source == Source::CAM)                                 capture = QStringLiteral("CAM");
    else if (s.source == Source::TS || s.source == Source::HDTS) capture = QStringLiteral("TS");
    else if (s.source == Source::TC)                             capture = QStringLiteral("TC");
    else if (s.source == Source::SCR)                            capture = QStringLiteral("SCR");

    const RowConfidence conf = qualityConfidence(s);
    if (!capture.isEmpty()) {
        out << capture;
    } else if (conf == RowConfidence::Unlabeled) {
        out << QStringLiteral("NO LABEL");
    } else if (conf == RowConfidence::Unverified) {
        out << QStringLiteral("UNKNOWN");
    } else {
        switch (s.resolution) {
        case Resolution::FourK:  out << QStringLiteral("4K"); break;
        case Resolution::FullHD: out << QStringLiteral("1080p"); break;
        case Resolution::HD:     out << QStringLiteral("720p"); break;
        case Resolution::SD_480:
            out << (s.source == Source::DVDRip ? QStringLiteral("DVD") : QStringLiteral("480p"));
            break;
        case Resolution::SD:
            out << (s.source == Source::DVDRip ? QStringLiteral("DVD") : QStringLiteral("SD"));
            break;
        case Resolution::None: break;
        }
    }

    // 2) release/source badge, only when no capture source.
    if (capture.isEmpty()) {
        if (s.remux)
            out << QStringLiteral("REMUX");
        else if (s.source == Source::BluRay || s.source == Source::BDRip)
            out << QStringLiteral("BluRay");
        else if (s.source == Source::WEB_DL || s.source == Source::WEBRip
                 || s.source == Source::HDRip)
            out << QStringLiteral("WEB-DL");
        else if (s.source == Source::HDTV)
            out << QStringLiteral("HDTV");
    }

    // 3) codec.
    if (s.codec == Codec::HEVC)     out << QStringLiteral("HEVC");
    else if (s.codec == Codec::AV1) out << QStringLiteral("AV1");

    // 4) HDR.
    if (s.hdrFormat == HdrFormat::DV || s.hdrFormat == HdrFormat::DV_HDR10) out << QStringLiteral("DV");
    else if (s.hdrFormat == HdrFormat::HDR10Plus)                          out << QStringLiteral("HDR10+");
    else if (s.hdrFormat == HdrFormat::HDR10)                             out << QStringLiteral("HDR10");
    else if (s.hdrFormat == HdrFormat::HLG)                               out << QStringLiteral("HLG");

    // 5) audio.
    switch (s.audio.codec) {
    case AudioCodec::Atmos:     out << QStringLiteral("ATMOS"); break;
    case AudioCodec::TrueHD:    out << QStringLiteral("TRUEHD"); break;
    case AudioCodec::DTS_HD_MA: out << QStringLiteral("DTS-HD"); break;
    case AudioCodec::DTS:       out << QStringLiteral("DTS"); break;
    case AudioCodec::DDPlus:    out << QStringLiteral("DD+"); break;
    case AudioCodec::FLAC:      out << QStringLiteral("FLAC"); break;
    case AudioCodec::AAC:       out << QStringLiteral("AAC"); break;
    default:
        if (s.audio.channels == 1) out << QStringLiteral("MONO");
        break;
    }
    return out;
}

bool isPlayable(const ScoredStream& s)
{
    const bool hasDirectUrl = !s.url.isEmpty() && s.url != QStringLiteral("#");
    return hasDirectUrl || !s.infoHash.isEmpty();
}

} // namespace tankoban
