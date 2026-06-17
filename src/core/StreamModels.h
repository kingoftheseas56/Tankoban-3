// Tankoban 3 - StreamModels (Slice 5).
// Faithful adapter for Harbor's src/lib/streams/types.ts:
// TypeScript intersections and nullable/undefined fields mapped onto C++ structs.
// Pipeline: raw JSON -> Stream -> parse -> ParsedStream -> score
//           -> ScoredStream -> rank -> RankedPicker
//
// Sentinel convention: C++ fields use explicit "unknown" values where Harbor uses
// null/undefined: Resolution::None, HdrFormat::None, Codec/AudioCodec/Source::Unknown,
// Container::Unknown, empty QString for nullable strings, size=0, seeders=-1,
// year/yearStart/yearEnd=0, season/episode=0, discIndex=-1, and std::optional
// RankedPicker::primary. Parser/scorer code must treat these as unknown/no-op, not
// meaningful data.
#pragma once

#include <optional>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVector>

namespace tankoban {

// -- enums (Harbor values plus C++ sentinels where needed) -------------

enum class Resolution {
    None,
    FourK,      // "4K"
    FullHD,     // "1080p"
    HD,         // "720p"
    SD_480,     // "480p"
    SD          // "SD" (Harbor has both 480p and SD as distinct)
};

enum class HdrFormat {
    None,
    HDR10,
    HDR10Plus,  // "HDR10+"
    DV,         // "DV"
    DV_HDR10,   // "DV+HDR10"
    HLG
};

enum class Codec {
    Unknown,
    HEVC,
    AVC,
    AV1,
    VP9,
    MPEG2,
    Other
};

enum class AudioCodec {
    Unknown,
    Atmos,
    TrueHD,
    DTS_HD_MA,  // "DTS-HD MA"
    DTS,
    DDPlus,     // "DD+"
    AC3,
    AAC,
    Opus,
    FLAC,
    Other
};

enum class Source {
    Unknown,
    BluRay,
    REMUX,
    WEB_DL,     // "WEB-DL"
    WEBRip,
    BDRip,
    HDRip,
    DVDRip,
    HDTV,
    CAM,
    TS,
    HDTS,
    TC,
    SCR,
    Other
};

enum class Tier {
    FourK_DV,       // "4K_DV"
    FourK_HDR,      // "4K_HDR"
    FourK,          // "4K"
    FullHD_HDR,     // "1080p_HDR"
    FullHD,         // "1080p"
    HD,             // "720p"
    SD,             // "SD"
    ROUGH           // "ROUGH"
};

enum class Container {
    Unknown,
    MKV,
    MP4,
    M4V,
    AVI,
    WebM,
    MOV,
    TS,             // MPEG-TS
    WMV
};

// DEFERRED (debrid not in v1 scope per ROADMAP Slice 5: "No debrid"):
enum class DebridSlug {
    None,
    RD,     // Real-Debrid
    TB,     // TorBox
    AD,     // AllDebrid
    PM,     // Premiumize
    DL      // Debrid-Link
};

// -- subtitle attached to a stream ------------------------------------
// Harbor-shaped StreamSubtitle.
// url is raw QString (addon JSON may have odd/encoded URLs;
// normalize to QUrl later in resolver/player handoff).
struct StreamSubtitle {
    QString id;
    QString url;        // raw string from addon JSON - NOT QUrl
    QString lang;       // Harbor uses "lang" not "language"
    QString m;          // optional metadata string
};

// -- audio info (parsed) -----------------------------------------------
// Harbor-shaped AudioInfo.
struct AudioInfo {
    AudioCodec codec = AudioCodec::Unknown;
    int channels = 2;
    int bitDepth = 0;   // 0 = unknown
};

// -- contributor (from stream.addons[i].contributors) ------------------
struct Contributor {
    QString id;
    QString name;
};

// -- proxy headers (Harbor: { request?, response? }) -------------------
struct ProxyHeaders {
    QHash<QString, QString> request;
    QHash<QString, QString> response;
};

// -- TIER 1: raw stream from an addon ----------------------------------
// Harbor-shaped Stream type; behaviorHints is flattened into explicit fields.
struct Stream {
    // from addon JSON
    QString name;
    QString title;
    QString description;
    QString infoHash;       // lowercase
    int fileIdx = -1;       // -1 = unspecified
    QString fileMustInclude;
    QString url;            // direct playable URL (empty / "#" = unsupported)
    QString ytId;
    QString externalUrl;
    QString nzbUrl;
    QStringList sources;    // tracker announce URLs
    QStringList servers;    // debrid-style server URLs
    // archive URLs
    QStringList rarUrls;
    QStringList zipUrls;
    QStringList tarUrls;
    QStringList tgzUrls;
    QStringList sevenZipUrls;
    QVector<StreamSubtitle> subtitles;

