#include "engine/MpvController.h"

#include <mpv/client.h>

#include <QByteArray>
#include <QDir>
#include <QStandardPaths>
#include <QVarLengthArray>

#include <cmath>
#include <cstring>
#include <vector>

// ── node helpers (track-list / chapter-list parsing) ─────────────────────────
namespace {
const mpv_node* mapGet(const mpv_node_list* m, const char* key)
{
    if (!m) return nullptr;
    for (int i = 0; i < m->num; ++i)
        if (m->keys[i] && std::strcmp(m->keys[i], key) == 0) return &m->values[i];
    return nullptr;
}
QString nStr(const mpv_node* n)
{
    return (n && n->format == MPV_FORMAT_STRING && n->u.string)
               ? QString::fromUtf8(n->u.string) : QString();
}
long long nInt(const mpv_node* n, long long d = 0)
{
    if (!n) return d;
    if (n->format == MPV_FORMAT_INT64) return n->u.int64;
    if (n->format == MPV_FORMAT_DOUBLE) return (long long)n->u.double_;
    return d;
}
bool nFlag(const mpv_node* n, bool d = false)
{
    return (n && n->format == MPV_FORMAT_FLAG) ? (n->u.flag != 0) : d;
}
double nDbl(const mpv_node* n, double d = 0)
{
    if (!n) return d;
    if (n->format == MPV_FORMAT_DOUBLE) return n->u.double_;
    if (n->format == MPV_FORMAT_INT64) return (double)n->u.int64;
    return d;
}
const char* endReasonStr(int reason)
{
    switch (reason) {
    case MPV_END_FILE_REASON_EOF:      return "eof";
    case MPV_END_FILE_REASON_STOP:     return "stop";
    case MPV_END_FILE_REASON_QUIT:     return "quit";
    case MPV_END_FILE_REASON_ERROR:    return "error";
    case MPV_END_FILE_REASON_REDIRECT: return "redirect";
    default:                           return "other";
    }
}
} // namespace

// ── lifecycle ────────────────────────────────────────────────────────────────
MpvController::MpvController(QObject* parent) : QObject(parent)
{
    connect(this, &MpvController::wakeup, this, &MpvController::onWakeup, Qt::QueuedConnection);
}

MpvController::~MpvController()
{
    if (m_mpv) { mpv_terminate_destroy(m_mpv); m_mpv = nullptr; }
}

void MpvController::initialize()
{
    if (m_initialized) return;
    m_mpv = mpv_create();
    if (!m_mpv) {
        m_snap.status = PlayerSnapshot::Error;
        m_snap.errorCode = PlayerSnapshot::Unknown;
        m_snap.errorMessage = QStringLiteral("mpv_create failed");
        emitSnapshot();
        return;
    }

    // PRE-INIT options (spec §3.1). Hermetic: ignore the user's local mpv config.
    setOpt("config", "no");
    setOpt("title", "Tankoban");
    setOpt("audio-client-name", "Tankoban");
    setOpt("terminal", "no");
    setOpt("msg-level", "all=warn,vo=v,gpu=v");
    setOpt("user-agent", "VLC/3.0.20 LibVLC/3.0.20");
    setOpt("input-default-bindings", "no");   // app owns all hotkeys
    setOpt("input-cursor", "no");
    setOpt("osc", "no");
    setOpt("osd-level", "0");
    setOpt("cursor-autohide", "200");
    setOpt("volume-max", "600");               // required, else mpv clamps writes at 100
    setOpt("background-color", "#000000");
    setOpt("background", "color");
    setOpt("media-controls", "yes");
    // render-API path (all platforms here — Qt owns the GL surface)
    setOpt("vo", "libmpv");
    setOpt("force-window", "no");
    setOpt("hwdec", "auto");

    mpv_request_log_messages(m_mpv, "warn");

    int rc = mpv_initialize(m_mpv);
    if (rc < 0) {
        m_snap.status = PlayerSnapshot::Error;
        m_snap.errorCode = PlayerSnapshot::Unknown;
        m_snap.errorMessage = QStringLiteral("mpv_initialize failed: %1")
                                  .arg(QString::fromUtf8(mpv_error_string(rc)));
        mpv_terminate_destroy(m_mpv);
        m_mpv = nullptr;
        emitSnapshot();
        return;
    }

    // POST-INIT cache / demuxer / network (spec §3.1)
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                       + QStringLiteral("/mpv-cache");
    QDir().mkpath(cacheDir);
    setStr("cache", "yes");
    setStr("cache-dir", cacheDir);
    setStr("cache-secs", "60");
    setStr("cache-on-disk", "yes");
    setStr("cache-pause", "yes");
    setStr("network-timeout", "600");
    setStr("demuxer-max-bytes", "128MiB");
    setStr("stream-buffer-size", "32MiB");
    setStr("demuxer-max-back-bytes", "32MiB");
    setStr("stream-lavf-o",
           "reconnect=1,reconnect_streamed=1,reconnect_delay_max=10,reconnect_on_network_error=1");
    setStr("demuxer-readahead-secs", "60");
    // subtitle base styling (libass draws natively; do NOT hide subs, §0.1 #4)
    setStr("sub-font-provider", "auto");
    setStr("sub-font", "Noto Sans JP");
    setStr("embeddedfonts", "yes");
    // TODO(plan-3): sub-fonts-dir once fonts are bundled.

    observeProperties();
    mpv_set_wakeup_callback(m_mpv, &MpvController::wakeupTrampoline, this);
    m_initialized = true;
}

