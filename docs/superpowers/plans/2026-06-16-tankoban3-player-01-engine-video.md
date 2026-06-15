# Tankoban 3 Player — Plan 1: Engine + Video On Screen Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** A standalone-testable Tankoban 3 video window that plays a hardcoded stream URL via libmpv (rendered into a `QOpenGLWidget`), with full keyboard transport control and click/double-click — and **no visible chrome yet**.

**Architecture:** One in-process C++ engine class (`MpvController`) collapses Harbor's split Rust (`mpv.rs`) + JS (`mpv.ts` + `bridge.ts`) layer — it owns a single `mpv_handle*`, sets init options, sends commands, observes properties, and emits a `PlayerSnapshot`. A sibling `MpvGlWidget : QOpenGLWidget` owns the libmpv **render context** (`MPV_RENDER_API_TYPE_OPENGL`) and paints frames — this mirrors Harbor's own Mac/Linux render path (NOT its Windows `wid` hack), which Qt makes available on Windows. `PlayerView` hosts the GL widget fullscreen, `StageInputLayer` handles click/double-click, `HotkeyDispatcher` handles all keys.

**Tech Stack:** C++20, Qt6 (Core/Gui/Widgets/**OpenGL/OpenGLWidgets**), libmpv (client + render API), CMake + Ninja + MSVC 2022.

**Spec source of truth:** `docs/harbor-player-spec.md` — read **§0 (decisions/corrections)** first; this plan implements §2 (render), §3 (mpv contract), §4 (controller), §9.1 (hotkeys), §9.3/9.5 (fullscreen/click). When this plan and the spec body conflict, **spec §0 wins**.

**Verification model (read this — it adapts the skill's default TDD):** This is a GPU/video/Qt-widget subsystem with no test target yet. Per the project convention (build + watch-it-play smoke) and the spec, verification gates are: **(a)** build succeeds and the `.exe` mtime advanced, **(b)** a registered CTest self-test passes for pure-logic helpers (no external deps — a tiny assert exe), **(c)** a runtime smoke observation (video paints / key works). Full GoogleTest harness is deferred to a later plan; the pure-logic units here (`TimeFormat`) get an inline self-test so there is still an automated check.

---

## File Structure

| File | Responsibility |
|---|---|
| `third_party/libmpv/include/mpv/*.h` | Vendored libmpv headers (client.h, render.h, render_gl.h, stream_cb.h) — from Harbor's matched set |
| `third_party/libmpv/lib/mpv.lib` | Vendored MSVC import lib (Harbor's) — gitignored |
| `third_party/libmpv/bin/libmpv-2.dll` | Vendored runtime (~117MB, proven HDR/HEVC) — gitignored, copied next to exe |
| `setup_libmpv.bat` | Idempotent dev helper: stage headers+lib+dll into `third_party/libmpv/` |
| `CMakeLists.txt` | MODIFY: add Qt6 OpenGL/OpenGLWidgets, mpv include/link, post-build dll copy, new sources, self-test target |
| `src/engine/PlayerSnapshot.h` | `PlayerSnapshot` + `TrackInfo` + `Chapter` structs (spec §4.3) |
| `src/util/TimeFormat.{h,cpp}` | `fmtTime(sec)` (spec §7.1) — pure logic, self-tested |
| `src/engine/MpvController.{h,cpp}` | Owns `mpv_handle*`: init options (§3.1), commands (§3.2/§4.2), observers (§3.3), event pump, snapshot emit, value coercion (§0.2) |
| `src/engine/MpvGlWidget.{h,cpp}` | `QOpenGLWidget`: render-context create/render/teardown (§2) |
| `src/engine/PlaybackClock.{h,cpp}` | Singleton: 60Hz interpolated position, snapping to real ticks (§0.1 #2, §4.6) |
| `src/player/PlayerView.{h,cpp}` | Hosts MpvGlWidget fullscreen; owns controller; fullscreen toggle (§9.3) |
| `src/player/StageInputLayer.{h,cpp}` | Click=play/pause, dblclick=fullscreen (§9.5) |
| `src/player/HotkeyDispatcher.{h,cpp}` | Full hotkey table + hold-to-speed (§9.1/§9.1a) |
| `src/main.cpp` | MODIFY: `TANKOBAN3_PLAYER_DEMO=<url>` (or `--player-demo <url>`) launches PlayerView standalone; `--self-test` runs pure-logic asserts |

**Pre-rebuild discipline (every build step):** `taskkill //F //IM Tankoban3.exe` first (a running exe makes "BUILD OK" lie — the link silently keeps the old binary). After CMakeLists changes, force a reconfigure (T3's `build.bat` only configures when `CMakeCache.txt` is absent): delete `out/CMakeCache.txt` + `out/build.ninja` (or `rm -rf out`). Verify `out/Tankoban3.exe` mtime advanced after every build.

---

## Task 1: Vendor libmpv + CMake wiring + linkage probe

**Files:**
- Create: `setup_libmpv.bat`
- Create (staged on disk, gitignored): `third_party/libmpv/{include/mpv/*.h, lib/mpv.lib, bin/libmpv-2.dll}`
- Modify: `CMakeLists.txt`, `.gitignore`
- Create (temporary probe, deleted in Step 6): `src/engine/mpv_probe.cpp`

- [ ] **Step 1: Write `setup_libmpv.bat`** — stage the matched libmpv set idempotently.

```bat
@echo off
setlocal
:: Stage libmpv (headers + MSVC import lib + runtime dll) into third_party\libmpv\.
:: Source = Harbor's matched set (import lib + headers) + a proven libmpv-2.dll.
set "DST=%~dp0third_party\libmpv"
set "HARBOR=%LOCALAPPDATA%\Temp\harbor-ref\src-tauri\libmpv"
set "DLL_SRC=%USERPROFILE%\Desktop\Tankoban Electron\resources\mpv\windows\libmpv-2.dll"

if not exist "%DST%\include\mpv" mkdir "%DST%\include\mpv"
if not exist "%DST%\lib" mkdir "%DST%\lib"
if not exist "%DST%\bin" mkdir "%DST%\bin"

copy /Y "%HARBOR%\include\mpv\*.h" "%DST%\include\mpv\" >nul
copy /Y "%HARBOR%\mpv.lib" "%DST%\lib\mpv.lib" >nul
if not exist "%DST%\bin\libmpv-2.dll" copy /Y "%DLL_SRC%" "%DST%\bin\libmpv-2.dll" >nul

if not exist "%DST%\lib\mpv.lib" ( echo SETUP_LIBMPV: mpv.lib missing & exit /b 2 )
if not exist "%DST%\bin\libmpv-2.dll" ( echo SETUP_LIBMPV: libmpv-2.dll missing & exit /b 3 )
if not exist "%DST%\include\mpv\render_gl.h" ( echo SETUP_LIBMPV: headers missing & exit /b 4 )
echo SETUP_LIBMPV OK
exit /b 0
```

- [ ] **Step 2: Run it + add to `.gitignore`.**

Run: `cmd //c setup_libmpv.bat`
Expected: `SETUP_LIBMPV OK`, and `ls third_party/libmpv/bin/libmpv-2.dll` shows ~117MB.

Append to `.gitignore`:
```
# Vendored libmpv binaries (staged by setup_libmpv.bat; headers are committed)
/third_party/libmpv/lib/
/third_party/libmpv/bin/
```

- [ ] **Step 3: Modify `CMakeLists.txt`** — OpenGL modules, libmpv import target, dll copy, sources placeholder, self-test target.

Add `OpenGL OpenGLWidgets` to the existing `find_package(Qt6 ...)` components line. Then add, after the existing `qt_add_executable(...)` block:

```cmake
# ── libmpv (vendored, in-process; client + render API) ───────────────────────
set(LIBMPV_DIR ${CMAKE_SOURCE_DIR}/third_party/libmpv)
add_library(libmpv SHARED IMPORTED)
set_target_properties(libmpv PROPERTIES
    IMPORTED_IMPLIB   ${LIBMPV_DIR}/lib/mpv.lib
    IMPORTED_LOCATION ${LIBMPV_DIR}/bin/libmpv-2.dll
    INTERFACE_INCLUDE_DIRECTORIES ${LIBMPV_DIR}/include)

target_link_libraries(Tankoban3 PRIVATE
    Qt6::OpenGL Qt6::OpenGLWidgets
    libmpv)

# Copy the runtime dll next to the exe after build.
add_custom_command(TARGET Tankoban3 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${LIBMPV_DIR}/bin/libmpv-2.dll $<TARGET_FILE_DIR:Tankoban3>)
```

Add `Qt6::OpenGL Qt6::OpenGLWidgets` to the existing `target_link_libraries` if a single block is preferred — but the above appended block is sufficient (CMake merges).

- [ ] **Step 4: Write the linkage probe** `src/engine/mpv_probe.cpp` (temporary; proves the import lib links + the dll loads + the API is reachable).

```cpp
#include <mpv/client.h>
#include <cstdio>

int mpv_probe_main() {
    mpv_handle* h = mpv_create();
    if (!h) { std::printf("PROBE: mpv_create failed\n"); return 1; }
    std::printf("PROBE: libmpv client api = 0x%lx\n", mpv_client_api_version());
    int rc = mpv_initialize(h);
    std::printf("PROBE: mpv_initialize rc=%d (%s)\n", rc, mpv_error_string(rc));
    mpv_terminate_destroy(h);
    std::printf("PROBE: OK\n");
    return rc < 0 ? 1 : 0;
}
```

Temporarily register it in CMake's `qt_add_executable` source list and call `mpv_probe_main()` from `main.cpp` first line of `main()` (guarded by `if (qEnvironmentVariableIsSet("TANKOBAN3_MPV_PROBE")) return mpv_probe_main();`).

- [ ] **Step 5: Build + run the probe.**

Run:
```
taskkill //F //IM Tankoban3.exe 2>/dev/null; rm -rf out
cmd //c build.bat
```
Expected: `BUILD OK`. Verify `out/Tankoban3.exe` exists and is freshly built.

Run: `TANKOBAN3_MPV_PROBE=1 out/Tankoban3.exe`
Expected stdout: `PROBE: libmpv client api = 0x2....`, `PROBE: mpv_initialize rc=0 (success)`, `PROBE: OK`.
**This is the keystone de-risk** — it proves mpv.lib links against this toolchain and libmpv-2.dll loads + initializes. If `mpv_create`/`mpv_initialize` fail, STOP and resolve (lib/dll ABI mismatch → use Harbor's matching dll via its `setup-libmpv.ps1`) before proceeding.

- [ ] **Step 6: Remove the probe** (`rm src/engine/mpv_probe.cpp`, revert its CMake line + the `main.cpp` guard), rebuild to confirm still `BUILD OK`.

- [ ] **Step 7: Commit.**

```bash
git add setup_libmpv.bat CMakeLists.txt .gitignore third_party/libmpv/include
git commit -m "build(player): vendor libmpv + wire Qt6 OpenGL render-API link (probe verified)"
```

---

## Task 2: `PlayerSnapshot` structs + `TimeFormat` (pure logic + self-test)

**Files:**
- Create: `src/engine/PlayerSnapshot.h`, `src/util/TimeFormat.h`, `src/util/TimeFormat.cpp`
- Create: `src/selftest_main.cpp` (CTest-registered pure-logic check, no external deps)
- Modify: `CMakeLists.txt` (add sources + a `Tankoban3SelfTest` executable + `add_test`)

- [ ] **Step 1: Write `src/engine/PlayerSnapshot.h`** — port spec §4.3 exactly.

```cpp
#pragma once
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
    bool   buffering = false;                                  // dead
    double volume = 1.0;        // 0..6 (= mpv volume/100)
    bool   muted = false;
    double rate = 1.0;          // optimistic; speed not observed
    QVector<TrackInfo> audioTracks, subtitleTracks;
    QVector<Chapter>   chapters;
    double subDelaySec = 0, audioDelaySec = 0;
    bool   audioNormalize = false;
    int    videoWidth = 0, videoHeight = 0;
    QString errorMessage;
    enum ErrorCode { None, Decode, Codec, Network, Source, Unknown } errorCode = None;
    bool   noAudio = false;     // dead (never written by mpv path, §0.2)
};
```

- [ ] **Step 2: Write `src/util/TimeFormat.h` / `.cpp`** — spec §7.1 verbatim.

```cpp
// TimeFormat.h
#pragma once
#include <QString>
QString fmtTime(double sec);   // "M:SS" under 1h, "H:MM:SS" at/over; NaN/neg -> "0:00"
```
```cpp
// TimeFormat.cpp
#include "util/TimeFormat.h"
#include <cmath>
QString fmtTime(double sec) {
    if (!std::isfinite(sec) || sec < 0) return QStringLiteral("0:00");
    long long total = (long long)std::floor(sec);
    long long h = total / 3600, m = (total % 3600) / 60, s = total % 60;
    if (h > 0)
        return QStringLiteral("%1:%2:%3").arg(h).arg(m, 2, 10, QLatin1Char('0')).arg(s, 2, 10, QLatin1Char('0'));
    return QStringLiteral("%1:%2").arg(m).arg(s, 2, 10, QLatin1Char('0'));
}
```

- [ ] **Step 3: Write the self-test** `src/selftest_main.cpp`.

```cpp
#include "util/TimeFormat.h"
#include <cstdio>
#include <cassert>
static int failures = 0;
static void check(const QString& got, const char* want) {
    if (got != QString::fromUtf8(want)) { std::printf("FAIL: got '%s' want '%s'\n", got.toUtf8().constData(), want); ++failures; }
}
int main() {
    check(fmtTime(0), "0:00");
    check(fmtTime(65), "1:05");
    check(fmtTime(425), "7:05");
    check(fmtTime(3729), "1:02:09");
    check(fmtTime(-5), "0:00");
    check(fmtTime(std::nan("")), "0:00");
    std::printf(failures ? "SELFTEST FAILED (%d)\n" : "SELFTEST OK\n", failures);
    return failures ? 1 : 0;
}
```

- [ ] **Step 4: Wire CMake** — add `src/engine/PlayerSnapshot.h`, `src/util/TimeFormat.{h,cpp}` to `qt_add_executable`. Add a self-test target after it:

```cmake
add_executable(Tankoban3SelfTest src/selftest_main.cpp src/util/TimeFormat.cpp)
target_link_libraries(Tankoban3SelfTest PRIVATE Qt6::Core)
target_include_directories(Tankoban3SelfTest PRIVATE ${CMAKE_SOURCE_DIR}/src)
enable_testing()
add_test(NAME selftest COMMAND Tankoban3SelfTest)
```

- [ ] **Step 5: Build + run self-test.**

Run:
```
taskkill //F //IM Tankoban3.exe 2>/dev/null; rm -f out/CMakeCache.txt out/build.ninja
cmd //c build.bat
out/Tankoban3SelfTest.exe
```
Expected: `BUILD OK` then `SELFTEST OK`. (Or `ctest --test-dir out --output-on-failure`.)

- [ ] **Step 6: Commit.**

```bash
git add src/engine/PlayerSnapshot.h src/util/TimeFormat.h src/util/TimeFormat.cpp src/selftest_main.cpp CMakeLists.txt
git commit -m "feat(player): PlayerSnapshot/TrackInfo structs + fmtTime (self-tested)"
```

---

## Task 3: `MpvController` — engine (init, commands, observers, event pump)

**Files:**
- Create: `src/engine/MpvController.h`, `src/engine/MpvController.cpp`
- Modify: `CMakeLists.txt` (add sources; `MpvController.h` needs MOC — it's a QObject)

- [ ] **Step 1: Write `MpvController.h`** — the collapsed engine API (spec §4.5).

```cpp
#pragma once
#include "engine/PlayerSnapshot.h"
#include <QObject>
#include <QString>
struct mpv_handle;
class MpvGlWidget;

class MpvController : public QObject {
    Q_OBJECT
public:
    explicit MpvController(QObject* parent = nullptr);
    ~MpvController() override;

    mpv_handle* handle() const { return m_mpv; }   // MpvGlWidget grabs this for render ctx
    void initialize();                              // mpv_create + options + mpv_initialize
    void load(const QString& url, double startSec = 0.0);

    // transport (spec §4.2 — values coerced per §0.2)
    void play();  void pause();
    void seek(double sec);                           // ["seek", sec, "absolute", "exact"]
    void setVolume(double v0to6);  void setMuted(bool m);
    void setRate(double r);
    void setAudioTrack(const QString& id);
    void setSubtitleTrack(const QString& id);        // empty -> "no"
    void setSubVisible(bool on);
    void setSubDelay(double s);  void setAudioDelay(double s);
    void setPanscan(double v);  void setVideoZoom(double log2);  void setAspectOverride(const QString& ratio);

    PlayerSnapshot snapshot() const { return m_snap; }

signals:
    void snapshotChanged(const PlayerSnapshot& snap);
    void wakeup();   // queued: mpv thread -> GUI thread

private slots:
    void onWakeup(); // drains mpv_wait_event(0) on GUI thread

private:
    void setOpt(const char* name, const char* value);
    void setPropFlag(const char* name, bool v);
    void setPropDouble(const char* name, double v);
    void setPropString(const char* name, const QString& v);   // §0.2 coercion lives here
    void command(std::initializer_list<QByteArray> args);
    void observeProperties();
    void handlePropertyChange(void* prop);   // mpv_event_property*
    void handleEndFile(void* ev);

    mpv_handle* m_mpv = nullptr;
    bool m_initialized = false;
    PlayerSnapshot m_snap;
};
```

- [ ] **Step 2: Write `MpvController.cpp`** — implement against spec §3.1/§3.2/§3.3, §0.2.

Key requirements (cite the spec tables — they are exact and in-repo):
- **ctor:** create the queued connection `connect(this,&MpvController::wakeup,this,&MpvController::onWakeup,Qt::QueuedConnection);`
- **initialize():** `mpv_create()`; set PRE-INIT options from **spec §3.1 PRE-INIT table** (title=`Tankoban`, `terminal=no`, `input-default-bindings=no`, `osc=no`, `osd-level=0`, `volume-max=600`, etc.) + render path opts `vo=libmpv`, `force-window=no`, `hwdec=auto`; `mpv_initialize()`; POST-INIT cache/demuxer/network + subtitle-base from **§3.1 POST-INIT tables**; `observeProperties()`; `mpv_set_wakeup_callback(m_mpv, &wakeupTrampoline, this)`. **Do NOT set** `sub-visibility=no` (§0.1 #4). On Windows skip the `setlocale` call (§3.1 note, Windows-exempt).
- **wakeup trampoline** (static, runs on mpv thread): `static_cast<MpvController*>(ctx)` → `emit wakeup()` only (queued → GUI thread). Nothing else on that thread.
- **onWakeup():** loop `mpv_wait_event(m_mpv, 0)`; switch on `event_id`: `MPV_EVENT_PROPERTY_CHANGE` → `handlePropertyChange`; `MPV_EVENT_END_FILE` → `handleEndFile`; `MPV_EVENT_FILE_LOADED` → status=Playing, clear error; ignore `playback-restart`/`seek`/`log` (§0.2); break on `MPV_EVENT_NONE`.
- **observeProperties():** register all **15 properties from spec §3.3** with the listed `MPV_FORMAT_*` and reply-ids.
- **handlePropertyChange():** map each property → snapshot field per §3.3 (time-pos→positionSec, duration→durationSec, pause→status, eof-reached→Ended, track-list→parse §8.8, volume→/100, mute, chapter-list, sub-delay, audio-delay, af→audioNormalize, dwidth→videoWidth, dheight→videoHeight). After any change, `m_snap` updated → `emit snapshotChanged(m_snap)`.
- **handleEndFile():** decode reason **lowercase** (§0.2): ignore stop/quit/redirect; `eof`→status=Ended; else status=Error, errorCode=Decode, errorMessage=`mpv ended playback: <reason>`.
- **commands/verbs:** `load` → `["loadfile", url, "replace"]` (fresh) or with `start=<sec>` 4th arg if resume (§3.2); `seek`→`["seek", sec, "absolute", "exact"]`; transport verbs → `set_property` per **§4.2 table**.
- **§0.2 value coercion (LOAD-BEARING):** `setPropFlag` uses `MPV_FORMAT_FLAG`; for the generic string path coerce bool→`yes`/`no`, number→decimal string, null/empty→`""`. Use typed `mpv_set_property` (FLAG/DOUBLE) where the property is bool/double; use `mpv_set_property_string` only for string-valued props.
- `setVolume`: `mpv volume = round(v*100)` (0..600); snapshot reads volume/100 (§3.4).
- `setSubtitleTrack("")` → `sid = "no"`.
- **dtor:** `if (m_mpv) mpv_terminate_destroy(m_mpv);` (render ctx is freed by MpvGlWidget first — see Task 4 teardown order).

- [ ] **Step 3: Wire CMake** (add `src/engine/MpvController.h` + `.cpp`; AUTOMOC already on). Build.

Run: `taskkill //F //IM Tankoban3.exe 2>/dev/null; cmd //c build.bat`
Expected: `BUILD OK`. (No runtime smoke yet — no surface. Compile-clean is the gate; MpvController is exercised in Task 4.)

- [ ] **Step 4: Commit.**

```bash
git add src/engine/MpvController.h src/engine/MpvController.cpp CMakeLists.txt
git commit -m "feat(player): MpvController engine — init/commands/observers/event-pump (Harbor mpv.rs+mpv.ts 1:1)"
```

---

## Task 4: `MpvGlWidget` — render context + first frame on screen

**Files:**
- Create: `src/engine/MpvGlWidget.h`, `src/engine/MpvGlWidget.cpp`
- Modify: `CMakeLists.txt`; temporarily extend `main.cpp` for an isolated render smoke

- [ ] **Step 1: Write `MpvGlWidget.h`.**

```cpp
#pragma once
#include <QOpenGLWidget>
#include <atomic>
struct mpv_render_context;
class MpvController;

class MpvGlWidget : public QOpenGLWidget {
    Q_OBJECT
public:
    explicit MpvGlWidget(MpvController* controller, QWidget* parent = nullptr);
    ~MpvGlWidget() override;
protected:
    void initializeGL() override;
    void paintGL() override;
private:
    static void* getProcAddress(void* ctx, const char* name);  // -> Qt's getProcAddress
    static void  onMpvUpdate(void* ctx);                       // mpv thread -> schedule repaint
    MpvController* m_controller;
    mpv_render_context* m_mpvGl = nullptr;
    std::atomic<bool> m_redrawPending{false};
};
```

- [ ] **Step 2: Write `MpvGlWidget.cpp`** — spec §2.1/§2.2 exactly.

```cpp
#include "engine/MpvGlWidget.h"
#include "engine/MpvController.h"
#include <QOpenGLContext>
#include <mpv/client.h>
#include <mpv/render_gl.h>

MpvGlWidget::MpvGlWidget(MpvController* c, QWidget* parent)
    : QOpenGLWidget(parent), m_controller(c) {}

void* MpvGlWidget::getProcAddress(void* ctx, const char* name) {
    auto* gl = QOpenGLContext::currentContext();
    if (!gl) return nullptr;
    return reinterpret_cast<void*>(gl->getProcAddress(name));   // Qt resolves across GL/GLES/ANGLE
}

void MpvGlWidget::onMpvUpdate(void* ctx) {
    auto* self = static_cast<MpvGlWidget*>(ctx);
    bool expected = false;
    if (self->m_redrawPending.compare_exchange_strong(expected, true))   // coalesce (§2.2)
        QMetaObject::invokeMethod(self, "update", Qt::QueuedConnection);
}

void MpvGlWidget::initializeGL() {
    mpv_opengl_init_params gl_init{ &MpvGlWidget::getProcAddress, this };
    int advanced = 1;
    mpv_render_param params[] = {
        { MPV_RENDER_PARAM_API_TYPE, const_cast<char*>(MPV_RENDER_API_TYPE_OPENGL) },
        { MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init },
        { MPV_RENDER_PARAM_ADVANCED_CONTROL, &advanced },
        { MPV_RENDER_PARAM_INVALID, nullptr }
    };
    if (mpv_render_context_create(&m_mpvGl, m_controller->handle(), params) < 0) {
        qFatal("mpv_render_context_create failed");
    }
    mpv_render_context_set_update_callback(m_mpvGl, &MpvGlWidget::onMpvUpdate, this);
}

void MpvGlWidget::paintGL() {
    m_redrawPending.store(false);
    if (!m_mpvGl) return;
    const qreal dpr = devicePixelRatioF();
    mpv_opengl_fbo fbo{ static_cast<int>(defaultFramebufferObject()),   // NOT 0 (§2.2)
                        int(width() * dpr), int(height() * dpr), 0 };
    int flip_y = 1;
    mpv_render_param p[] = {
        { MPV_RENDER_PARAM_OPENGL_FBO, &fbo },
        { MPV_RENDER_PARAM_FLIP_Y, &flip_y },
        { MPV_RENDER_PARAM_INVALID, nullptr }
    };
    mpv_render_context_render(m_mpvGl, p);
}

MpvGlWidget::~MpvGlWidget() {
    makeCurrent();
    if (m_mpvGl) { mpv_render_context_free(m_mpvGl); m_mpvGl = nullptr; }   // free on GL thread, ctx valid
    doneCurrent();
}
```

- [ ] **Step 3: Isolated render smoke in `main.cpp`** (temporary) — construct `MpvController`, `initialize()`, build an `MpvGlWidget`, `show()` it ~960×540, `load(<hardcoded URL>)`. Use a stable public test stream:
`https://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4`

```cpp
// guarded temporary smoke (removed in Step 5)
if (qEnvironmentVariableIsSet("TANKOBAN3_RENDER_SMOKE")) {
    auto* ctrl = new MpvController();
    ctrl->initialize();
    auto* w = new MpvGlWidget(ctrl);
    w->resize(960, 540); w->show();
    ctrl->load(QStringLiteral("https://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4"));
    return app.exec();
}
```

- [ ] **Step 4: Build + run the render smoke.**

Run:
```
taskkill //F //IM Tankoban3.exe 2>/dev/null; cmd //c build.bat
TANKOBAN3_RENDER_SMOKE=1 out/Tankoban3.exe
```
Expected: a 960×540 window shows **Big Buck Bunny playing with audio** (no controls). This is the milestone-defining smoke — video frames composited into the QOpenGLWidget via the render API on the Intel UHD GPU. **Eyes-on-screen gate** (per memory: `firstFrameSeen`/signal ≠ pixels painted — a human or screenshot must confirm actual video). If black: check `defaultFramebufferObject()` is used (not 0), `FLIP_Y`, and that `vo=libmpv` was set pre-init.

- [ ] **Step 5: Leave the smoke guard in place for now** (Task 6 replaces it with PlayerView). Commit.

```bash
git add src/engine/MpvGlWidget.h src/engine/MpvGlWidget.cpp src/main.cpp CMakeLists.txt
git commit -m "feat(player): MpvGlWidget render-API into QOpenGLWidget — video on screen (smoke verified)"
```

---

## Task 5: `PlaybackClock` (60Hz interpolation, snap to ticks)

**Files:**
- Create: `src/engine/PlaybackClock.h`, `src/engine/PlaybackClock.cpp`; Modify `CMakeLists.txt`

- [ ] **Step 1: Write `PlaybackClock`** — spec §4.6 + §0.1 #2.

```cpp
// PlaybackClock.h
#pragma once
#include <QObject>
#include <QElapsedTimer>
class QTimer;
class PlaybackClock : public QObject {
    Q_OBJECT
public:
    static PlaybackClock& instance();
    double position() const { return m_displayPos; }
    double buffered() const { return m_buffered; }
    // fed from snapshotChanged: real time-pos + whether playing (drives interpolation)
    void onSnapshot(double positionSec, double bufferedSec, bool playing, double rate);
    void reset();
signals:
    void changed();
private:
    explicit PlaybackClock(QObject* parent = nullptr);
    void tick();
    double m_anchorPos = 0, m_displayPos = 0, m_buffered = 0, m_rate = 1.0;
    bool m_playing = false;
    QElapsedTimer m_since;
    QTimer* m_timer;
};
```
Implementation: a ~60Hz `QTimer` (16ms). On `onSnapshot`, set `m_anchorPos = positionSec`, restart `m_since`, store playing/rate/buffered, emit `changed()`. On `tick()`, if playing compute `m_displayPos = m_anchorPos + m_since.elapsed()/1000.0 * m_rate` and emit `changed()`; if paused, `m_displayPos = m_anchorPos`. `reset()` zeroes everything (call on teardown / load).

- [ ] **Step 2: Build.** `taskkill //F //IM Tankoban3.exe 2>/dev/null; cmd //c build.bat` → `BUILD OK`.

- [ ] **Step 3: Commit.**
```bash
git add src/engine/PlaybackClock.h src/engine/PlaybackClock.cpp CMakeLists.txt
git commit -m "feat(player): PlaybackClock — 60Hz interpolation snapping to time-pos ticks"
```

---

## Task 6: `PlayerView` + `StageInputLayer` + fullscreen

**Files:**
- Create: `src/player/PlayerView.{h,cpp}`, `src/player/StageInputLayer.{h,cpp}`; Modify `CMakeLists.txt`, `main.cpp`

- [ ] **Step 1: Write `StageInputLayer`** — spec §9.5 (Qt mapping).

A transparent `QWidget` filling the stage. `mousePressEvent`: record press; `mouseReleaseEvent`: if not a drag, start a single-shot `QTimer(QApplication::doubleClickInterval())` that emits `clicked()` (play/pause) unless a double-click arrived. `mouseDoubleClickEvent`: cancel the pending single-click timer, emit `doubleClicked()` (fullscreen). Drop Harbor's window-drag-on-video (reference-only). Signals: `clicked()`, `doubleClicked()`.

- [ ] **Step 2: Write `PlayerView`** — hosts `MpvGlWidget` + `StageInputLayer`, owns the `MpvController`, wires snapshot→PlaybackClock, exposes `play(url)`, drives fullscreen (§9.3).

```cpp
// PlayerView.h (essentials)
class PlayerView : public QWidget {
    Q_OBJECT
public:
    explicit PlayerView(QWidget* parent = nullptr);
    void play(const QString& url, double startSec = 0.0);
    MpvController* controller() const { return m_controller; }
    void toggleFullscreen();                       // window()->showFullScreen()/showNormal()
    PlayerSnapshot snap() const { return m_controller->snapshot(); }
public slots:
    void playPauseToggle();
private:
    MpvController* m_controller;
    MpvGlWidget*   m_video;       // fills the view
    StageInputLayer* m_input;     // on top, transparent
};
```
- Layout: `m_video` and `m_input` both fill the PlayerView (stacked; input on top, `Qt::WA_TransparentForMouseEvents=false` on input so it receives clicks but is visually empty).
- Wire: `connect(m_controller,&MpvController::snapshotChanged, this, [](const PlayerSnapshot& s){ PlaybackClock::instance().onSnapshot(s.positionSec,s.bufferedSec,s.status==PlayerSnapshot::Playing,s.rate); });`
- `connect(m_input,&StageInputLayer::clicked,this,&PlayerView::playPauseToggle);`
- `connect(m_input,&StageInputLayer::doubleClicked,this,&PlayerView::toggleFullscreen);`
- `playPauseToggle()`: read `snap().status`; if Playing → `m_controller->pause()` else `m_controller->play()`.
- `toggleFullscreen()`: `window()->isFullScreen() ? window()->showNormal() : window()->showFullScreen();` (do NOT set mpv `fullscreen`, §0.1 #9).

- [ ] **Step 3: Replace the Task-4 render smoke in `main.cpp`** with the real demo entry (kept for Task 8): for now, if `TANKOBAN3_PLAYER_DEMO` set, construct a `PlayerView`, `showFullScreen()` (or 1280×720), `play(<hardcoded URL>)`.

- [ ] **Step 4: Build + smoke.** Run `TANKOBAN3_PLAYER_DEMO=1 out/Tankoban3.exe`.
Expected: video plays; **single-click pauses/resumes**, **double-click toggles fullscreen**. (Eyes-on gate.)

- [ ] **Step 5: Commit.**
```bash
git add src/player/PlayerView.h src/player/PlayerView.cpp src/player/StageInputLayer.h src/player/StageInputLayer.cpp src/main.cpp CMakeLists.txt
git commit -m "feat(player): PlayerView + StageInputLayer — click=play/pause, dblclick=fullscreen"
```

---

## Task 7: `HotkeyDispatcher` — full hotkey table + hold-to-speed

**Files:**
- Create: `src/player/HotkeyDispatcher.{h,cpp}`; Modify `CMakeLists.txt`, `PlayerView` (install it)

- [ ] **Step 1: Write `HotkeyDispatcher`** — implement the **complete spec §9.1 table** + §9.1a, with the §0.2 corrections (mute `m` doesn't persist; `[`/`]` reach 3× and round to 2 dp; `e.repeat`/auto-repeat arms hold-timer once).

An app-level event filter (installed on the PlayerView/window). On `QKeyEvent` (KeyPress): skip if `QApplication::focusWidget()` is a `QLineEdit`/`QTextEdit`/`QComboBox` (typing-target gate). Dispatch per the table — load-bearing magnitudes (cite §9.1):
- Space → tap=play/pause, hold 350ms → `setRate(max(2, baseRate))` (auto-repeat: consume, arm timer once); Escape → close; `f` → fullscreen; ArrowL/R → seek ∓10s; `,`/`.` → seek ∓30s; Home→seek 0; End→`if dur>0 seek(dur-0.5)`; `0`→seek 0; `1`–`9`→`seek(dur*d/10)`; ArrowUp/Down → vol ±0.05 (Shift ±0.5) clamp [0,6], persist (but **not** on `m`); `m` → mute toggle (no persist); `s`/`c` → cycle subtitles (OFF→sub0→…→OFF); `z`/`x` → sub-delay ∓0.1 (Shift 0.05), round2, persist; `[`/`]` → speed ∓0.25 clamp **[0.25,3]** round2, persist; `v` → cycle fill (Task in Plan 3 — wire a no-op signal now); `=`/`-` → zoom ±0.1 (Plan 3 — signal now).
- Seek-step reads `PlaybackClock::instance().position()` (not the raw snapshot), per §9.1 note.

Expose signals the PlayerView connects to controller methods, OR hold a `MpvController*` + `PlayerView*` directly. Keep volume/sub-delay/speed persistence as TODO hooks calling a `PlayerPrefs` stub (real persistence = Plan 3 Task; for now apply to the live controller, skip disk writes).

- [ ] **Step 2: Install it** in `PlayerView` ctor (`qApp->installEventFilter(m_hotkeys)` or on the window).

- [ ] **Step 3: Build + smoke.** Run the demo; verify: **Space** pauses/plays, **←/→** seek ±10s, **↑/↓** change volume, **m** mutes, **f** toggles fullscreen, **hold Space** speeds to 2×, releasing restores. (Eyes/ears-on gate.)

- [ ] **Step 4: Commit.**
```bash
git add src/player/HotkeyDispatcher.h src/player/HotkeyDispatcher.cpp src/player/PlayerView.cpp CMakeLists.txt
git commit -m "feat(player): HotkeyDispatcher — full Harbor hotkey table + hold-to-speed (§9.1)"
```

---

## Task 8: Standalone demo entry + milestone smoke

**Files:** Modify `src/main.cpp` (finalize the demo entry), `README.md` (document the flag)

- [ ] **Step 1: Finalize `main.cpp` demo entry.** Accept the URL from `TANKOBAN3_PLAYER_DEMO` (a value other than `1`) or `--player-demo <url>`; fall back to the BigBuckBunny URL. Launch a fullscreen `PlayerView`. Keep `--self-test` (Task 2) routing intact. Default (no flag) still launches the normal `MainWindow` (untouched).

- [ ] **Step 2: Document in `README.md`** under Status:
```
Player (standalone test): set TANKOBAN3_PLAYER_DEMO=<stream-url> (or --player-demo <url>) to launch
the libmpv player fullscreen against a hardcoded/given URL. Controls: click=play/pause, dblclick=fullscreen,
Space/←/→/↑/↓/m/f/[/]/s + digit-seek per spec §9.1.
```

- [ ] **Step 3: Final milestone smoke (acceptance).** Run `out/Tankoban3.exe --player-demo "https://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4"`.
Acceptance checklist (eyes-on): video plays smoothly fullscreen on the Intel UHD GPU; click pauses; double-click toggles fullscreen; Space/seek/volume/mute/hold-speed all work; closing the window terminates cleanly (no hang, no crash on teardown — verifies §2.2 free order).

- [ ] **Step 4: Commit.**
```bash
git add src/main.cpp README.md
git commit -m "feat(player): standalone PlayerView demo entry (TANKOBAN3_PLAYER_DEMO) — Plan 1 milestone"
```

---

## Self-Review (spec coverage)

- §2 render → Task 4 ✓ · §3.1 options → Task 3 ✓ · §3.2 commands / §3.3 observers / event pump → Task 3 ✓ · §3.4 volume curve write → Task 3 (`setVolume`) ✓ · §4.3 structs → Task 2 ✓ · §4.5 controller API → Task 3 ✓ · §4.6 clock → Task 5 ✓ · §7.1 fmtTime → Task 2 ✓ · §9.1 hotkeys + §9.1a hold-speed → Task 7 ✓ · §9.3 fullscreen → Task 6 ✓ · §9.5 click/dblclick → Task 6 ✓ · §0.2 coercion / end-file / mute-no-persist / speed-3× → Task 3 + Task 7 ✓.
- **Deferred to Plan 2/3 (NOT in Plan 1, by design):** all visible chrome (seek bar, control bar, volume widget, time labels, speed/audio/subtitle menus), chrome auto-hide, sub styling/`applySubStyle`, track autoload, video-fill apply (hotkey signals stubbed), stage-overlay pills, prefs persistence, loader. These need a working video surface (this plan) first.
- **Not pure-unit-tested (build+smoke gated, per verification model):** render context, event pump, widgets. **Self-tested:** `fmtTime` (Task 2). `VolumeCurve`/`langScore` self-tests land with their code in Plan 2/3.

---

## Execution Handoff

Plan 1 saved. Plans 2 (transport chrome) and 3 (tracks + behaviors) will be written in the same structure. Execution options for Plan 1:
1. **Subagent-Driven (recommended)** — one fresh subagent per task, review between tasks.
2. **Inline Execution** — execute here with checkpoints after each task's smoke.
