#pragma once
// MpvController — the in-process libmpv engine. Collapses Harbor's split
// Rust (mpv.rs, owns mpv_handle) + JS (mpv.ts commands, bridge.ts events) into
// ONE C++ object: init options (spec §3.1), commands (§3.2/§4.2), observed
// properties (§3.3), event pump, snapshot emit. No IPC, no `wid`.
//
// Threading: libmpv's wakeup callback fires on an arbitrary internal thread;
// it only emits the queued `wakeup()` signal, which lands on the GUI thread in
// onWakeup() where mpv events are drained and widgets/snapshots are touched.
#include "engine/PlayerSnapshot.h"
#include <QObject>
#include <QString>
#include <initializer_list>

struct mpv_handle;

class MpvController : public QObject {
    Q_OBJECT
public:
    explicit MpvController(QObject* parent = nullptr);
    ~MpvController() override;

    mpv_handle* handle() const { return m_mpv; }   // MpvGlWidget attaches its render ctx to this
    bool isInitialized() const { return m_initialized; }
    void initialize();
    void load(const QString& url, double startSec = 0.0);

    // transport (spec §4.2 — values coerced per §0.2)
    void play();  void pause();
    void seek(double sec);                          // ["seek", sec, "absolute", "exact"]
    void setVolume(double v0to6);  void setMuted(bool m);
    void setRate(double r);
    void setAudioTrack(const QString& id);
    void setSubtitleTrack(const QString& id);       // empty -> "no"
    void setSubVisible(bool on);
    void setSubDelay(double s);  void setAudioDelay(double s);
    void setPanscan(double v);  void setVideoZoom(double log2);  void setAspectOverride(const QString& ratio);

    PlayerSnapshot snapshot() const { return m_snap; }

signals:
    void snapshotChanged(const PlayerSnapshot& snap);
    void wakeup();   // emitted from the mpv thread; queued -> onWakeup() on GUI thread

private slots:
    void onWakeup();

private:
    static void wakeupTrampoline(void* ctx);
    void setOpt(const char* name, const char* value);     // pre-init mpv_set_option_string
    void setFlag(const char* name, bool v);               // MPV_FORMAT_FLAG
    void setDouble(const char* name, double v);           // MPV_FORMAT_DOUBLE
    void setStr(const char* name, const QString& v);      // mpv_set_property_string (§0.2 string path)
    void command(std::initializer_list<QByteArray> args); // NULL-terminated argv
    void observeProperties();
    void handlePropertyChange(void* prop);                // mpv_event_property*
    void handleEndFile(void* ev);                         // mpv_event_end_file*
    void emitSnapshot() { emit snapshotChanged(m_snap); }

    mpv_handle* m_mpv = nullptr;
    bool m_initialized = false;
    bool m_started = false;            // has a file been loaded into this handle yet
    PlayerSnapshot m_snap;
};