// ── loading ──────────────────────────────────────────────────────────────────
void MpvController::load(const QString& url, double startSec)
{
    if (!m_initialized) initialize();
    if (!m_mpv) return;

    m_snap.status = PlayerSnapshot::Loading;
    m_snap.audioTracks.clear();
    m_snap.subtitleTracks.clear();
    m_snap.chapters.clear();
    m_snap.positionSec = 0;
    m_snap.durationSec = 0;
    m_snap.errorCode = PlayerSnapshot::None;
    m_snap.errorMessage.clear();
    emitSnapshot();

    const QByteArray u = url.toUtf8();
    if (m_started) command({"stop"});               // reuse the handle for a new source
    if (startSec > 0.0) {
        command({"loadfile", u, "replace", "0",
                 QByteArray("start=") + QByteArray::number(startSec, 'f', 3)});
    } else {
        command({"loadfile", u, "replace"});
    }
    m_started = true;
}

// ── transport (spec §4.2; §0.2 coercion in the set* helpers) ─────────────────
void MpvController::play()  { setFlag("pause", false); }
void MpvController::pause() { setFlag("pause", true); }
void MpvController::seek(double sec)
{
    command({"seek", QByteArray::number(sec, 'f', 6), "absolute", "exact"});
}
void MpvController::setVolume(double v) { setDouble("volume", std::round(v * 100.0)); }
void MpvController::setMuted(bool m)    { setFlag("mute", m); }
void MpvController::setRate(double r)   { setDouble("speed", r); m_snap.rate = r; emitSnapshot(); }
void MpvController::setAudioTrack(const QString& id)    { setStr("aid", id); }
void MpvController::setSubtitleTrack(const QString& id) { setStr("sid", id.isEmpty() ? QStringLiteral("no") : id); }
void MpvController::setSubVisible(bool on)   { setFlag("sub-visibility", on); }
void MpvController::setSubDelay(double s)    { setDouble("sub-delay", s); }
void MpvController::setAudioDelay(double s)  { setDouble("audio-delay", s); }
void MpvController::setPanscan(double v)     { setDouble("panscan", std::clamp(v, 0.0, 1.0)); }
void MpvController::setVideoZoom(double l)   { setDouble("video-zoom", l); }
void MpvController::setAspectOverride(const QString& r) { setStr("video-aspect-override", r); }

// ── low-level helpers ────────────────────────────────────────────────────────
void MpvController::setOpt(const char* name, const char* value)
{
    if (m_mpv) mpv_set_option_string(m_mpv, name, value);
}
void MpvController::setFlag(const char* name, bool v)
{
    if (!m_mpv) return;
    int f = v ? 1 : 0;
    mpv_set_property(m_mpv, name, MPV_FORMAT_FLAG, &f);
}
void MpvController::setDouble(const char* name, double v)
{
    if (m_mpv) mpv_set_property(m_mpv, name, MPV_FORMAT_DOUBLE, &v);
}
void MpvController::setStr(const char* name, const QString& v)
{
    if (m_mpv) mpv_set_property_string(m_mpv, name, v.toUtf8().constData());
}
void MpvController::command(std::initializer_list<QByteArray> args)
{
    if (!m_mpv) return;
    std::vector<QByteArray> hold(args.begin(), args.end());
    QVarLengthArray<const char*, 8> a;
    for (const auto& b : hold) a.append(b.constData());
    a.append(nullptr);                              // mpv_command wants a NULL terminator
    mpv_command(m_mpv, a.data());
}