    // behaviorHints sub-object (Harbor: behaviorHints?: { ... } & Record<string,unknown>)
    QString bingeGroup;
    QString videoHash;
    qint64 videoSize = 0;
    QString filename;       // Harbor canonical
    QString fileName;       // Harbor alternate spelling (both accepted)
    QStringList countryWhitelist;
    bool notWebReady = false;
    ProxyHeaders proxyHeaders;  // { request, response }
    QHash<QString, QString> headers;  // separate from proxyHeaders.request

    // addon provenance (attached by fetchAddonStreams)
    QString addonId;
    QString addonName;
    QUrl addonUrl;
    bool addonRanked = false;
    int addonPriority = 0;
    QVector<Contributor> contributors;

    // extra
    int availability = 0;
    bool liveStreamCheck = false;
};

// -- TIER 2: parsed stream ---------------------------------------------
// Harbor ParsedStream = Stream & { parsedTitle, ... }, adapted with C++ sentinels.
struct ParsedStream : Stream {
    QString parsedTitle;
    QString episodeTitle;       // null = empty
    Resolution resolution = Resolution::None;
    HdrFormat hdrFormat = HdrFormat::None;
    Codec codec = Codec::Unknown;
    Source source = Source::Unknown;
    AudioInfo audio;
    QStringList audioLanguages;
    qint64 size = 0;            // bytes, 0 = unknown
    int seeders = -1;           // -1 = unknown
    Container container = Container::Unknown;
    QString releaseGroup;
    QString releaseGroupNormalized; // uppercase, stripped non-alnum
    bool remux = false;
    QString edition;
    int year = 0;
    int yearStart = 0;
    int yearEnd = 0;
    int season = 0;
    int episode = 0;
    bool seasonPack = false;
    int discIndex = -1;
    int repackIteration = 0;
    bool proper = false;
    bool hardcoded = false;
    QString animeHash;
    int scamScore = 0;

    // DEFERRED - debrid not in v1 scope (ROADMAP Slice 5: "No debrid")
    QHash<int, bool> cached;        // DebridSlug -> isCached
    QHash<int, bool> inLibrary;     // DebridSlug -> isInLibrary
};

// -- one scoring reason ------------------------------------------------
// Harbor-shaped ScoreReason.
struct ScoreReason {
    QString signal;     // e.g. "cached", "CAM penalty"
    int delta = 0;
};

// -- TIER 3: scored stream ---------------------------------------------
// Harbor ScoredStream = ParsedStream & { score, reasons, tier }.
struct ScoredStream : ParsedStream {
    int score = 0;
    QVector<ScoreReason> reasons;
    Tier tier = Tier::ROUGH;
    int nativeIdx = -1;     // original index before sorting
};

// -- final ranked output -----------------------------------------------
// Harbor-shaped RankedPicker.
// primary is optional - Harbor has ScoredStream | null.
struct RankedPicker {
    std::optional<ScoredStream> primary;
    QHash<int, ScoredStream> byTier;    // Tier enum -> best stream
    QVector<ScoredStream> all;          // all scored streams, ranked
};

} // namespace tankoban
