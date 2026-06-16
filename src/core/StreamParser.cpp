#include "core/StreamParser.h"

#include <cmath>
#include <limits>

#include <QPair>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QSet>

namespace tankoban {

namespace {

struct PttResult {
    QString title;
    QString resolution;
    QString codec;
    QString group;
    int year = 0;
    int season = 0;
    int episode = 0;
    bool proper = false;
    bool repack = false;
    bool extended = false;
    bool unrated = false;
    bool theatrical = false;
    bool uncut = false;
    bool remastered = false;
    bool criterion = false;
    bool openmatte = false;
    bool hardcoded = false;
    int channels = 2;
    int bitDepth = 0;
};

QRegularExpression makeRx(const QString& pattern,
                          QRegularExpression::PatternOptions options
                          = QRegularExpression::CaseInsensitiveOption)
{
    return QRegularExpression(pattern, options);
}

const QRegularExpression torrentioNoiseRx = makeRx(
    QStringLiteral(R"re(^[\s\p{So}\p{Cf}]+|[\s\p{So}\p{Cf}]+$)re"),
    QRegularExpression::UseUnicodePropertiesOption);
const QRegularExpression newlineRx = makeRx(QStringLiteral(R"re(\r?\n)re"),
                                           QRegularExpression::NoPatternOption);
const QRegularExpression providerHeadingRx = makeRx(
    QStringLiteral(R"re(^(?:torrentio|comet|mediafusion|aiostreams|knightcrawler|jackettio|torbox)\b)re"));
const QRegularExpression standaloneQualityRx = makeRx(
    QStringLiteral(R"re(^(?:4k|1080p|720p|480p|sd|hd|hdr|dv|uhd)$)re"));
const QRegularExpression lineStartsEmojiRx = makeRx(
    QStringLiteral(R"re(^[\p{So}\p{Cf}])re"),
    QRegularExpression::UseUnicodePropertiesOption);
const QRegularExpression metricLineRx = makeRx(
    QStringLiteral(R"re(^(?:size|seeders?|peers?|languages?)\s*[:=])re"));
const QRegularExpression libraryLineRx = makeRx(
    QStringLiteral(R"re(^\[(?:RD|TB|AD|PM|DL)\+\]\s+\S+\s+library)re"));
const QRegularExpression yearRx = makeRx(QStringLiteral(R"re(\b(?:19|20)\d{2}\b)re"),
                                         QRegularExpression::NoPatternOption);
const QRegularExpression resolutionProbeRx = makeRx(
    QStringLiteral(R"re(\b\d{3,4}p\b|\b(?:4k|uhd|2160p)\b)re"));
const QRegularExpression episodeProbeRx = makeRx(QStringLiteral(R"re(\bS\d{1,2}E\d{1,3}\b)re"));
const QRegularExpression sourceProbeRx = makeRx(
    QStringLiteral(R"re(\b(?:Blu[.\-]?Ray|WEB[.\-]?DL|WEBRip|HDRip|BDRip|HDTV|REMUX|Remux|HDCAM|TELESYNC|TELECINE|CAM|HDTS|DVDRip)\b)re"));
const QRegularExpression codecProbeRx = makeRx(
    QStringLiteral(R"re(\b(?:x264|x265|HEVC|AVC|h264|h265|AV1|MPEG2|MPEG-2)\b)re"));
const QRegularExpression containerProbeRx = makeRx(QStringLiteral(R"re(\.(?:mkv|mp4|m4v|avi|ts)\b)re"));

const QRegularExpression pttYearRx = makeRx(QStringLiteral(R"re(\b((?:19|20)\d{2})\b)re"),
                                           QRegularExpression::NoPatternOption);
const QRegularExpression pttSeasonEpisodeRx = makeRx(
    QStringLiteral(R"re(\bS(\d{1,2})E(\d{1,3})\b)re"));
const QRegularExpression pttSeasonOnlyRx = makeRx(QStringLiteral(R"re(\bS(\d{1,2})\b(?!E))re"));
const QRegularExpression pttEpisodeOnlyRx = makeRx(QStringLiteral(R"re(\bE(\d{1,3})\b)re"));
const QRegularExpression pttResolutionRx = makeRx(
    QStringLiteral(R"re(\b(2160p|1080p|720p|480p|4k|uhd)\b)re"));
const QRegularExpression pttCodecRx = makeRx(
    QStringLiteral(R"re(\b(x265|x264|h\.?265|h\.?264|hevc|avc|av1|vp9|mpeg-?2)\b)re"));
const QRegularExpression pttGroupDashRx = makeRx(QStringLiteral(R"re(-([A-Za-z0-9][A-Za-z0-9._-]{1,30})$)re"));
const QRegularExpression pttGroupBracketRx = makeRx(QStringLiteral(R"re(\[([A-Za-z0-9][A-Za-z0-9._-]{1,30})\]$)re"));
const QRegularExpression pttGroupExtensionRx = makeRx(QStringLiteral(R"re(\.(?:mkv|mp4|m4v|avi|webm|mov|ts|wmv)$)re"));
const QRegularExpression pttProperRx = makeRx(QStringLiteral(R"re(\bPROPER\b)re"));
const QRegularExpression pttRepackRx = makeRx(QStringLiteral(R"re(\bREPACK\d*\b)re"));
const QRegularExpression pttExtendedRx = makeRx(QStringLiteral(R"re(\bEXTENDED\b)re"));
const QRegularExpression pttUnratedRx = makeRx(QStringLiteral(R"re(\bUNRATED\b)re"));
const QRegularExpression pttTheatricalRx = makeRx(QStringLiteral(R"re(\bTHEATRICAL\b)re"));
const QRegularExpression pttUncutRx = makeRx(QStringLiteral(R"re(\bUNCUT\b)re"));
const QRegularExpression pttRemasteredRx = makeRx(QStringLiteral(R"re(\bREMASTERED\b)re"));
const QRegularExpression pttCriterionRx = makeRx(QStringLiteral(R"re(\bCRITERION\b)re"));
const QRegularExpression pttOpenMatteRx = makeRx(QStringLiteral(R"re(\bOPEN[.\s]?MATTE\b)re"));
const QRegularExpression pttHardcodedRx = makeRx(QStringLiteral(R"re(\b(HC|HARDCODED|HARDSUB)\b)re"));
const QRegularExpression pttChannelsRx = makeRx(QStringLiteral(R"re(\b(7\.1|6\.1|5\.1|2\.1|2\.0)\b)re"),
                                               QRegularExpression::NoPatternOption);
const QRegularExpression pttBitDepthRx = makeRx(QStringLiteral(R"re(\b(8|10|12)\s*bit\b)re"));
const QRegularExpression titleStopRx = makeRx(
    QStringLiteral(R"re([\s._\-\[]+(?:19|20)\d{2}\b|[\s._\-\[]+S\d{1,2}E\d{1,3}\b|[\s._\-\[]+S\d{1,2}\b|[\s._\-\[]+(?:2160p|1080p|720p|480p|4k|uhd)\b)re"));
const QRegularExpression titleCleanRx = makeRx(QStringLiteral(R"re([._]+)re"),
                                              QRegularExpression::NoPatternOption);
const QRegularExpression titleSpaceRx = makeRx(QStringLiteral(R"re(\s+)re"),
                                              QRegularExpression::NoPatternOption);

const QRegularExpression sourceHcCamRx = makeRx(
    QStringLiteral(R"re(\bHC[\s._\-]?(?:HDRip|HD[\s._\-]?Rip|CAM(?:Rip)?)\b)re"));
const QRegularExpression sourceCamRx = makeRx(
    QStringLiteral(R"re(\b(?:HD|Clean|New|HQ|TS)[\s._\-]?CAM(?:Rip)?\b|\bCAM(?:Rip)?\b)re"));
const QRegularExpression sourceHdtsRx = makeRx(QStringLiteral(R"re(\bHD[\s._\-]?TS\b|\bHDTS\b)re"));
const QRegularExpression sourceTsRx = makeRx(
    QStringLiteral(R"re(\bTELESYNC\b|\bTS[\s._\-]?Rip\b|\bPDVDRip\b|\bTS\b(?=[\s._\-]\d{3,4}[pi]\b)|(?<=\b(?:19|20)\d{2}[\s._\-])TS\b)re"));
const QRegularExpression sourceTcRx = makeRx(
    QStringLiteral(R"re(\bTELECINE\b|\bHD[\s._\-]?TC\b|\bTC\b(?=[\s._\-]\d{3,4}[pi]\b)|(?<=\b(?:19|20)\d{2}[\s._\-])TC\b)re"));
const QRegularExpression sourceScrRx = makeRx(
    QStringLiteral(R"re(\bSCREENER\b|\bDVDSCR\b|\bDVDScreener\b|\bBDSCR\b|\bWEB[\s._\-]?SCR\b|\bSCR\b)re"));
const QRegularExpression sourceRemuxRx = makeRx(QStringLiteral(R"re(\bRemux\b)re"));
const QRegularExpression sourceBlurayRx = makeRx(QStringLiteral(R"re(\bBluRay\b|\bBDRip\b|\bBRRip\b)re"));
const QRegularExpression sourceWebDlRx = makeRx(QStringLiteral(R"re(\bWEB[.\-]?DL\b)re"));
const QRegularExpression sourceWebRipRx = makeRx(QStringLiteral(R"re(\bWEBRip\b|\bWEB-Rip\b)re"));
const QRegularExpression sourceHdripRx = makeRx(QStringLiteral(R"re(\bHDRip\b)re"));
const QRegularExpression sourceDvdripRx = makeRx(QStringLiteral(R"re(\bDVDRip\b)re"));
const QRegularExpression sourceHdtvRx = makeRx(QStringLiteral(R"re(\bHDTV\b)re"));
const QRegularExpression sourceWebRx = makeRx(QStringLiteral(R"re(\bWEB\b)re"));

const QRegularExpression hdrDvHdr10Rx = makeRx(
    QStringLiteral(R"re(\bDV[+\-\s.]?HDR10\+?\b|\bDoVi[+\-\s.]?HDR10\+?\b|\bDolby[.\s]?Vision[+\-\s.]?HDR10\+?\b)re"));
const QRegularExpression hdrDvRx = makeRx(QStringLiteral(R"re(\bDV\b|\bDoVi\b|\bDolby[.\s]?Vision\b)re"));
const QRegularExpression hdrHdr10PlusRx = makeRx(QStringLiteral(R"re(\bHDR10\+\b)re"));
const QRegularExpression hdrHlgRx = makeRx(QStringLiteral(R"re(\bHLG\b)re"));
const QRegularExpression hdrHdr10Rx = makeRx(QStringLiteral(R"re(\bHDR10?\b|\bHDR\b)re"));

const QRegularExpression audioAtmosRx = makeRx(QStringLiteral(R"re(\bAtmos\b)re"));
const QRegularExpression audioTrueHdRx = makeRx(QStringLiteral(R"re(\bTrueHD\b)re"));
const QRegularExpression audioDtsHdMaRx = makeRx(QStringLiteral(R"re(\bDTS-HD\.?MA\b|\bDTS\.?HD\.?MA\b)re"));
const QRegularExpression audioDtsRx = makeRx(QStringLiteral(R"re(\bDTS\b)re"));
const QRegularExpression audioDdPlusRx = makeRx(QStringLiteral(R"re(\bDDP?5?\.?1\+?\b|\bE-?AC3\b|\bDD\+\b)re"));
const QRegularExpression audioAc3Rx = makeRx(QStringLiteral(R"re(\bAC3\b)re"));
const QRegularExpression audioAacRx = makeRx(QStringLiteral(R"re(\bAAC\b)re"));
const QRegularExpression audioFlacRx = makeRx(QStringLiteral(R"re(\bFLAC\b)re"));
const QRegularExpression audioOpusRx = makeRx(QStringLiteral(R"re(\bOpus\b)re"));
const QRegularExpression channelsRx = makeRx(QStringLiteral(R"re(\b(7\.1|5\.1|6\.1|2\.1|2\.0)\b)re"),
                                            QRegularExpression::NoPatternOption);
const QRegularExpression bitDepthRx = makeRx(QStringLiteral(R"re(\b(8|10|12)\s*bit\b)re"));

const QRegularExpression langRx = makeRx(
    QStringLiteral(R"re(\b(ENG(?:LISH)?|ITA(?:LIAN)?|RUS(?:SIAN)?|HIN(?:DI)?|ESP|SPA(?:NISH)?|LAT(?:INO|AM)?|CASTELLANO|KOR(?:EAN)?|JPN|JAPANESE|JAP|CHN|CHI(?:NESE)?|ZHO|MAN(?:DARIN)?|CANTONESE|POR(?:TUGUESE)?|PTBR|DUBLADO|GER(?:MAN)?|DEU|FRA|FRENCH|FRE|VFF|VFQ|VOSTFR|TUR(?:KISH)?|ARA(?:BIC)?|TAM(?:IL)?|TEL(?:UGU)?|CES|CZECH|CZE|DAN(?:ISH)?|FIN(?:NISH)?|HEB(?:REW)?|HUN(?:GARIAN)?|NLD|DUTCH|DUT|NOR(?:WEGIAN)?|POL(?:ISH)?|RON|ROM|ROMANIAN|SWE(?:DISH)?|THA|THAI|UKR(?:AINIAN)?|VIE(?:TNAMESE)?|MULTI|DUAL)\b)re"));
const QRegularExpression flagRx = makeRx(
    QStringLiteral(R"re([\x{1F1E6}-\x{1F1FF}][\x{1F1E6}-\x{1F1FF}])re"),
    QRegularExpression::UseUnicodePropertiesOption);
const QRegularExpression isoPairRx = makeRx(
    QStringLiteral(R"re((?:^|[.\-_\s\[(])(EN|FR|ES|IT|DE|PT|RU|JA|JP|KO|KR|ZH|CN|HI|AR|TR|NL|PL|RO|SV|SE|DA|FI|CS|CZ|HU|TH|UK|UA|VI|GB|US|MX|BR|CA|AU|NZ|TW|HK|IL|HE|SA|AE|EG|GR|ID|MY|PH|IR|FA)(?:[.\-_\s\])]|$))re"),
    QRegularExpression::NoPatternOption);

const QRegularExpression containerRx = makeRx(QStringLiteral(R"re(\.(mkv|mp4|m4v|avi|webm|mov|ts|wmv)\b)re"));
const QRegularExpression sizeRx = makeRx(QStringLiteral(R"re((\d+(?:\.\d+)?)\s*(GB|MB|TB|GiB|MiB|TiB)\b)re"));
const QRegularExpression seedersRx = makeRx(QStringLiteral(R"re((?:S:|seeds?:?|\bS\s*=\s*)\s*(\d+))re"));
const QRegularExpression seedersEmojiRx = makeRx(QStringLiteral(R"re([\p{So}]\s*(\d+))re"),
                                                 QRegularExpression::UseUnicodePropertiesOption);
const QRegularExpression animeHashRx = makeRx(QStringLiteral(R"re(\[([0-9A-F]{8})\])re"));
const QRegularExpression repackRx = makeRx(QStringLiteral(R"re(\bREPACK(\d+)?\b)re"));
const QRegularExpression yearRangeRx = makeRx(QStringLiteral(R"re(\b(19\d\d|20\d\d)[\-.](19\d\d|20\d\d)\b)re"),
                                             QRegularExpression::NoPatternOption);
const QRegularExpression discRx = makeRx(QStringLiteral(R"re(\bDISC\s*(\d+)\b)re"));
const QRegularExpression editionRx = makeRx(
    QStringLiteral(R"re(\b(IMAX|EXTENDED|DIRECTORS?[.\s]?CUT|THEATRICAL|UNRATED|UNCUT|REMASTERED|RESTORATION|CRITERION|OPEN[.\s]?MATTE|HYBRID)\b)re"));
const QRegularExpression editionNormalizeRx = makeRx(QStringLiteral(R"re([.\s]+)re"),
                                                     QRegularExpression::NoPatternOption);
const QRegularExpression qualityStopRx = makeRx(
    QStringLiteral(R"re(\.(?:480p|576p|720p|1080p|1440p|2160p|4k|uhd|hdr|hdr10|dv|dovi|bluray|bdrip|brrip|web[.\-]?dl|webrip|hdrip|hdtv|remux|cam|ts|hdts|tc|scr|x264|x265|hevc|avc|h\.?264|h\.?265|av1|aac|ac3|ddp?|eac3|dts|truehd|atmos|flac|opus|10bit|8bit|repack|proper|extended|directors?|imax|hybrid|hdr10\+|repack\d?|multi|dual|dubbed|sub|subbed|complete|amzn|nf|hulu|max|atvp|dsnp))re"));
const QRegularExpression remuxRx = makeRx(QStringLiteral(R"re(\bRemux\b)re"));
const QRegularExpression hardcodedRx = makeRx(QStringLiteral(R"re(\b(HC|HARDCODED|HARDSUB)\b)re"));
const QRegularExpression nonAlnumRx = makeRx(QStringLiteral(R"re([^A-Z0-9])re"),
                                            QRegularExpression::NoPatternOption);
const QRegularExpression episodeJunkRx = makeRx(QStringLiteral(R"re(^(?:e\d+|episode|hdtv|webrip)$)re"));
const QRegularExpression leadingSepRx = makeRx(QStringLiteral(R"re(^[.\-_\s]+)re"),
                                              QRegularExpression::NoPatternOption);
const QRegularExpression separatorRx = makeRx(QStringLiteral(R"re([.\-_]+)re"),
                                             QRegularExpression::NoPatternOption);
const QRegularExpression seasonPackRx = makeRx(
    QStringLiteral(R"re(\b(complete|season[\s.]?pack|s\d{1,2}\b(?!e))\b)re"));

QVector<const QRegularExpression*> allRegexes()
{
    return {
        &torrentioNoiseRx, &newlineRx, &providerHeadingRx, &standaloneQualityRx, &lineStartsEmojiRx,
        &metricLineRx, &libraryLineRx, &yearRx, &resolutionProbeRx, &episodeProbeRx,
        &sourceProbeRx, &codecProbeRx, &containerProbeRx, &pttYearRx,
        &pttSeasonEpisodeRx, &pttSeasonOnlyRx, &pttEpisodeOnlyRx, &pttResolutionRx,
        &pttCodecRx, &pttGroupDashRx, &pttGroupBracketRx, &pttGroupExtensionRx,
        &pttProperRx, &pttRepackRx, &pttExtendedRx, &pttUnratedRx, &pttTheatricalRx,
        &pttUncutRx, &pttRemasteredRx, &pttCriterionRx, &pttOpenMatteRx,
        &pttHardcodedRx, &pttChannelsRx, &pttBitDepthRx, &titleStopRx, &titleCleanRx,
        &titleSpaceRx, &sourceHcCamRx, &sourceCamRx, &sourceHdtsRx, &sourceTsRx, &sourceTcRx, &sourceScrRx, &sourceRemuxRx,
        &sourceBlurayRx, &sourceWebDlRx, &sourceWebRipRx, &sourceHdripRx, &sourceDvdripRx,
        &sourceHdtvRx, &sourceWebRx, &hdrDvHdr10Rx, &hdrDvRx, &hdrHdr10PlusRx,
        &hdrHlgRx, &hdrHdr10Rx, &audioAtmosRx, &audioTrueHdRx, &audioDtsHdMaRx,
        &audioDtsRx, &audioDdPlusRx, &audioAc3Rx, &audioAacRx, &audioFlacRx,
        &audioOpusRx, &channelsRx, &bitDepthRx, &langRx, &flagRx, &isoPairRx,
        &containerRx, &sizeRx, &seedersRx, &seedersEmojiRx, &animeHashRx, &repackRx,
        &yearRangeRx, &discRx, &editionRx, &editionNormalizeRx, &qualityStopRx, &remuxRx,
        &hardcodedRx, &nonAlnumRx, &episodeJunkRx, &leadingSepRx, &separatorRx, &seasonPackRx
    };
}

int mapChannels(const QString& label)
{
    if (label == QStringLiteral("7.1"))
        return 8;
    if (label == QStringLiteral("6.1"))
        return 7;
    if (label == QStringLiteral("5.1"))
        return 6;
    if (label == QStringLiteral("2.1"))
        return 3;
    return 2;
}

int filenameScore(const QString& line)
{
    if (line.length() < 8)
        return -100;
    if (providerHeadingRx.match(line).hasMatch() || standaloneQualityRx.match(line).hasMatch()
        || lineStartsEmojiRx.match(line).hasMatch())
        return -100;
    if (metricLineRx.match(line).hasMatch() || libraryLineRx.match(line).hasMatch())
        return -50;

    const bool hasYear = yearRx.match(line).hasMatch();
    const bool hasResolution = resolutionProbeRx.match(line).hasMatch();
    const bool hasEpisode = episodeProbeRx.match(line).hasMatch();
    const bool hasSource = sourceProbeRx.match(line).hasMatch();
    const bool hasCodec = codecProbeRx.match(line).hasMatch();
    const bool hasContainer = containerProbeRx.match(line).hasMatch();
    const bool hasDots = line.count(QLatin1Char('.')) >= 3;
    const int technicalMarkers = int(hasYear) + int(hasResolution) + int(hasEpisode)
        + int(hasSource) + int(hasCodec) + int(hasContainer) + int(hasDots);

    if (technicalMarkers == 0)
        return -20;

    int score = 0;
    if (line.length() >= 20)
        score += 2;
    if (hasDots)
        score += 3;
    if (hasYear)
        score += 2;
    if (hasResolution)
        score += 2;
    if (hasEpisode)
        score += 3;
    if (hasSource)
        score += 3;
    if (hasCodec)
        score += 1;
    if (hasContainer)
        score += 2;
    return score;
}

QString extractFilenameLine(const Stream& stream)
{
    QStringList lines;
    const QString filename = !stream.filename.isEmpty() ? stream.filename : stream.fileName;
    for (const QString& raw : {stream.title, filename, stream.description, stream.name}) {
        if (raw.isEmpty())
            continue;
        for (const QString& line : raw.split(newlineRx)) {
            const QString trimmed = QString(line).replace(torrentioNoiseRx, QString()).trimmed();
            if (!trimmed.isEmpty())
                lines.push_back(trimmed);
        }
    }

    QString best;
    int bestScore = std::numeric_limits<int>::min();
    for (const QString& line : lines) {
        const int score = filenameScore(line);
        if (score > bestScore) {
            bestScore = score;
            best = line;
        }
    }
    return best;
}

PttResult parsePtt(const QString& input)
{
    PttResult out;
    const QString text = input.trimmed();

    auto matchInt = [](const QRegularExpression& rx, const QString& value, int group = 1) {
        const QRegularExpressionMatch match = rx.match(value);
        return match.hasMatch() ? match.captured(group).toInt() : 0;
    };

    QRegularExpressionMatch match = pttYearRx.match(text);
    if (match.hasMatch())
        out.year = match.captured(1).toInt();
    match = pttSeasonEpisodeRx.match(text);
    if (match.hasMatch()) {
        out.season = match.captured(1).toInt();
        out.episode = match.captured(2).toInt();
    } else {
        out.season = matchInt(pttSeasonOnlyRx, text);
        out.episode = matchInt(pttEpisodeOnlyRx, text);
    }
    match = pttResolutionRx.match(text);
    if (match.hasMatch())
        out.resolution = match.captured(1);
    match = pttCodecRx.match(text);
    if (match.hasMatch())
        out.codec = match.captured(1);

    match = pttGroupDashRx.match(text);
    if (match.hasMatch())
        out.group = match.captured(1);
    else {
        match = pttGroupBracketRx.match(text);
        if (match.hasMatch())
            out.group = match.captured(1);
    }
    out.group.replace(pttGroupExtensionRx, QString());

    out.proper = pttProperRx.match(text).hasMatch();
    out.repack = pttRepackRx.match(text).hasMatch();
    out.extended = pttExtendedRx.match(text).hasMatch();
    out.unrated = pttUnratedRx.match(text).hasMatch();
    out.theatrical = pttTheatricalRx.match(text).hasMatch();
    out.uncut = pttUncutRx.match(text).hasMatch();
    out.remastered = pttRemasteredRx.match(text).hasMatch();
    out.criterion = pttCriterionRx.match(text).hasMatch();
    out.openmatte = pttOpenMatteRx.match(text).hasMatch();
    out.hardcoded = pttHardcodedRx.match(text).hasMatch();

    match = pttChannelsRx.match(text);
    if (match.hasMatch())
        out.channels = mapChannels(match.captured(1));
    match = pttBitDepthRx.match(text);
    if (match.hasMatch())
        out.bitDepth = match.captured(1).toInt();

    int stop = text.length();
    match = titleStopRx.match(text);
    if (match.hasMatch())
        stop = match.capturedStart();
    QString title = text.left(stop);
    title.replace(pttGroupBracketRx, QString());
    title.replace(titleCleanRx, QStringLiteral(" "));
    title.replace(titleSpaceRx, QStringLiteral(" "));
    out.title = title.trimmed();
    return out;
}

Resolution mapResolution(const QString& value)
{
    if (value.isEmpty())
        return Resolution::SD;
    const QString lower = value.toLower();
    if (lower.contains(QStringLiteral("2160")) || lower == QStringLiteral("4k")
        || lower == QStringLiteral("uhd"))
        return Resolution::FourK;
    if (lower.contains(QStringLiteral("1080")))
        return Resolution::FullHD;
    if (lower.contains(QStringLiteral("720")))
        return Resolution::HD;
    if (lower.contains(QStringLiteral("480")))
        return Resolution::SD_480;
    return Resolution::SD;
}

Codec mapCodec(const QString& value)
{
    const QString lower = value.toLower();
    if (lower.contains(QStringLiteral("265")) || lower == QStringLiteral("hevc"))
        return Codec::HEVC;
    if (lower.contains(QStringLiteral("264")) || lower == QStringLiteral("avc"))
        return Codec::AVC;
    if (lower.contains(QStringLiteral("av1")))
        return Codec::AV1;
    if (lower.contains(QStringLiteral("vp9")))
        return Codec::VP9;
    if (lower.contains(QStringLiteral("mpeg")))
        return Codec::MPEG2;
    return Codec::Other;
}

HdrFormat detectHdr(const QString& text)
{
    if (hdrDvHdr10Rx.match(text).hasMatch())
        return HdrFormat::DV_HDR10;
    if (hdrDvRx.match(text).hasMatch())
        return HdrFormat::DV;
    if (hdrHdr10PlusRx.match(text).hasMatch())
        return HdrFormat::HDR10Plus;
    if (hdrHlgRx.match(text).hasMatch())
        return HdrFormat::HLG;
    if (hdrHdr10Rx.match(text).hasMatch())
        return HdrFormat::HDR10;
    return HdrFormat::None;
}

Source detectSource(const QString& text)
{
    if (sourceHcCamRx.match(text).hasMatch())
        return Source::CAM;
    if (sourceCamRx.match(text).hasMatch())
        return Source::CAM;
    if (sourceHdtsRx.match(text).hasMatch())
        return Source::HDTS;
    if (sourceTsRx.match(text).hasMatch())
        return Source::TS;
    if (sourceTcRx.match(text).hasMatch())
        return Source::TC;
    if (sourceScrRx.match(text).hasMatch())
        return Source::SCR;
    if (sourceRemuxRx.match(text).hasMatch())
        return Source::REMUX;
    if (sourceBlurayRx.match(text).hasMatch())
        return Source::BluRay;
    if (sourceWebDlRx.match(text).hasMatch())
        return Source::WEB_DL;
    if (sourceWebRipRx.match(text).hasMatch())
        return Source::WEBRip;
    if (sourceHdripRx.match(text).hasMatch())
        return Source::HDRip;
    if (sourceDvdripRx.match(text).hasMatch())
        return Source::DVDRip;
    if (sourceHdtvRx.match(text).hasMatch())
        return Source::HDTV;
    if (sourceWebRx.match(text).hasMatch())
        return Source::WEB_DL;
    return Source::Other;
}

AudioInfo parseAudio(const QString& text, const PttResult& ptt)
{
    AudioInfo audio;
    audio.codec = AudioCodec::Other;
    if (audioAtmosRx.match(text).hasMatch())
        audio.codec = AudioCodec::Atmos;
    else if (audioTrueHdRx.match(text).hasMatch())
        audio.codec = AudioCodec::TrueHD;
    else if (audioDtsHdMaRx.match(text).hasMatch())
        audio.codec = AudioCodec::DTS_HD_MA;
    else if (audioDtsRx.match(text).hasMatch())
        audio.codec = AudioCodec::DTS;
    else if (audioDdPlusRx.match(text).hasMatch())
        audio.codec = AudioCodec::DDPlus;
    else if (audioAc3Rx.match(text).hasMatch())
        audio.codec = AudioCodec::AC3;
    else if (audioAacRx.match(text).hasMatch())
        audio.codec = AudioCodec::AAC;
    else if (audioFlacRx.match(text).hasMatch())
        audio.codec = AudioCodec::FLAC;
    else if (audioOpusRx.match(text).hasMatch())
        audio.codec = AudioCodec::Opus;

    QRegularExpressionMatch match = channelsRx.match(text);
    audio.channels = match.hasMatch() ? mapChannels(match.captured(1)) : ptt.channels;
    match = bitDepthRx.match(text);
    audio.bitDepth = match.hasMatch() ? match.captured(1).toInt() : ptt.bitDepth;
    return audio;
}

const QHash<QString, QString> langTokens = {
    {QStringLiteral("ENG"), QStringLiteral("English")},
    {QStringLiteral("ENGLISH"), QStringLiteral("English")},
    {QStringLiteral("ITA"), QStringLiteral("Italian")},
    {QStringLiteral("ITALIAN"), QStringLiteral("Italian")},
    {QStringLiteral("RUS"), QStringLiteral("Russian")},
    {QStringLiteral("RUSSIAN"), QStringLiteral("Russian")},
    {QStringLiteral("HIN"), QStringLiteral("Hindi")},
    {QStringLiteral("HINDI"), QStringLiteral("Hindi")},
    {QStringLiteral("ESP"), QStringLiteral("Spanish")},
    {QStringLiteral("SPA"), QStringLiteral("Spanish")},
    {QStringLiteral("SPANISH"), QStringLiteral("Spanish")},
    {QStringLiteral("LAT"), QStringLiteral("Spanish (Latin America)")},
    {QStringLiteral("LATINO"), QStringLiteral("Spanish (Latin America)")},
    {QStringLiteral("LATAM"), QStringLiteral("Spanish (Latin America)")},
    {QStringLiteral("CASTELLANO"), QStringLiteral("Spanish")},
    {QStringLiteral("KOR"), QStringLiteral("Korean")},
    {QStringLiteral("KOREAN"), QStringLiteral("Korean")},
    {QStringLiteral("JPN"), QStringLiteral("Japanese")},
    {QStringLiteral("JAPANESE"), QStringLiteral("Japanese")},
    {QStringLiteral("JAP"), QStringLiteral("Japanese")},
    {QStringLiteral("CHN"), QStringLiteral("Chinese")},
    {QStringLiteral("CHI"), QStringLiteral("Chinese")},
    {QStringLiteral("CHINESE"), QStringLiteral("Chinese")},
    {QStringLiteral("ZHO"), QStringLiteral("Chinese")},
    {QStringLiteral("MAN"), QStringLiteral("Chinese")},
    {QStringLiteral("MANDARIN"), QStringLiteral("Chinese")},
    {QStringLiteral("CANTONESE"), QStringLiteral("Chinese")},
    {QStringLiteral("POR"), QStringLiteral("Portuguese")},
    {QStringLiteral("PORTUGUESE"), QStringLiteral("Portuguese")},
    {QStringLiteral("PTBR"), QStringLiteral("Portuguese")},
    {QStringLiteral("DUBLADO"), QStringLiteral("Portuguese")},
    {QStringLiteral("GER"), QStringLiteral("German")},
    {QStringLiteral("GERMAN"), QStringLiteral("German")},
    {QStringLiteral("DEU"), QStringLiteral("German")},
    {QStringLiteral("FRA"), QStringLiteral("French")},
    {QStringLiteral("FRENCH"), QStringLiteral("French")},
    {QStringLiteral("FRE"), QStringLiteral("French")},
    {QStringLiteral("VFF"), QStringLiteral("French")},
    {QStringLiteral("VFQ"), QStringLiteral("French")},
    {QStringLiteral("VOSTFR"), QStringLiteral("French")},
    {QStringLiteral("TUR"), QStringLiteral("Turkish")},
    {QStringLiteral("TURKISH"), QStringLiteral("Turkish")},
    {QStringLiteral("ARA"), QStringLiteral("Arabic")},
    {QStringLiteral("ARABIC"), QStringLiteral("Arabic")},
    {QStringLiteral("TAM"), QStringLiteral("Tamil")},
    {QStringLiteral("TAMIL"), QStringLiteral("Tamil")},
    {QStringLiteral("TEL"), QStringLiteral("Telugu")},
    {QStringLiteral("TELUGU"), QStringLiteral("Telugu")},
    {QStringLiteral("CES"), QStringLiteral("Czech")},
    {QStringLiteral("CZECH"), QStringLiteral("Czech")},
    {QStringLiteral("CZE"), QStringLiteral("Czech")},
    {QStringLiteral("DAN"), QStringLiteral("Danish")},
    {QStringLiteral("DANISH"), QStringLiteral("Danish")},
    {QStringLiteral("FIN"), QStringLiteral("Finnish")},
    {QStringLiteral("FINNISH"), QStringLiteral("Finnish")},
    {QStringLiteral("HEB"), QStringLiteral("Hebrew")},
    {QStringLiteral("HEBREW"), QStringLiteral("Hebrew")},
    {QStringLiteral("HUN"), QStringLiteral("Hungarian")},
    {QStringLiteral("HUNGARIAN"), QStringLiteral("Hungarian")},
    {QStringLiteral("NLD"), QStringLiteral("Dutch")},
    {QStringLiteral("DUTCH"), QStringLiteral("Dutch")},
    {QStringLiteral("DUT"), QStringLiteral("Dutch")},
    {QStringLiteral("NOR"), QStringLiteral("Norwegian")},
    {QStringLiteral("NORWEGIAN"), QStringLiteral("Norwegian")},
    {QStringLiteral("POL"), QStringLiteral("Polish")},
    {QStringLiteral("POLISH"), QStringLiteral("Polish")},
    {QStringLiteral("RON"), QStringLiteral("Romanian")},
    {QStringLiteral("ROMANIAN"), QStringLiteral("Romanian")},
    {QStringLiteral("ROM"), QStringLiteral("Romanian")},
    {QStringLiteral("SWE"), QStringLiteral("Swedish")},
    {QStringLiteral("SWEDISH"), QStringLiteral("Swedish")},
    {QStringLiteral("THA"), QStringLiteral("Thai")},
    {QStringLiteral("THAI"), QStringLiteral("Thai")},
    {QStringLiteral("UKR"), QStringLiteral("Ukrainian")},
    {QStringLiteral("UKRAINIAN"), QStringLiteral("Ukrainian")},
    {QStringLiteral("VIE"), QStringLiteral("Vietnamese")},
    {QStringLiteral("VIETNAMESE"), QStringLiteral("Vietnamese")},
};

const QHash<QString, QString> flagToLanguage = {
    {QStringLiteral("US"), QStringLiteral("English")}, {QStringLiteral("GB"), QStringLiteral("English")},
    {QStringLiteral("CA"), QStringLiteral("English")}, {QStringLiteral("AU"), QStringLiteral("English")},
    {QStringLiteral("NZ"), QStringLiteral("English")}, {QStringLiteral("IE"), QStringLiteral("English")},
    {QStringLiteral("ES"), QStringLiteral("Spanish")}, {QStringLiteral("MX"), QStringLiteral("Spanish")},
    {QStringLiteral("AR"), QStringLiteral("Spanish")}, {QStringLiteral("CO"), QStringLiteral("Spanish")},
    {QStringLiteral("PE"), QStringLiteral("Spanish")}, {QStringLiteral("CL"), QStringLiteral("Spanish")},
    {QStringLiteral("IT"), QStringLiteral("Italian")}, {QStringLiteral("DE"), QStringLiteral("German")},
    {QStringLiteral("AT"), QStringLiteral("German")}, {QStringLiteral("CH"), QStringLiteral("German")},
    {QStringLiteral("FR"), QStringLiteral("French")}, {QStringLiteral("BE"), QStringLiteral("French")},
    {QStringLiteral("LU"), QStringLiteral("French")}, {QStringLiteral("JP"), QStringLiteral("Japanese")},
    {QStringLiteral("KR"), QStringLiteral("Korean")}, {QStringLiteral("KP"), QStringLiteral("Korean")},
    {QStringLiteral("CN"), QStringLiteral("Chinese")}, {QStringLiteral("HK"), QStringLiteral("Chinese")},
    {QStringLiteral("TW"), QStringLiteral("Chinese")}, {QStringLiteral("SG"), QStringLiteral("Chinese")},
    {QStringLiteral("PT"), QStringLiteral("Portuguese")}, {QStringLiteral("BR"), QStringLiteral("Portuguese")},
    {QStringLiteral("RU"), QStringLiteral("Russian")}, {QStringLiteral("BY"), QStringLiteral("Russian")},
    {QStringLiteral("IN"), QStringLiteral("Hindi")}, {QStringLiteral("PK"), QStringLiteral("Hindi")},
    {QStringLiteral("SA"), QStringLiteral("Arabic")}, {QStringLiteral("AE"), QStringLiteral("Arabic")},
    {QStringLiteral("EG"), QStringLiteral("Arabic")}, {QStringLiteral("IQ"), QStringLiteral("Arabic")},
    {QStringLiteral("JO"), QStringLiteral("Arabic")}, {QStringLiteral("KW"), QStringLiteral("Arabic")},
    {QStringLiteral("LB"), QStringLiteral("Arabic")}, {QStringLiteral("MA"), QStringLiteral("Arabic")},
    {QStringLiteral("QA"), QStringLiteral("Arabic")}, {QStringLiteral("SY"), QStringLiteral("Arabic")},
    {QStringLiteral("TN"), QStringLiteral("Arabic")}, {QStringLiteral("IL"), QStringLiteral("Hebrew")},
    {QStringLiteral("TR"), QStringLiteral("Turkish")}, {QStringLiteral("NL"), QStringLiteral("Dutch")},
    {QStringLiteral("NO"), QStringLiteral("Norwegian")}, {QStringLiteral("PL"), QStringLiteral("Polish")},
    {QStringLiteral("RO"), QStringLiteral("Romanian")}, {QStringLiteral("MD"), QStringLiteral("Romanian")},
    {QStringLiteral("SE"), QStringLiteral("Swedish")}, {QStringLiteral("DK"), QStringLiteral("Danish")},
    {QStringLiteral("FI"), QStringLiteral("Finnish")}, {QStringLiteral("CZ"), QStringLiteral("Czech")},
    {QStringLiteral("HU"), QStringLiteral("Hungarian")}, {QStringLiteral("TH"), QStringLiteral("Thai")},
    {QStringLiteral("UA"), QStringLiteral("Ukrainian")}, {QStringLiteral("VN"), QStringLiteral("Vietnamese")},
    {QStringLiteral("GR"), QStringLiteral("Greek")}, {QStringLiteral("ID"), QStringLiteral("Indonesian")},
    {QStringLiteral("MY"), QStringLiteral("Malay")}, {QStringLiteral("PH"), QStringLiteral("Tagalog")},
    {QStringLiteral("IR"), QStringLiteral("Persian")},
};

const QHash<QString, QString> isoPairToLanguage = {
    {QStringLiteral("EN"), QStringLiteral("English")}, {QStringLiteral("GB"), QStringLiteral("English")},
    {QStringLiteral("US"), QStringLiteral("English")}, {QStringLiteral("CA"), QStringLiteral("English")},
    {QStringLiteral("AU"), QStringLiteral("English")}, {QStringLiteral("NZ"), QStringLiteral("English")},
    {QStringLiteral("ES"), QStringLiteral("Spanish")}, {QStringLiteral("MX"), QStringLiteral("Spanish")},
    {QStringLiteral("IT"), QStringLiteral("Italian")}, {QStringLiteral("DE"), QStringLiteral("German")},
    {QStringLiteral("FR"), QStringLiteral("French")}, {QStringLiteral("PT"), QStringLiteral("Portuguese")},
    {QStringLiteral("BR"), QStringLiteral("Portuguese")}, {QStringLiteral("RU"), QStringLiteral("Russian")},
    {QStringLiteral("JA"), QStringLiteral("Japanese")}, {QStringLiteral("JP"), QStringLiteral("Japanese")},
    {QStringLiteral("KO"), QStringLiteral("Korean")}, {QStringLiteral("KR"), QStringLiteral("Korean")},
    {QStringLiteral("ZH"), QStringLiteral("Chinese")}, {QStringLiteral("CN"), QStringLiteral("Chinese")},
    {QStringLiteral("TW"), QStringLiteral("Chinese")}, {QStringLiteral("HK"), QStringLiteral("Chinese")},
    {QStringLiteral("HI"), QStringLiteral("Hindi")}, {QStringLiteral("AR"), QStringLiteral("Arabic")},
    {QStringLiteral("SA"), QStringLiteral("Arabic")}, {QStringLiteral("AE"), QStringLiteral("Arabic")},
    {QStringLiteral("EG"), QStringLiteral("Arabic")}, {QStringLiteral("TR"), QStringLiteral("Turkish")},
    {QStringLiteral("NL"), QStringLiteral("Dutch")}, {QStringLiteral("PL"), QStringLiteral("Polish")},
    {QStringLiteral("RO"), QStringLiteral("Romanian")}, {QStringLiteral("SV"), QStringLiteral("Swedish")},
    {QStringLiteral("SE"), QStringLiteral("Swedish")}, {QStringLiteral("DA"), QStringLiteral("Danish")},
    {QStringLiteral("FI"), QStringLiteral("Finnish")}, {QStringLiteral("CS"), QStringLiteral("Czech")},
    {QStringLiteral("CZ"), QStringLiteral("Czech")}, {QStringLiteral("HU"), QStringLiteral("Hungarian")},
    {QStringLiteral("TH"), QStringLiteral("Thai")}, {QStringLiteral("UK"), QStringLiteral("Ukrainian")},
    {QStringLiteral("UA"), QStringLiteral("Ukrainian")}, {QStringLiteral("VI"), QStringLiteral("Vietnamese")},
    {QStringLiteral("IL"), QStringLiteral("Hebrew")}, {QStringLiteral("HE"), QStringLiteral("Hebrew")},
    {QStringLiteral("GR"), QStringLiteral("Greek")}, {QStringLiteral("ID"), QStringLiteral("Indonesian")},
    {QStringLiteral("MY"), QStringLiteral("Malay")}, {QStringLiteral("PH"), QStringLiteral("Tagalog")},
    {QStringLiteral("IR"), QStringLiteral("Persian")}, {QStringLiteral("FA"), QStringLiteral("Persian")},
};

void appendLanguage(QStringList& out, QSet<QString>& seen, const QString& language)
{
    if (language.isEmpty() || seen.contains(language))
        return;
    seen.insert(language);
    out.push_back(language);
}

QStringList parseLanguages(const QString& text)
{
    QStringList out;
    QSet<QString> seen;

    QRegularExpressionMatchIterator it = langRx.globalMatch(text);
    while (it.hasNext()) {
        const QString upper = it.next().captured(1).toUpper();
        if (langTokens.contains(upper))
            appendLanguage(out, seen, langTokens.value(upper));
        else if (upper == QStringLiteral("MULTI") || upper == QStringLiteral("DUAL"))
            appendLanguage(out, seen, QStringLiteral("Multi"));
    }

    it = flagRx.globalMatch(text);
    while (it.hasNext()) {
        const QString flag = it.next().captured(0);
        if (flag.size() < 4)
            continue;
        const uint a = flag.at(0).isHighSurrogate()
            ? QChar::surrogateToUcs4(flag.at(0), flag.at(1))
            : flag.at(0).unicode();
        const uint b = flag.at(2).isHighSurrogate()
            ? QChar::surrogateToUcs4(flag.at(2), flag.at(3))
            : flag.at(2).unicode();
        const QString code = QString(QChar(ushort(a - 0x1F1E6 + 65)))
            + QString(QChar(ushort(b - 0x1F1E6 + 65)));
        appendLanguage(out, seen, flagToLanguage.value(code));
    }

    it = isoPairRx.globalMatch(text);
    while (it.hasNext())
        appendLanguage(out, seen, isoPairToLanguage.value(it.next().captured(1).toUpper()));

    QStringList concrete;
    for (const QString& language : out) {
        if (language != QStringLiteral("Multi"))
            concrete.push_back(language);
    }
    if (concrete.size() > 1) {
        concrete.prepend(QStringLiteral("Multi"));
        return concrete;
    }
    if (concrete.size() == 1)
        return concrete;
    return seen.contains(QStringLiteral("Multi")) ? QStringList{QStringLiteral("Multi")} : QStringList{};
}

Container parseContainer(const QString& filenameHint, const QString& filenameLine, const QString& text)
{
    for (const QString& src : {filenameHint, filenameLine, text}) {
        if (src.isEmpty())
            continue;
        const QRegularExpressionMatch match = containerRx.match(src);
        if (!match.hasMatch())
            continue;
        const QString c = match.captured(1).toLower();
        if (c == QStringLiteral("mkv"))
            return Container::MKV;
        if (c == QStringLiteral("mp4"))
            return Container::MP4;
        if (c == QStringLiteral("m4v"))
            return Container::M4V;
        if (c == QStringLiteral("avi"))
            return Container::AVI;
        if (c == QStringLiteral("webm"))
            return Container::WebM;
        if (c == QStringLiteral("mov"))
            return Container::MOV;
        if (c == QStringLiteral("ts"))
            return Container::TS;
        if (c == QStringLiteral("wmv"))
            return Container::WMV;
    }
    return Container::Unknown;
}

qint64 parseSize(const QString& text, qint64 hint)
{
    if (hint > 0)
        return hint;
    const QRegularExpressionMatch match = sizeRx.match(text);
    if (!match.hasMatch())
        return 0;
    const double n = match.captured(1).toDouble();
    const QString unit = match.captured(2).toLower();
    if (unit.startsWith(QLatin1Char('t')))
        return qRound64(n * std::pow(1024.0, 4));
    if (unit.startsWith(QLatin1Char('g')))
        return qRound64(n * std::pow(1024.0, 3));
    if (unit.startsWith(QLatin1Char('m')))
        return qRound64(n * std::pow(1024.0, 2));
    return 0;
}

int parseSeeders(const QString& text)
{
    QRegularExpressionMatch match = seedersRx.match(text);
    if (!match.hasMatch())
        match = seedersEmojiRx.match(text);
    return match.hasMatch() ? match.captured(1).toInt() : -1;
}

QString parseEdition(const QString& text, const PttResult& ptt)
{
    if (ptt.extended)
        return QStringLiteral("EXTENDED");
    if (ptt.unrated)
        return QStringLiteral("UNRATED");
    if (ptt.theatrical)
        return QStringLiteral("THEATRICAL");
    if (ptt.uncut)
        return QStringLiteral("UNCUT");
    if (ptt.remastered)
        return QStringLiteral("REMASTERED");
    if (ptt.criterion)
        return QStringLiteral("CRITERION");
    if (ptt.openmatte)
        return QStringLiteral("OPEN MATTE");
    const QRegularExpressionMatch match = editionRx.match(text);
    return match.hasMatch()
        ? QString(match.captured(1).toUpper()).replace(editionNormalizeRx, QStringLiteral(" "))
        : QString();
}

QPair<int, int> parseYearRange(const QString& text)
{
    const QRegularExpressionMatch match = yearRangeRx.match(text);
    if (!match.hasMatch())
        return {0, 0};
    const int a = match.captured(1).toInt();
    const int b = match.captured(2).toInt();
    return (b - a > 0 && b - a < 30) ? QPair<int, int>{a, b} : QPair<int, int>{0, 0};
}

bool parseSeasonPack(const QString& text, const PttResult& ptt)
{
    return ptt.season > 0 && ptt.episode == 0 && seasonPackRx.match(text).hasMatch();
}

int parseDisc(const QString& text)
{
    const QRegularExpressionMatch match = discRx.match(text);
    return match.hasMatch() ? match.captured(1).toInt() : -1;
}

int parseRepackIteration(const QString& text, const PttResult& ptt)
{
    const QRegularExpressionMatch match = repackRx.match(text);
    if (!match.hasMatch())
        return ptt.repack ? 1 : 0;
    return match.captured(1).isEmpty() ? 1 : match.captured(1).toInt();
}

QString parseAnimeHash(const QString& text)
{
    const QRegularExpressionMatch match = animeHashRx.match(text);
    return match.hasMatch() ? match.captured(1).toUpper() : QString();
}

int computeScamScore(Source source, Resolution resolution, qint64 size)
{
    int score = 0;
    if (resolution == Resolution::FourK && size > 0 && size < 5ll * 1024 * 1024 * 1024)
        score += 3;
    if (resolution == Resolution::FullHD && size > 0 && size < 700ll * 1024 * 1024)
        score += 3;
    if (resolution == Resolution::HD && size > 0 && size < 250ll * 1024 * 1024)
        score += 3;
    if ((resolution == Resolution::SD || resolution == Resolution::SD_480)
        && size > 0 && size < 250ll * 1024 * 1024)
        score += 3;
    if ((resolution == Resolution::SD || resolution == Resolution::SD_480)
        && source == Source::Other)
        score += 2;
    return score;
}

QString parseEpisodeTitle(const QString& filename, int season, int episode)
{
    if (season <= 0 || episode <= 0)
        return QString();
    const QString code = QStringLiteral("S%1E%2")
        .arg(season, 2, 10, QLatin1Char('0'))
        .arg(episode, 2, 10, QLatin1Char('0'));
    const int idx = filename.toUpper().indexOf(code);
    if (idx < 0)
        return QString();
    QString after = filename.mid(idx + code.length()).replace(leadingSepRx, QString());
    const QRegularExpressionMatch stop = qualityStopRx.match(after);
    if (stop.hasMatch() && stop.capturedStart() > 0)
        after = after.left(stop.capturedStart());
    const QString cleaned = after.replace(separatorRx, QStringLiteral(" "))
        .replace(titleSpaceRx, QStringLiteral(" "))
        .trimmed();
    if (cleaned.length() < 2 || cleaned.length() > 80 || episodeJunkRx.match(cleaned).hasMatch())
        return QString();
    return cleaned;
}

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

bool isTrustedGroup(const QString& normalized)
{
    return !normalized.isEmpty() && trustedGroups.contains(normalized);
}

} // namespace

bool validateStreamParserRegexes(QStringList* errors)
{
    bool ok = true;
    for (const QRegularExpression* rx : allRegexes()) {
        if (rx->isValid())
            continue;
        ok = false;
        if (errors) {
            errors->push_back(QStringLiteral("%1 | offset=%2 | %3")
                                  .arg(rx->pattern())
                                  .arg(rx->patternErrorOffset())
                                  .arg(rx->errorString()));
        }
    }
    return ok;
}

ParsedStream StreamParser::parseStream(const Stream& stream)
{
    const QString filenameLine = extractFilenameLine(stream);
    QStringList parts;
    if (!filenameLine.isEmpty())
        parts.append(filenameLine);
    if (!stream.title.isEmpty())
        parts.append(stream.title);
    if (!stream.description.isEmpty())
        parts.append(stream.description);
    if (!stream.name.isEmpty())
        parts.append(stream.name);
    const QString text = parts.join(QLatin1Char(' '));
    const PttResult ptt = parsePtt(!filenameLine.isEmpty() ? filenameLine : text);

    ParsedStream out;
    static_cast<Stream&>(out) = stream;

    out.parsedTitle = !ptt.title.isEmpty()
        ? ptt.title
        : (!filenameLine.isEmpty() ? filenameLine.left(100) : text.left(100));
    out.resolution = mapResolution(ptt.resolution);
    out.hdrFormat = detectHdr(text);
    out.codec = mapCodec(ptt.codec);
    out.source = detectSource(text);
    out.audio = parseAudio(text, ptt);
    out.audioLanguages = parseLanguages(text);
    out.size = parseSize(text, stream.videoSize);
    out.seeders = parseSeeders(text);
    out.cached.clear();
    out.inLibrary.clear();
    out.container = parseContainer(stream.filename, filenameLine, text);
    out.releaseGroup = ptt.group;
    out.releaseGroupNormalized = ptt.group.isEmpty()
        ? QString()
        : QString(ptt.group.toUpper()).replace(nonAlnumRx, QString());
    out.remux = remuxRx.match(text).hasMatch();
    out.edition = parseEdition(text, ptt);
    out.year = ptt.year;
    const QPair<int, int> range = parseYearRange(text);
    out.yearStart = range.first;
    out.yearEnd = range.second;
    out.season = ptt.season;
    out.episode = ptt.episode;
    out.seasonPack = parseSeasonPack(text, ptt);
    out.episodeTitle = parseEpisodeTitle(filenameLine, ptt.season, ptt.episode);
    out.discIndex = parseDisc(text);
    out.repackIteration = parseRepackIteration(text, ptt);
    out.proper = ptt.proper;
    out.hardcoded = hardcodedRx.match(text).hasMatch() || ptt.hardcoded;
    out.animeHash = parseAnimeHash(text);
    out.scamScore = computeScamScore(out.source, out.resolution, out.size);
    Q_UNUSED(isTrustedGroup(out.releaseGroupNormalized));
    return out;
}

} // namespace tankoban