void MpvController::observeProperties()
{
    mpv_observe_property(m_mpv, 1,  "time-pos",     MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 2,  "duration",     MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 3,  "pause",        MPV_FORMAT_FLAG);
    mpv_observe_property(m_mpv, 4,  "eof-reached",  MPV_FORMAT_FLAG);
    mpv_observe_property(m_mpv, 5,  "track-list",   MPV_FORMAT_NODE);
    mpv_observe_property(m_mpv, 6,  "volume",       MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 7,  "mute",         MPV_FORMAT_FLAG);
    mpv_observe_property(m_mpv, 8,  "chapter-list", MPV_FORMAT_NODE);
    mpv_observe_property(m_mpv, 9,  "sub-delay",    MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 10, "audio-delay",  MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 11, "sub-text",     MPV_FORMAT_STRING);  // ignored (libass renders)
    mpv_observe_property(m_mpv, 12, "sub-start",    MPV_FORMAT_DOUBLE);  // ignored
    mpv_observe_property(m_mpv, 13, "af",           MPV_FORMAT_STRING);
    mpv_observe_property(m_mpv, 14, "dwidth",       MPV_FORMAT_INT64);
    mpv_observe_property(m_mpv, 15, "dheight",      MPV_FORMAT_INT64);
}

// ── event pump (GUI thread) ──────────────────────────────────────────────────
void MpvController::wakeupTrampoline(void* ctx)
{
    // Runs on an arbitrary mpv thread: only emit the queued signal.
    auto* self = static_cast<MpvController*>(ctx);
    emit self->wakeup();
}

void MpvController::onWakeup()
{
    if (!m_mpv) return;
    while (true) {
        mpv_event* ev = mpv_wait_event(m_mpv, 0);
        if (!ev || ev->event_id == MPV_EVENT_NONE) break;
        switch (ev->event_id) {
        case MPV_EVENT_PROPERTY_CHANGE:
            handlePropertyChange(ev->data);
            break;
        case MPV_EVENT_END_FILE:
            handleEndFile(ev->data);
            break;
        case MPV_EVENT_FILE_LOADED:
            m_snap.status = PlayerSnapshot::Playing;
            m_snap.errorCode = PlayerSnapshot::None;
            m_snap.errorMessage.clear();
            emitSnapshot();
            break;
        default:
            break;   // playback-restart / seek / log etc. are no-ops (§0.2)
        }
    }
}

static void parseTracks(const mpv_node* arr, QVector<TrackInfo>& audio, QVector<TrackInfo>& subs)
{
    audio.clear(); subs.clear();
    if (!arr || arr->format != MPV_FORMAT_NODE_ARRAY || !arr->u.list) return;
    const mpv_node_list* list = arr->u.list;
    for (int i = 0; i < list->num; ++i) {
        const mpv_node* e = &list->values[i];
        if (e->format != MPV_FORMAT_NODE_MAP) continue;
        const mpv_node_list* m = e->u.list;
        const QString type = nStr(mapGet(m, "type"));
        TrackInfo t;
        t.id    = QString::number(nInt(mapGet(m, "id")));
        t.lang  = nStr(mapGet(m, "lang"));
        t.title = nStr(mapGet(m, "title"));
        t.codec = nStr(mapGet(m, "codec"));
        t.selected = nFlag(mapGet(m, "selected"));
        t.external = nFlag(mapGet(m, "external"));
        t.externalFilename = nStr(mapGet(m, "external-filename"));
        t.forced = nFlag(mapGet(m, "forced"));
        t.isDefault = nFlag(mapGet(m, "default"));
        t.hearingImpaired = nFlag(mapGet(m, "hearing-impaired"));
        t.channelCount = (int)nInt(mapGet(m, "demux-channel-count"));
        t.channels = nStr(mapGet(m, "demux-channels"));
        QString base = !t.title.isEmpty() ? t.title
                       : (!t.lang.isEmpty() ? t.lang : QStringLiteral("%1 %2").arg(type, t.id));
        t.label = base;
        if (type == QStringLiteral("audio"))     { t.kind = TrackInfo::Audio;    audio.push_back(t); }
        else if (type == QStringLiteral("sub"))  { t.kind = TrackInfo::Subtitle; subs.push_back(t); }
    }
}

static void parseChapters(const mpv_node* arr, QVector<Chapter>& out)
{
    out.clear();
    if (!arr || arr->format != MPV_FORMAT_NODE_ARRAY || !arr->u.list) return;
    const mpv_node_list* list = arr->u.list;
    for (int i = 0; i < list->num; ++i) {
        const mpv_node* e = &list->values[i];
        if (e->format != MPV_FORMAT_NODE_MAP) continue;
        const mpv_node_list* m = e->u.list;
        Chapter c;
        c.title = nStr(mapGet(m, "title"));
        c.startSec = nDbl(mapGet(m, "time"));
        if (std::isfinite(c.startSec) && c.startSec >= 0) out.push_back(c);
    }
}

void MpvController::handlePropertyChange(void* p)
{
    auto* prop = static_cast<mpv_event_property*>(p);
    if (!prop || !prop->name) return;
    const char* n = prop->name;
    void* d = prop->data;

    if      (!std::strcmp(n, "time-pos"))    { if (d) m_snap.positionSec = *(double*)d; }
    else if (!std::strcmp(n, "duration"))    { if (d) m_snap.durationSec = *(double*)d; }
    else if (!std::strcmp(n, "pause"))       { if (d) m_snap.status = (*(int*)d) ? PlayerSnapshot::Paused : PlayerSnapshot::Playing; }
    else if (!std::strcmp(n, "eof-reached")) { if (d && *(int*)d) m_snap.status = PlayerSnapshot::Ended; }
    else if (!std::strcmp(n, "track-list"))  { parseTracks((mpv_node*)d, m_snap.audioTracks, m_snap.subtitleTracks); }
    else if (!std::strcmp(n, "volume"))      { if (d) m_snap.volume = (*(double*)d) / 100.0; }
    else if (!std::strcmp(n, "mute"))        { if (d) m_snap.muted = (*(int*)d) != 0; }
    else if (!std::strcmp(n, "chapter-list")){ parseChapters((mpv_node*)d, m_snap.chapters); }
    else if (!std::strcmp(n, "sub-delay"))   { if (d) m_snap.subDelaySec = *(double*)d; }
    else if (!std::strcmp(n, "audio-delay")) { if (d) m_snap.audioDelaySec = *(double*)d; }
    else if (!std::strcmp(n, "af"))          { const char* s = d ? *(char**)d : nullptr; m_snap.audioNormalize = s && std::strstr(s, "dynaudnorm"); }
    else if (!std::strcmp(n, "dwidth"))      { if (d) m_snap.videoWidth = (int)*(int64_t*)d; }
    else if (!std::strcmp(n, "dheight"))     { if (d) m_snap.videoHeight = (int)*(int64_t*)d; }
    else return;   // sub-text / sub-start ignored (libass renders natively)

    emitSnapshot();
}

void MpvController::handleEndFile(void* ev)
{
    auto* ef = static_cast<mpv_event_end_file*>(ev);
    const int reason = ef ? ef->reason : MPV_END_FILE_REASON_EOF;
    if (reason == MPV_END_FILE_REASON_STOP || reason == MPV_END_FILE_REASON_QUIT
        || reason == MPV_END_FILE_REASON_REDIRECT)
        return;   // ignored (§0.2)
    if (reason == MPV_END_FILE_REASON_EOF) {
        m_snap.status = PlayerSnapshot::Ended;
    } else {
        m_snap.status = PlayerSnapshot::Error;
        m_snap.errorCode = PlayerSnapshot::Decode;
        m_snap.errorMessage = QStringLiteral("mpv ended playback: %1")
                                  .arg(QString::fromUtf8(endReasonStr(reason)));
    }
    emitSnapshot();
}
