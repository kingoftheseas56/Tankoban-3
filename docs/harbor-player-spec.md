# Harbor Player → Tankoban 3 (Qt6) Recreation Spec — CORE milestone

> **Source of truth:** Harbor (Tauri/Rust + React/TS), clone at `C:/Users/Suprabha/AppData/Local/Temp/harbor-ref/`. This document merges 7 parallel source-reading slices into one build-ready spec for recreating Harbor's video player **1:1** in native C++/Qt6 Widgets ("Tankoban 3"). All `file:line` citations are relative to the harbor-ref root.
>
> **Render decision (do NOT re-spec):** video draws via `QOpenGLWidget` + libmpv render API (`MPV_RENDER_API_TYPE_OPENGL`), mirroring Harbor's own Mac/Linux render path (`mpv_render_mac.rs` / `mpv_render_linux.rs`). Harbor's Windows `wid`/child-HWND window-in-window hack exists only because a webview can't host a GL surface — it is **reference-only** everywhere it appears (geometry-poll timers, `mpv_set_geometry`, `webview_helpers.rs`, `d3d11Flip`, `force-window=immediate`). We own the GL surface directly; none of that is recreated.

---

## 0. Decisions & Corrections — READ FIRST (overrides the body where they conflict)

This block resolves the §12 open questions and folds in the load-bearing fixes the 3 completeness critics found (full audit in Appendix A). **When §0 conflicts with the body below, §0 wins.**

### 0.1 Open-question resolutions (all per "match Harbor 1:1")
1. **Volume widget = HORIZONTAL slider** (120×8 track, 48×48 mute, icons `Volume2`/`VolumeX` size 24 sw 1.75, **pct readout only while boosting**). The default `Transport` shell renders the horizontal `VolumeControl` (`player-chrome.ts:186` `volumeStyle:"slider"`). The vertical Stremio pop-up is the EXTRA alt-skin — defer. (Critic-confirmed; §7.2's "vertical primary" is wrong.)
2. **Seek bar = 60Hz interpolated, snapping to each real `time-pos` tick.** Same values Harbor shows, just smooth between ticks (Harbor's literal ~1Hz relay would look choppy). The only place we render smoother-than-literal; flagged for the lead.
3. **Buffered bar = OFF (match Harbor).** Harbor's `bufferedSec`/`buffering` are dead (never populated). Ship no buffered indicator for 1:1; trivially revivable later by observing `demuxer-cache-time` + `paused-for-cache`.
4. **Subtitles = mpv/libass native into the GL surface.** Do NOT set `sub-visibility=no`; do NOT build any HTML/QPainter sub overlay. Keep only `applySubStyle → sub-*` properties (§8.6). Drop `sub-text`/`sub-start` observers.
5. **Seek-step buttons = full Harbor behavior (NOT a plain ±10).** 56×56, `RotateCcw`/`RotateCw` size **32** sw 1.8, with a centered numeric badge (`{seconds}` e.g. "10", `text-[10.5px] font-mono font-bold`), **380ms-hold OR right-click → options popover** `[5,10,15,30,60,90]`, per-direction persistence (`harbor.seek-step.{back|fwd}`). This is the default shell behavior. (Critic P1.)
6. **Gold accent** = Tankoban's own OKLCH gold token (which matches Harbor's `oklch(0.78 0.13 60)` ladder per [[reference_harbor_ultimate_design]]); precompute the sRGB hex at build.
7. **backdrop-blur** = approximate with slightly-more-opaque solid fills for CORE; revisit only if it reads cheap (lead's eyes).
8. **Loader** = minimal state for milestone 1 (spinner + Cancel); defer the cinematic backdrop/torrent-progress polish.
9. **Fullscreen** = drive the Qt top-level window (`showFullScreen`/`showNormal`) only; do NOT set mpv's `fullscreen` property.

### 0.2 Critic corrections that override the body
- **mpv `vo`:** our render path uses `vo=libmpv` (correct). Note Harbor's *non-render* default is `vo=gpu-next,` — informational only; we always use the render-context path.
- **`set_property` value coercion (P0):** the collapsed Qt `MpvController` must coerce values itself before `mpv_set_property_string`: **bool→`yes`/`no`, number→decimal string, null→`""`** (Harbor gets this free via its JSON→Rust hop; we don't). Applies to every control verb in §4.2, not just commands.
- **`subsOff` precedence (HIGH bug fix):** NOT `prefs.subsOff || subtitlesOffByDefault`. Exact 4-step: `if prefs.subsOff != null → return prefs.subsOff` (per-show wins, incl. explicit `false`); else `if subtitlesOffByDefault → true`; else `if prefs.subLang → false`; else `false`.
- **Audio autoload switch test:** switch when `want && (no current selection || current.id != want.id)` — by **track id difference**, not a score-vs-current compare.
- **Forced-sub pick (native-audio case):** `subtitleTracks.filter(isForcedTrack).sort(desc langScore vs subLangs)[0]` where `isForcedTrack` = `/\bforced\b/i` on title+label — NOT `pickBestTrack` (which skips forced).
- **Hotkey `[`/`]` speed reaches 3×** (beyond the menu's 2× ceiling) and must round each step to 2 decimals (`round(r*100)/100`).
- **Mute hotkey (`m`) does NOT persist** to disk (unlike volume up/down). Don't write `muted` to settings on `m`.
- **`e.repeat` on Space:** consume the event but arm the 350ms hold timer only once (Qt auto-repeat).
- **`end-file` reasons are lowercase** (`eof`/`stop`/`quit`/`error`/`redirect`/`other`); ignore stop/quit/redirect; non-`eof` → `status=Error, errorCode=Decode, errorMessage="mpv ended playback: {reason}"`.
- **`noAudio` is a dead field** (never written by the mpv path) — like `buffering`.
- **`sub-add` title fallback:** title → lang → literal `"Subtitle"` (never empty, else external tracks show blank labels).
- **Exact constants to capture now:** normalize af = `dynaudnorm=f=200:g=15:m=10:r=0.6`; screenshots need `screenshot-sw=yes` + `["screenshot-to-file", path, "video"]` (EXTRA); `ab-loop-a/b` clear sentinel = `"no"` (EXTRA).
- **Events emitted-but-ignored:** `playback-restart`, `seek`, `log` (no-ops); `shutdown` breaks the event loop only.

### 0.3 Verified-correct (critics confirmed against source — build with confidence)
All 15 observed properties (ids/formats), every `set_property` control name, the slot order + `1fr_auto_1fr`/`auto_1fr_auto` grid, gradients + padding, play-pause sizes 64/56/48, breakpoints **mid/compact/tight = 1300/1000/600**, `fmtTime`, autohide **1800/4500**, the 6 video-fill modes, hold-FF **350ms→max(2,base)**, seek ±10/±30, volume ±0.05/±0.5, sub-delay ±0.1/±0.05.

---

## 1. Overview & Scope

### 1.1 CORE this milestone
Recreate everything the user sees and feels, plus the mpv command/property logic, **1:1**:
- **Video surface** — QOpenGLWidget + libmpv render API.
- **Transport chrome** — default control bar: back, title/info, seek bar, time, play/pause, ±10s seek, volume, audio menu, subtitle menu, speed menu, fullscreen.
- **Track selection** — subtitle + audio track menus, sub styling (mpv-native/libass), track autoload rules.
- **Core behaviors** — full hotkey set, chrome auto-hide, fullscreen, video fill/zoom modes, click/double-click, loader UX.

### 1.2 Architecture at a glance
- Route entry `src/views/player.tsx` (`PlayerView`) is a thin orchestrator wiring ~40 hooks + ~10 "layer" components.
- Engine abstraction: `src/lib/player/bridge.ts` (the `PlayerBridge` interface → recreate as a C++ class) with mpv impl in `src/lib/player/mpv.ts` (every mpv command/property to recreate 1:1). HTML5 path is web-only — **EXTRA, skip entirely** (Tankoban is mpv-only).
- Chrome is **config-driven**: `src/lib/player-chrome.ts` defines control IDs, 7 slots, and `DEFAULT_DEFAULT_CONFIG` (the default control order). `transport.tsx` lays out slots; `control-renderer.tsx` renders each control. Recreate the **default** layout 1:1; the user re-ordering profile system is EXTRA.

### 1.3 Collapse the process boundary
Harbor splits the engine across Rust (`mpv.rs` owns `mpv_handle*`) and JS (`mpv.ts` calls 5 Tauri commands; events flow back as one `"mpv://event"`). **Qt collapses this into one C++ class** (call it `MpvController`/`MpvEngine`): the Rust pre/post-init options + the JS event translator + the JS bridge methods all become one in-process object holding one `mpv_handle*`. No IPC, no geometry-poll loop, no `wid`.

### 1.4 Phasing map (CORE now; EXTRAs grouped into future phases)

EXTRAs are noted by name only throughout; deep-spec is out of scope. Grouped phases:

| Phase | Feature | Key files (reference) |
|---|---|---|
| **Casting** | Chromecast/AirPlay/DLNA/Roku | cast-layer, `use-player-cast`, `cast-button`, `cast.rs`/`cast_hls.rs`/`cast_server.rs`/`cast_subs.rs`/`airplay.rs`/`dlna.rs`/`roku.rs` |
| **Together / Watch-Party** | room sync, avatars, chat, lobby | room-layer, `use-room-sync`/`use-lobby-gate`/`use-host-source`/`use-stremio-sync`, `avatar-dock`/`chat-overlay`, `discord_rp.rs` |
| **PiP** | picture-in-picture | `use-pip-mode`, `pip-chrome`/`pip-controls`/`pip-seek-bar`/`pip-volume`, `pip.rs`/`pip_mac.rs` |
| **DVR / Live-TV** | record, channel overlay | live-layer, `live-controls`, `dvr-button`/`dvr-modal/*`, `dvr.rs` |
| **Stream-Switcher** | in-player stream swap | `use-stream-switcher`/`use-stream-pill`, `stream-switcher/*`, `stream-check-pill` |
| **Episode-Panel** | up-next browser, auto-advance | panels-layer (episode), `use-episode-navigation`/`use-auto-next-episode`, `episode-panel/*` (prev/next **buttons** are CORE chrome, disabled if no series) |
| **Draw** | draw-on-video | `use-draw-mode`, `draw-canvas`/`pen-cursor`/`draw-toggle` |
| **Gif / Frame-grab** | screenshot, GIF record | `use-frame-grab`/`use-gif-recorder`, `gif-record-pill`/`quick-tools`, `capture-path.ts` |
| **Sleep / AB-loop / Skip-intro** | timers, loop region, skip pill | `use-sleep-timer`/`use-ab-loop`, `skip-pill`, speed-menu sleep half |
| **Multiview** | multi-pane playback | `multiview.rs` |
| **Anime4K / HDR** | shaders, motion interp | `anime4k-modes.ts`/`anime4k.rs`, `motion-interp.ts`, tone-mapping option |
| **Trailer** | trailer fetch/cache | `trailer.rs` |
| **Trickplay/thumbs** | seek thumbnail previews | `use-trickplay`, `thumb-preview`, `thumbs.rs` |
| **Debrid / Download** | download-while-watching | `use-video-download`, `download-button`, `download.rs` |
| **Auto-retry / Transcode** | stuck-stream recovery | `use-auto-retry`/`use-stub-detection`/`use-engine-stats`, `transcode.rs`/`stream_proxy.rs` |
| **Chrome-customize / Alt-shells** | profile editor, Stremio skin, minimal shell, custom icons | `player-chrome-profiles.ts`, `transport-stremio`, `shells/minimal-shell`, `custom-icon-renderer` |
| **Diagnostics (optional)** | stats HUD (`i` key) | `stats-overlay`, `proc_mem.rs` |
| **Web fallback (skip entirely)** | HTML5 `<video>` bridge | `lib/player/html5.ts`+`html5/*`, `webview_helpers.rs`, `modal_overlay.rs` |

CORE-adjacent extras to keep in mind but not deep-spec this milestone: resume-prompt, pause-on-inactive, screensaver-inhibit, resume-autosave, sub-drop.

---

## 2. Render Architecture (QOpenGLWidget + libmpv render API)

This mirrors Harbor's mac/linux render path (`mpv_render_mac.rs` / `mpv_render_linux.rs`), both of which use libmpv's `mpv_render_context_*` C API with `MPV_RENDER_API_TYPE_OPENGL`. The two platforms agree on the call sequence; Linux additionally pushes X11/Wayland display params (reference-only — Qt's `QOpenGLContext` owns the display binding).

### 2.1 Canonical C call sequence (what Qt mirrors)

```c
// 1. CREATE — after QOpenGLContext is current (QOpenGLWidget::initializeGL):
mpv_opengl_init_params gl_init = { .get_proc_address = get_proc, .get_proc_address_ctx = ctx };
mpv_render_param params[] = {
    { MPV_RENDER_PARAM_API_TYPE,           MPV_RENDER_API_TYPE_OPENGL },
    { MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init },
    { 0 }
};
mpv_render_context_create(&mpv_gl, mpv, params);

// 2. UPDATE CALLBACK — fires on an arbitrary mpv thread; schedule a GUI-thread repaint:
mpv_render_context_set_update_callback(mpv_gl, on_mpv_update, this);

// 3. RENDER — each QOpenGLWidget::paintGL, FBO + size in DEVICE pixels:
mpv_opengl_fbo fbo = { .fbo = (int)defaultFramebufferObject(), .w = w, .h = h };
int flip_y = 1;
mpv_render_param p[] = {
    { MPV_RENDER_PARAM_OPENGL_FBO, &fbo },
    { MPV_RENDER_PARAM_FLIP_Y,     &flip_y },
    { 0 }
};
mpv_render_context_render(mpv_gl, p);

// 4. TEARDOWN — make context current, then:
mpv_render_context_free(mpv_gl);
```

### 2.2 The decisive Qt mappings
- **`get_proc_address`** → `QOpenGLContext::currentContext()->getProcAddress(name)` (NOT raw `dlsym` — let Qt resolve symbols across its GL/GLES/ANGLE backends). Harbor uses `dlsym(RTLD_DEFAULT,...)` on mac (`mpv_render_mac.rs:342-348`), `glXGetProcAddressARB`/`eglGetProcAddress`+dlsym on linux (`mpv_render_linux.rs:83-95`) — reference only.
- **Update callback** (`mpv_render_context_set_update_callback`) runs off-thread → do `QMetaObject::invokeMethod(widget, "update", Qt::QueuedConnection)` (or emit a queued signal). **Coalesce** like Harbor: an atomic `REDRAW_PENDING` flag so only one `update()` is queued at a time (`mpv_render_mac.rs:350-364`, `mpv_render_linux.rs:331-344`). Never render inline in the callback.
- **FBO target** → pass `this->defaultFramebufferObject()` as `mpv_opengl_fbo.fbo`. This matches Harbor's **Linux** behavior, which queries `glGetIntegerv(GL_FRAMEBUFFER_BINDING)` (`mpv_render_linux.rs:110-120,293`) because GtkGLArea renders into its own FBO, not 0 — and warns if it's 0 → black video. QOpenGLWidget likewise renders into its own FBO, **never 0**; hardcoding 0 (the Mac path, `mpv_render_mac.rs:308`) would give a black video region. **Always use `defaultFramebufferObject()`.**
- **Size** → `w = width()*devicePixelRatioF()`, `h = height()*devicePixelRatioF()` (device/backing pixels), each clamped ≥1. Matches Harbor's `convertRectToBacking` (mac) / `allocated*scale_factor` (linux).
- **`MPV_RENDER_PARAM_FLIP_Y = 1`** → always pass it. GL's origin is bottom-left; mpv flips so the image is upright in the FBO. Both Harbor platforms pass `flip_y=true`.
- **Resize** → no manual surface reposition needed (QOpenGLWidget resizes its own FBO in `resizeGL`); just ensure the next `paintGL` reads the new `width()/height()`, and call `update()` after resize. (Harbor's mac/linux `setFrame`/`set_size_request` + `schedule_redraw` machinery is webview-embed plumbing — drop it.)
- **Teardown order** → in `~MpvWidget`/`destroy`: `makeCurrent()`, then `mpv_render_context_free(mpv_gl)`, then `doneCurrent()`, then `mpv_terminate_destroy`. Free the render context **on the GL thread while the context is still valid/current**; null the `mpv_gl` pointer under the redraw flag so a queued `update()` can't render into a freed context. Harbor free-orders carefully (even `mem::forget`s a stale ctx to avoid use-after-free, `mpv_render_mac.rs:75-78`).

### 2.3 Reference-only (do NOT recreate)
X11Display/WaylandDisplay params; the entire mac NSOpenGLView / linux GtkGLArea / Windows-`wid` embed machinery (subview insertion, autoresize masks, webview transparency, RGBA visuals, window rounding, `mpv_force_below`, `mpv_set_geometry`, geometry resustain timers). These exist solely to glue a GL surface beneath a webview. We own the GL surface via QOpenGLWidget.

---

## 3. mpv Contract

### 3.1 (a) Init options table

**PRE-INIT** (set before `mpv_initialize`; from `apply_pre_init`, `mpv.rs:160-234`). Always-set values (`mpv.rs:168-196`):

| Property | Value | Notes |
|---|---|---|
| `title` | `Tankoban` | (Harbor: `Harbor`, :168) |
| `audio-client-name` | `Tankoban` | (Harbor: `Harbor`, :169) |
| `terminal` | `no` | :170 |
| `msg-level` | `all=warn,vo=v,gpu=v` | :171 (drop d3d11/win32 levels) |
| `user-agent` | `VLC/3.0.20 LibVLC/3.0.20` | :172 — spoof for stream hosts |
| `input-default-bindings` | `no` | :188 — **app owns all hotkeys** |
| `input-cursor` | `no` | :189 |
| `osc` | `no` | :190 — no built-in OSD controller |
| `osd-level` | `0` | :191 |
| `cursor-autohide` | `200` | :192 (mpv-internal; Qt does its own chrome autohide §9.2) |
| `volume-max` | `600` | :193 — **required**, else mpv clamps at 100 |
| `background-color` | `#000000` | :194 (best-effort) |
| `background` | `color` | :195 |
| `media-controls` | `yes` | :196 (best-effort) |

**Render-API path** (Tankoban — all platforms):
- `vo = libmpv` (mandatory for the GL render API; `mpv.rs:313`), `force-window = no` (`mpv.rs:316`).
- **hwdec per-OS:** Windows `auto` (or `d3d11va`), Linux `auto-safe`, macOS `videotoolbox-copy` (`mpv.rs:173-187`).
- **Do NOT set** `wid`, `ontop`, `d3d11-flip`, `geometry`, `force-window=immediate` — webview hack only.
- `force_c_numeric_locale`: call `setlocale(LC_NUMERIC, "C")` once at engine init on non-Windows (libmpv float-parse requirement; `mpv.rs:125-133`).

**POST-INIT — cache/demuxer/network** (`mpv.rs:379-399`, recreate exactly):

| Property | Value | | Property | Value |
|---|---|---|---|---|
| `cache` | `yes` | | `cache-dir` | `<app_cache>/mpv-cache` |
| `cache-secs` | `60` | | `cache-on-disk` | `yes` |
| `cache-pause` | `yes` | | `network-timeout` | `600` |
| `demuxer-max-bytes` | `128MiB` | | `stream-buffer-size` | `32MiB` |
| `demuxer-max-back-bytes` | `32MiB` | | `stream-lavf-o` | `reconnect=1,reconnect_streamed=1,reconnect_delay_max=10,reconnect_on_network_error=1` |
| `demuxer-readahead-secs` | `60` | | | |

**POST-INIT — subtitle base styling** (`mpv.rs:401-408`):

| Property | Value |
|---|---|
| `sub-fonts-dir` | `<bundled fonts dir>` |
| `sub-font-provider` | `auto` |
| `sub-font` | `Noto Sans JP` |
| `embeddedfonts` | `yes` |

**Do NOT set** `sub-visibility=no` / `secondary-sub-visibility=no` (`mpv.rs:396-399`). Those hide mpv's native subs because Harbor's webview re-draws subs from `sub-text`. **Tankoban renders subs natively via libass through the render API — subtitles MUST be visible.**

**Conditional (mostly EXTRA):** `tone-mapping=bt.2446a` (when HDR→SDR — keep, only tone-map knob), `glsl-shaders` (Anime4K — EXTRA), `start=<sec>` (resume — CORE, also passed per-loadfile §3.2). Screenshot props set lazily (EXTRA).

### 3.2 (b) Commands table

mpv argv-style (`["cmd", arg, ...]`). Arg coercion: bool→`yes`/`no`, number→string, null→`""` (`mpv.rs:565-573`).

| Command (argv) | When | Source |
|---|---|---|
| `["loadfile", <url>, "replace"]` | first load (fresh start) | `mpv.rs:427` |
| `["stop"]` | before reusing mpv for a new source | `mpv.ts:245` |
| `["loadfile", <url>, "replace", 0, "start=<sec>"]` | reload into existing mpv (resume offset via 4th option-arg slot) | `mpv.ts:246-250` |
| `["seek", <sec>, "absolute", "exact"]` | **every** seek (bar + all hotkeys) — never relative, never keyframe | `mpv.ts:367` |
| `["sub-add", <url>, <flag>, <title>, [lang]]` | add external sub; flag=`select` (if select≠false) or `auto`; initial subs at start use `auto` | `mpv.rs:966-1011`, `:412` |
| `["af", "set", <filters>]` | audio-filter rebuild (normalize/profile — EXTRA) | `mpv.ts:70` |
| `["screenshot-to-file", <path>, "video"]` | screenshot/gif/cast-thumb — EXTRA | `mpv.rs:721` |
| `["quit"]` | session teardown | `mpv.rs:1084,1103` |

**Seek detail:** all seek paths (bar drag, ±10/±30 hotkeys, digit-seek) compute an **absolute** target and call `["seek", target, "absolute", "exact"]`. Seek-step = `max(0, currentPos + delta)` read from the live clock (§4). There is NO `frame-step`/`cycle`/relative seek. On seek, the bridge also clears `subText`/`subStartSec` (`mpv.ts:364-365`).

### 3.3 (c) Observed properties / state-snapshot table

`mpv_observe_property` registered once after init (`mpv.rs:419-423`). Observe **all 15** with matching `MPV_FORMAT_*`:

| Property | reply-id | Format | Drives snapshot field |
|---|---|---|---|
| `time-pos` | 1 | Double | `positionSec` (throttled ≥200ms in Harbor's Rust; replace with in-proc clock §4) |
| `duration` | 2 | Double | `durationSec` |
| `pause` | 3 | Flag | `status = paused`/`playing` |
| `eof-reached` | 4 | Flag | `status = ended` when true |
| `track-list` | 5 | Node | `audioTracks[]` + `subtitleTracks[]` (parse §8) |
| `volume` | 6 | Double | `volume = data/100` (mpv reports 0..600) |
| `mute` | 7 | Flag | `muted` |
| `chapter-list` | 8 | Node | `chapters[]` ({title, startSec}) |
| `sub-delay` | 9 | Double | `subDelaySec` |
| `audio-delay` | 10 | Double | `audioDelaySec` |
| `sub-text` | 11 | String | `subText` — webview sub-overlay only (Qt: ignore, libass renders) |
| `sub-start` | 12 | Double | `subStartSec` — webview only (Qt: ignore) |
| `af` | 13 | String | `audioNormalize = repr.includes("dynaudnorm")` |
| `dwidth` | 14 | Int64 | `videoWidth` |
| `dheight` | 15 | Int64 | `videoHeight` |

**NOT observed (known gaps):**
- `speed` — not observed; `rate` tracked optimistically via `setRate`, default 1.
- `paused-for-cache` / `demuxer-cache-time` / `demuxer-cache-duration` — not observed; `buffering`/`bufferedSec` are declared but **dead** (always 0/false). Harbor's seek-bar buffered fill is effectively inert.
- **Faithful improvement (flagged):** to populate a real buffered bar, additionally observe `demuxer-cache-time` (Double → `bufferedSec`) and `paused-for-cache` (Flag → `buffering`). Optional; Harbor's literal behavior is "no buffered indicator."

**Event-pump non-property mapping** (`mpv.ts:174-191`):
- `end-file`: decode `reason` from `mpv_event_end_file.reason` (`EOF/STOP/QUIT/ERROR/REDIRECT`). Ignore STOP/QUIT/REDIRECT; ignore if within `suppressEndFileUntil` (1500ms guard after in-place reload); `EOF` → `status=ended`; other → `status=error`, `errorCode="decode"`.
- `file-loaded` → `status=playing`, clear error fields.

**Threading:** use `mpv_set_wakeup_callback` to post a queued signal to the GUI thread, then drain `mpv_event` non-blocking in a GUI-thread slot. Never touch widgets from the mpv thread. (Harbor runs `wait_event(0.5)` on a dedicated thread, `mpv.rs:448-451`; the 200ms `time-pos` throttle is unnecessary in-process.)

### 3.4 (d) Volume curve

**Volume model is 0.0–6.0 (0%–600%)**, linear to mpv:
- UI domain `[0, 6]`, default `1.0` (`player-volume.ts:16-17`).
- Snapshot: `snap.volume = mpvVolume / 100` (`mpv.ts:99`).
- Write to mpv: `volume = round(uiVolume * 100)` (`mpv.ts:370`). UI 1.0 → mpv 100; UI 6.0 → mpv 600.
- **Mapping is LINEAR** (no log/perceptual curve). The *slider geometry* is non-linear (§7.2 boost curve) but the value↔mpv relation is linear.
- Keyboard step: `0.05` normal, `0.5` Shift; clamp `[0, 6]`.
- Persist: `{volume:0..6, muted:bool}`, default `{1, false}`.

---

## 4. Engine / Bridge Model → Proposed `MpvController`

### 4.1 `PlayerBridge` method surface (`bridge.ts:56-87`) — CORE bolded

```
attach(host) / detach()                              // QOpenGLWidget mount lifecycle
**load(src:{url, startAtSec?, subtitles?, notWebReady?})**   // async; §4.4
**play() / pause()**
**seek(sec)**                                        // absolute exact; clears subText/subStart
**setVolume(v 0..6) / setMuted(m)**
**setRate(r)**
**setAudioTrack(id) / setSubtitleTrack(id|null)**    // null = sid "no"
**setSubVisible(on)**
setSubDelay(sec) / setAudioDelay(sec)
setPanscan(0..1) / setVideoZoom(log2) / setAspectOverride(ratioStr)   // fill/zoom — CORE
**addSubtitle(url, lang?, title?, select?) -> bool**  // downloads http subs first
setAudioNormalize(on) / setAudioProfile(profile)     // EXTRA (af chain)
setMediaInfo({title,...})                            // force-media-title
screenshot(path)                                     // EXTRA
setAbLoop(a,b)                                        // EXTRA
requestPiP()/exitPiP()                               // EXTRA (no-ops in mpv bridge)
**requestFullscreen()/exitFullscreen()**
capabilities()                                       // {engine:"mpv", pip:true, airplay:false, chromecast:true, hdrPassthrough:false, hardwareDecode:true}
**subscribe(listener) -> unsubscribe**
destroy()
```

### 4.2 Control verbs → mpv `set_property` (port 1:1, `mpv.ts`)

| Bridge method | mpv property | Value |
|---|---|---|
| `play()` / `pause()` | `pause` | `false` / `true` |
| `setVolume(v)` | `volume` | `round(v*100)` (0..600) |
| `setMuted(m)` | `mute` | bool |
| `setRate(r)` | `speed` | r |
| `setAudioTrack(id)` | `aid` | `Number(id)||id` |
| `setSubtitleTrack(id\|null)` | `sid` | `"no"` if null else `Number(id)||id` |
| `setSubVisible(on)` | `sub-visibility` | bool |
| `setSubDelay(sec)` | `sub-delay` | sec |
| `setAudioDelay(sec)` | `audio-delay` | sec |
| `setPanscan(v)` | `panscan` | clamp(v,0,1) |
| `setVideoZoom(log2)` | `video-zoom` | log2 |
| `setAspectOverride(r)` | `video-aspect-override` | ratio string |
| `setMediaInfo(i)` | `force-media-title` | i.title |
| `requestFullscreen()` / `exitFullscreen()` | `fullscreen` | `true` / `false` (Qt: drive window directly, §9.3 — keep mpv `fullscreen` only if useful) |

### 4.3 `PlayerSnapshot` state shape (`bridge.ts:25-47`)

```cpp
struct PlayerSnapshot {
  enum Status { Idle, Loading, Ready, Playing, Paused, Ended, Error } status; // default Idle
  double positionSec, durationSec, bufferedSec;   // bufferedSec dead (always 0)
  bool   buffering;                                // dead (always false)
  double volume;        // 0..6, default 1  (= mpv volume/100)
  bool   muted;
  double rate;          // optimistic, default 1 (speed not observed)
  QVector<TrackInfo> audioTracks, subtitleTracks;
  QVector<Chapter>   chapters;   // {QString title; double startSec;}
  double subDelaySec, audioDelaySec;
  QString subText; double subStartSec;             // webview-only; Qt ignores
  bool   audioNormalize;                           // af contains "dynaudnorm"
  int    videoWidth, videoHeight;                  // dwidth/dheight
  QString errorMessage;                            // null/empty
  enum ErrorCode { None, Decode, Codec, Network, Source, Unknown } errorCode;
  bool   noAudio;
};
```

`TrackInfo` (`bridge.ts:1-16`): `{QString id; QString label; QString lang; enum {Audio,Subtitle} kind; bool selected; QString codec; QString channels; int channelCount; QString title; bool external; QString externalFilename; bool forced; bool isDefault; bool hearingImpaired;}`.

**Status transitions:** `load()` → `Loading` (clears tracks/subs/positions); `pause` prop → `Paused`/`Playing`; `eof-reached` → `Ended`; `file-loaded` → `Playing`; `end-file` non-eof → `Error`.

### 4.4 `load(src)` boot sequence (`mpv.ts:221-356`)
1. Snapshot → `Loading`, clear tracks/subs/positions, emit.
2. Connect the mpv-event pump once.
3. **Reuse path** (mpv already running): set `suppressEndFileUntil = now+1500ms`; `["stop"]`; `["loadfile", url, "replace", 0, "start=<sec>"]`; for each subtitle: if `http(s)` download to local file first (`sub_download`), then `sub-add(localPath, lang, title=null, select=false)`. (Faster than recreate.)
4. **Fresh path**: run the §3.1 init sequence + first `["loadfile", url, "replace"]`.
5. On error → `Error`, `errorCode=Source`.

`sub_download` (`mpv.rs:1031-1073`): GET with browser UA → gunzip if gzip-magic → sniff ext (`.vtt`/`.ass`/`.ssa`/`.srt` default) → write `temp/tankoban-subs/<uuid>.<ext>` → `sub-add`. Qt: `QNetworkAccessManager` + zlib.

### 4.5 Proposed Qt `MpvController` API

```cpp
class MpvController : public QObject {
  Q_OBJECT
public:
  // lifecycle
  void attach(MpvGlWidget* host);   void detach();   void destroy();
  void load(const PlaySource& src);
  // transport
  void play(); void pause();
  void seek(double sec);                      // ["seek", sec, "absolute", "exact"]
  void setVolume(double v);  void setMuted(bool m);
  void setRate(double r);
  void setAudioTrack(const QString& id);
  void setSubtitleTrack(const QString& id);   // empty/null → "no"
  void setSubVisible(bool on);
  void setSubDelay(double s);  void setAudioDelay(double s);
  // fill/zoom
  void setPanscan(double v);  void setVideoZoom(double log2);  void setAspectOverride(const QString& ratio);
  // subs
  bool addSubtitle(const QString& url, const QString& lang={}, const QString& title={}, bool select=true);
  void applySubStyle(const Settings&, bool assActive);   // §8.5 — 12 set_property calls
  // fullscreen (Qt drives window; optionally mirror mpv property)
  void requestFullscreen(); void exitFullscreen();
  PlayerSnapshot snapshot() const;
signals:
  void snapshotChanged(const PlayerSnapshot&);   // = bridge subscribe(); emit current snap once on connect
};
```

`subscribe(listener)` immediately invokes with the current snap (`mpv.ts:479`) → emulate by emitting the current snapshot once when a slot connects.

### 4.6 Playback clock (`playback-clock.ts`)
A tiny external store of `positionSec` + `bufferedSec`, **push-driven, fan-out de-duplication** (NOT a wall-clock interpolator). Fed `setPlaybackClock(s.positionSec, s.bufferedSec)` on every snapshot (`use-player-bridge.ts:90`), reset to (0,0) on teardown. Early-returns if unchanged. Also holds `seekHovering` (separate store so hovering the bar doesn't churn the snapshot). Decouples high-frequency time widgets (seek bar, time label) from the full-snapshot signal.

**Qt:** a singleton `PlaybackClock : QObject { double positionSec, bufferedSec; signals: void changed(); }`, fed from `snapshotChanged`; time/seek widgets bind to `changed()` only.
- (a) **Faithful:** relay latest `time-pos` (no extrapolation).
- (b) **Recommended (still faithful to intent):** drive the seek bar from a ~60Hz `QTimer` interpolating `lastTimePos + (now-lastTick)*speed` while `Playing`, snapping to the real `time-pos` on each observed tick. Either is acceptable; Harbor's literal behavior is (a).

---

## 5. Transport Chrome (default `Transport` bar)

### 5.1 Which bar is canonical
Three renderers exist; **recreate `Transport` (the "default" shell)**. `activeLayout()` returns `"sidebar"` by default (`theme.ts:561-564`); only Stremio-preset themes return `"stremio"`. Shell registry has exactly two shells: `"default"` (`Transport`) and `"minimal"`; default shellId = `"default"`. `TransportStremio` and `MinimalShell` are EXTRA alt-skins (§11 maps them for later).

### 5.2 Chrome container — two absolute overlay layers, both `z-20`, both `pointer-events-none` on the frame with `pointer-events-auto` on the actual control clusters. Fade via a `visible` boolean over **300ms** opacity transition.

**Top layer** (`transport.tsx:322-338`):
- `absolute inset-x-0 top-0 z-20 flex items-start justify-between`
- Gradient (downward): black **55% → 15% → transparent**.
- Padding: **L/R 28px** (`px-7`), **top 16px** (`pt-4`), **bottom 32px** (`pb-8`).
- `data-tauri-drag-region` (window-move grip → Qt `windowHandle()->startSystemMove()`).
- Children: left cluster = `top-left` slot (default **Back**); right cluster = `top-right` slot (default **Title/info**); each `flex items-start gap-2` (**8px** gap).

**Bottom layer** (`transport.tsx:340-388`) — main control bar:
- `absolute inset-x-0 bottom-0 z-20 flex flex-col gap-2.5` (vertical gap **10px** between seek row and control row).
- Gradient (upward): black **70% → 25% → transparent**.
- Padding normal: **L/R 28, top 40, bottom 20**; tight: **L/R 12, top 24, bottom 12**.
- **Row A (seek row)** `flex items-center gap-3` (**12px**): `seek-leading` (default `time-start`) → `SeekBar` (`flex-1`) → `seek-trailing` (default `time-end`).
- **Row B (control row)** = CSS grid:
  - Normal: `grid-cols-[1fr_auto_1fr] gap-4` (**16px**) → left/center/right thirds, center truly centered.
  - Compact: `grid-cols-[auto_1fr_auto] gap-2` (**8px**).
  - Cells: **bottom-left** `flex min-w-0 items-center gap-2 justify-self-start` (8px); **bottom-center** `flex items-center gap-1.5` (**6px**); **bottom-right** `flex items-center gap-1.5 justify-self-end` (6px).

**Responsive breakpoints** (ResizeObserver on control-container width, `transport.tsx:217-231`): `mid <1300`, `compact <1000`, `tight <600` (px). These gate which controls render and which sizes apply (§9.2 list).

### 5.3 DEFAULT control layout (`DEFAULT_DEFAULT_CONFIG`, `player-chrome.ts:162-189`)

Controls placed into 7 slots, sorted ascending by `order`, hidden ones filtered (`controlsInSlot`):

```
TOP BAR:    [Back]  ...........................................  [Title / subtitle  (i)]
SEEK ROW:   0:00  [============ seek bar ============]  -42:17
CONTROL ROW:
  left:    [Volume]  (·[DVR]·[Download])
  center:  (·[⏮ Prev ep])  [⟲10 Seek-back]  [▶/⏸ Play/Pause]  [Seek-fwd 10⟳]  (·[Next ep ⏭])
  right:   (·[Switch stream])  [Audio]  [Subtitles]  [Speed]  (·[Draw]·[PiP]·[Cast])  [Fullscreen]
```
Per-slot order: **top-left**: back(0). **top-right**: title-info(0). **seek-leading**: time-start(0). **seek-trailing**: time-end(0). **bottom-left**: volume(0), dvr(10)*, download(20)*. **bottom-center**: prev-episode(0)*, seek-back(10), play-pause(20), seek-forward(30), next-episode(40)*. **bottom-right**: pick-another(0)*, audio-menu(10), subtitle-menu(20), speed-menu(30), draw-toggle(40)*, pip(50)*, cast(60)*, fullscreen(70). (`*` = EXTRA, appears only under its condition.)

Options defaults: `volumeStyle: "slider"`, `timeFormat: "start-end"`.

### 5.4 CORE controls (exact specs; icons = lucide-react, see §10)

**back** (`control-renderer.tsx:121-134`): `Tooltip "Back" side=bottom`. Button **48×48** `rounded-full bg-black/55 text-white backdrop-blur-md hover:bg-black/80`. Icon `ChevronLeft size=26 strokeWidth=2.2` (`dir-icon` RTL-mirror). Action `onBack`.

**title-info** (`:135-175`): If clickable (has meta, not live), button `inline-flex items-center gap-2 rounded-lg px-2 py-0.5 text-end hover:bg-white/10`. Title `<h1>` `text-[19px] font-semibold leading-tight text-white drop-shadow-[0_2px_8px_rgba(0,0,0,0.6)]`. Subtitle `<p>` `text-[13px] text-white/70`. Trailing `Info size=14 strokeWidth=2.2` opacity 0.5→0.95 hover. Right-aligned (`items-end gap-0.5`).

**play-pause** (`:246-264`) — the hero button: `Tooltip "Play"/"Pause"`. `rounded-full bg-white/12 text-white backdrop-blur-md hover:bg-white/22 active:scale-95`. Size by breakpoint: tight **48**, compact **56**, default **64** (`h-16 w-16`). Icon `PlayCircle`/`PauseCircle` `strokeWidth=1.5`, size **36/32/28** (default/compact/tight). Qt: `QToolButton` 64px translucent circle; `active:scale-95` = small press `QPropertyAnimation`.

**seek-back / seek-forward** (`:242-245,265-268`): both hidden when `tight` or live. `SeekStepBtn` step **10s** (§6.2). Custom-fallback box **56×56** `rounded-full text-white/85 hover:bg-white/10`, icon 28.

**volume** (`:197-208`): hidden when `tight`. Renders `VolumeControl` (default style `slider`). §7.2.

**audio-menu** (`:299-314`): hidden when `tight` or html5; on live hidden unless ≥2 audio tracks. §8.5.

**subtitle-menu** (`:315-334`): on live hidden if 0 sub tracks; else **always shown** (not gated by tight/compact). §8.

**speed-menu** (`:335-345`): hidden when `compact` or live. §7.3.

**fullscreen** (`:369-383`): `BigButton`, tooltip "Fullscreen"/"Exit fullscreen". Icon `Maximize`/`Minimize` `size=22 strokeWidth=1.9`. Action `onFullscreen`.

**time-start / time-end**: §7.1.

### 5.5 Shared button primitives

**BigButton** (`big-button.tsx`) — used by pick-another/pip/cast/fullscreen/most custom-icon controls: **48×48** `rounded-full transition-[background-color,color,opacity]`. States: disabled → `text-white/30 cursor-not-allowed`; active → `bg-white/22 text-white hover:bg-white/30`; default → `text-white/85 hover:bg-white/10 hover:text-white`. Optional `Tooltip`. Qt: reusable `QToolButton` style (48px circle, three states).

**Tooltip** (`tooltip.tsx`): hover-only, **150ms** fade. Bubble `rounded-lg border border-white/10 bg-black/90 px-2.5 py-1 text-[12px] font-medium text-white backdrop-blur-md shadow-[0_10px_30px_-12px_rgba(0,0,0,0.7)]`. Side top → `bottom-[calc(100%+8px)]`; bottom → `top-[calc(100%+8px)]`; **8px** offset; align center or end. Qt: custom popup `QWidget` (NOT `QToolTip`), `enterEvent`/`leaveEvent`, 150ms fade, anchored 8px above/below.

### 5.6 Slot engine + menu-open hook
- `controlsInSlot(config, slot)` (`player-chrome.ts:267-275`): filter by slot, drop hidden, sort by order. This is the layout engine — fully data-driven. Qt: `struct ControlConfig { ControlId id; Slot slot; int order; bool hidden; };` + hardcoded default vector + a `controlsInSlot` equivalent.
- **Menu-open reporting:** when any of audio/subtitle/speed menus open, fire `onMenuOpenChange(true)` (`transport.tsx:202-204`) — the player view uses it to **suppress auto-hide while a menu is open** (§9.2). Recreate this hook.
- Profile customizer / custom icons / live-update events (`harbor:player-chrome-changed`, storage) = EXTRA; hardcode `DEFAULT_DEFAULT_CONFIG`.

### 5.7 Container Qt mapping
Overlay = a `QWidget` child of the video stack; frame `Qt::WA_TransparentForMouseEvents` with real widgets in clusters. Gradients = `paintEvent` `QLinearGradient` (top: black 0.55→0.15→0 down; bottom: 0.70→0.25→0 up). Fade = `QGraphicsOpacityEffect` + `QPropertyAnimation` 300ms. Row B = `QGridLayout` stretch `1,0,1` (normal) / `0,1,0` (compact). Seek row = `QHBoxLayout`, seek bar expanding. `backdrop-blur-md` ≈ ~12px backdrop blur (`QGraphicsBlurEffect`/frosted-panel paint; approximate OK).

---

## 6. Seek Bar Spec

Files: `seek-bar.tsx` (logic), `seek-bar-visual.tsx` (paint), `thumb-preview.tsx` (hover preview — EXTRA card UI).

### 6.1 Anatomy & geometry
- Root: `pointer-events-auto group/seek relative h-12` → **48px** tall hit-area row.
- Inner track: `absolute inset-x-0 top-1/2 -translate-y-1/2 cursor-pointer` — full width, vertically centered; this is the `ref`'d element for the math.

**Value state machine** (`seek-bar.tsx:26-36`): three nullable layers — `hover` (time under pointer), `scrub` (live drag), `pending` (optimistic after release). Displayed `value = scrub ?? pending ?? position`.
- `pct = clamp(value/dur, 0, 1) * 100` (played).
- `bufferedPct = clamp((position + buffered)/dur, 0, 1) * 100` (buffered = position **plus** buffered-ahead seconds, not absolute). `dur = durationSec || 1`.
- `pending` auto-clears when real position within **0.75s**: `if (abs(position-pending) < 0.75) setPending(null)`.

### 6.2 Pointer → time math + commit (`seek-bar.tsx:54-78`)
```
fromEvent(clientX): x = clamp((clientX - rect.left)/rect.width, 0, 1);  return x*dur
onMove:  setHover(fromEvent); if scrub≠null setScrub(fromEvent)
onLeave: setHover(null)
onDown:  setPointerCapture(); setPending(null); setScrub(fromEvent)
onUp:    releasePointerCapture(); if scrub≠null { onSeek(scrub); setPending(scrub); } setScrub(null)
```
**Click-to-seek AND drag-scrub are the same gesture** (pointer-down begins a scrub at that x). **The seek commits on RELEASE only** — during drag only the visual moves; the single `["seek", sec, "absolute", "exact"]` fires on pointer-up; `pending` holds the visual until mpv catches within 0.75s. Hover side-effect: `setSeekHovering(hover≠null || scrub≠null)` — global flag other chrome reads to stay visible.

**Qt:** subclass `QWidget` (not `QSlider` — layered paint). Three `std::optional<double>` members. `mousePressEvent`→scrub + `grabMouse()`; `mouseMoveEvent`→hover always, scrub if pressed; `mouseReleaseEvent`→`seekRequested(scrub)` + pending. A clock-signal slot clears `pending` when `|position-pending|<0.75`.

### 6.3 Visual layers (`seek-bar-visual.tsx`) — bottom→top paint order
Resolved sizing (`:34-44`):
- `accent = settings.seekBarColor || "oklch(0.78 0.13 60)"` (**default gold**; precompute sRGB ≈ warm gold for Qt).
- `baseHeight = clamp(seekBarHeight ?? 6, 3, 14)` → **default 6px**.
- `trackHeight = (scrubbing||hovered) ? baseHeight+2 : baseHeight` → **+2px on hover/scrub** (`transition-[height] duration-150`).
- `baseDot = clamp(seekDotSize ?? 16, 8, 64)`; `dotSize = scrubbing ? baseDot+4 : baseDot` → **default 16px, +4 scrubbing**.

| Layer | Class / value |
|---|---|
| 1 background track | `w-full rounded-full bg-white/15`, height=`trackHeight` |
| 2 buffered range | child, `h-full rounded-full bg-white/28`, width=`bufferedPct%` |
| 3 played fill | `absolute overflow-hidden rounded-full`, width=`pct%`, height=`trackHeight`, centered, `backgroundColor=accent` (gold) |
| 4 skip-segment markers (EXTRA data) | per span `absolute rounded-full bg-white/45`, left=`startPct%`, width=`max(0.4, endPct-startPct)%`, height=`max(2, trackHeight-2)` |
| 5 handle / SeekDot | at `left=pct%`, `-translate-x/y-1/2`, circle (`50%`), `width=height=dotSize`, `backgroundColor=accent`, `boxShadow: 0 0 0 4px rgba(0,0,0,0.45)` (4px dark ring), `transition duration-200` |

**Qt paintEvent order:** rounded-rect bg (white 15%) → buffered rect (white 28%) → accent fill (gold, width=pct) → segment ticks (white 45%) → handle (filled gold circle `dotSize` with 4px `rgba(0,0,0,0.45)` outline). Animate height 6→8 (150ms) and dot 16→20 (200ms) via `QVariantAnimation`.

### 6.4 Hover tooltip (no trickplay) (`seek-bar.tsx:105-112`)
When `hover≠null` and trickplay inactive: bubble `pointer-events-none absolute -top-9 -translate-x-1/2 rounded-md border border-white/10 bg-black/90 px-2 py-1 font-mono text-[12px] font-semibold tabular-nums text-white shadow-lg backdrop-blur-md`, `left=(hover/dur)*100%`, text=`fmtTime(hover)`. **`-top-9` = 36px above**, centered on cursor x. Qt: frameless overlay `QLabel` 36px above at hover x, black 90% bg, 1px white-10% border, 4px radius, monospace 12px.

### 6.5 Thumbnail / trickplay preview — EXTRA (chrome noted, backend deferred)
`thumb-preview.tsx`: `CARD_WIDTH=192`, `CARD_HEIGHT=108` (16:9), `BUCKET_SECONDS=2`, `BUFFER_PAD_SEC=4`. Card `left=pct%`, `bottom:calc(100%+8px)`, `overflow-hidden rounded-lg border border-white/10 bg-black/85 shadow-... backdrop-blur-md`. **CORE milestone:** ship §6.4 tooltip; leave a `ThumbPreview` stub.

### 6.6 Style/shape variants — EXTRA
`glass`/`pinstripe`/`rainbow`/`image` bar styles + custom/animated-GIF dot + the seek-bar settings panel (height 3–14, color presets, dot shape circle/square/image/hidden, dot size 8–64/200) are taste extras. **CORE = flat gold bar, circle dot, height 6, dot 16.**

---

## 7. Time / Volume / Speed Specs

### 7.1 Time display (`time-display.tsx`, `transport-utils.ts`)

**`fmtTime(sec)`** (`transport-utils.ts:1-9`):
```
if (!isFinite(sec) || sec < 0) return "0:00";
total=floor(sec); h=floor(total/3600); m=floor((total%3600)/60); s=total%60;
return h>0 ? `${h}:${MM}:${SS}` : `${m}:${SS}`;   // minutes UNPADDED below 1h, seconds always zero-padded
```
So `M:SS` under an hour (`7:05`), `H:MM:SS` at/over (`1:02:09`). Negative/NaN → `"0:00"`.

**time-start** (default, non-stremio): `font-mono text-[13px] tabular-nums text-white/85 drop-shadow-[0_1px_3px_rgba(0,0,0,0.7)]` = `fmtTime(position)`. Hidden when `tight` or live.

**time-end** (default): `font-mono text-[13px] tabular-nums text-white/65 drop-shadow-...`. Driven by `timeFormat` (default `"start-end"`):
- `start-end` → total `fmtTime(duration)`.
- `remaining` → `-fmtTime(max(0, duration-position))`.
- `elapsed-only` → renders nothing.
Hidden when `tight` or live.

Three formats total: elapsed-only / remaining (`-` prefix) / start-end (default). Default layout splits into TimeStart (left of bar) + TimeEnd (right). Qt: `QLabel`(s) bound to `PlaybackClock::changed`, monospace/tabular font, `fmtTime`; drop-shadow via `QGraphicsDropShadowEffect` or painted.

### 7.2 Volume control

**Curve constants** (`transport-utils.ts:11-34`) — port the four helpers **verbatim**:
```
VOL_MAX = 6;  NORMAL_FRACTION = 0.6;  TRACK_WIDTH = 120;   // 100% sits at 60% of slider travel
fractionFromValue(v): clamp v[0,6]; v<=1 ? v*0.6 : 0.6 + ((v-1)/5)*0.4
valueFromFraction(f): clamp f[0,1]; f<=0.6 ? f/0.6 : 1 + ((f-0.6)/0.4)*5
boostColor(v): v<=1 → #ffffff; else t=min(1,(v-1)/5), lerp rgb(249,115,22)#f97316 → rgb(220,38,38)#dc2626
```
So 0–100% occupies the first 60% of travel; 100–600% the last 40%; fill tints orange→red above 100%.

**Two implementations** share the curve. **Recreate the Stremio vertical pop-up as primary** (it's Harbor's default feel); horizontal slider is the alternate. Both: `allowBoost = engine==="mpv"` → `max=6`; `v = muted ? 0 : clamp(snap.volume, 0, max)`; `muted = snap.muted || v===0`; `boosting = v > 1.001`; `pct = round(v*100)`. Icons: `VolumeX` muted, `Volume2` otherwise (no distinct low/high). **Wheel = ±0.05 (±0.5 Shift)**, `preventDefault`. Mute → `set_property mute`; volume → `set_property volume = round(v*100)` (0–600).

**Vertical pop-up** (`stremio-volume.tsx`):
- Trigger mute button **64×64** `rounded-xl text-white/90 hover:bg-white/[0.05]`, icon size 28 strokeWidth 1.9.
- `showPopup = (hover||pinned) && style==="slider"` — expands on hover; stays while dragging/pinned.
- Popup `absolute bottom-full left-1/2 -translate-x-1/2 flex-col items-center rounded-[20px] border border-white/12 bg-black/85 px-3 py-4 shadow-[0_18px_50px_rgba(0,0,0,0.7)] backdrop-blur-md`, transition opacity+transform **200ms ease-out**, `origin-bottom`; hidden `scale-90 opacity-0`, shown `scale-100 opacity-100`.
- Track region `relative h-44 w-9` (**176px tall, 36px wide**). Inner bar `rounded-full bg-white/25`, width **6px→13px** when `barWide` (hover/pinned), `transition-[width] duration-150`. Fill `absolute inset-x-0 bottom-0 rounded-full`, height=`fractionFromValue(v)*100%`, background gradient `linear-gradient(to top, #ffffff 0%, #fde68a 30%, #f59e0b 60%, #dc2626 100%)`. Handle `absolute left-1/2 -translate-x-1/2 rounded-full border-2 border-white bg-white`, **15×15 → 21×21 when barWide**, `bottom=fillPct%`, `shadow 0 2px 8px rgba(0,0,0,0.65)`, `boostColor(v)` when boosting.
- `pct%` label below: `text-[11.5px] font-semibold tabular-nums`, `boostColor` when boosting.
- Pointer math: `f = clamp(1 - (clientY-rect.top)/rect.height, 0, 1)` (inverted Y, top=max); `next = valueFromFraction(f)`. `pinned` on pointerdown, cleared on up.

**Horizontal slider** (`volume-control.tsx`, alt): `flex items-center gap-2`. Mute button **48×48** rounded, icon 24 strokeWidth 1.75. Track `relative h-2 rounded-full bg-white/15`, **width 120px**. Fill width=`fractionFromValue(v)*100%`; bg solid `rgba(255,255,255,0.92)` ≤1×, else gradient white→`#f97316` (at 100% breakpoint)→`boostColor(v)`. Handle **14×14** rounded, `left=filledPct%`, white / `boostColor` when boosting, `boxShadow 0 2px 8px rgba(0,0,0,0.6)`. Boost readout `text-[12px] font-semibold` `pct%` in `boostColor`, minWidth 36.

**Qt:** Primary = vertical pop-up: mute `QToolButton` 64×64; `enterEvent` shows popup `QWidget` (176px track, black-85, 20px radius, drop-shadow), animate opacity+scale 200ms. Custom `paintEvent`: white-25 rail (width 6→13 animated), bottom-anchored `QLinearGradient` fill, white-bordered circular handle (15→21px). Wheel ±0.05/±0.5. Stepper/icon-only styles = EXTRA.

### 7.3 Speed menu (`speed-menu.tsx`)

- **Choices** `[0.5, 0.75, 1, 1.25, 1.5, 1.75, 2]` (`:39`), **default 1×** (row labeled "Normal" / hint "default"). Discrete only — no 0.25-min and no >2 in this menu. Select → `onRate(r)` + close → `set_property speed = r`. **No hold-to-speed in this component** (that's the Space hotkey, §9.1a).
- **Trigger** `h-11 min-w-11 rounded-full px-2 gap-1`, icon `Gauge size=22 strokeWidth=1.9`. Accent state (`bg-white/22 text-white hover:bg-white/30`) when open OR rate≠1 OR sleep active; else `text-white/85 hover:bg-white/10`. When rate≠1 shows inline `text-[11px] font-bold tabular-nums tracking-wider` label `current = |rate-1|<0.01 ? "1×" : "{rate}×"`. Tooltip "Playback speed" (or "Speed & sleep" if sleep present).
- **Popover** anchored `bottom-[calc(100%+10px)] end-0` (right-aligned, 10px above), `w-[400px] max-w-[calc(100vw-32px)]`, `rounded-2xl border border-edge bg-elevated/97 shadow-[0_24px_60px_-18px_rgba(0,0,0,0.8)] backdrop-blur-xl`. Grid `grid-cols-1` (or `grid-cols-2` when sleep timer present — EXTRA right column). Section title `text-[10.5px] font-semibold uppercase tracking-[0.18em] text-ink-subtle`. Row `h-10 w-full rounded-lg px-3 text-[14px]`; selected (`|r-rate|<0.01`) → `bg-elevated text-ink ring-1 ring-edge font-medium`, else `text-ink-muted hover:bg-canvas/55`. Dismiss on outside mousedown / Escape.

**Qt:** `QToolButton` (Gauge) 44px tall; append "{rate}×" + accent bg when rate≠1. Click → custom popup or `QMenu` 400px above-right with the 7 fixed rows; check via `|r-rate|<0.01`. Sleep column = later EXTRA.

---

## 8. Subtitle & Audio Track Menus + Sub Styling + Autoload

**mpv renders subs natively via libass through the render API.** `subtitle-overlay.tsx` is HTML5-only and is **NOT recreated** (its `sub-text`/`sub-start` props are webview artifacts).

### 8.1 Subtitle button + popup
**Button** (`subtitle-menu.tsx:114-126`): **48×48** `rounded-full`, lucide `Subtitles size=19 strokeWidth=2`. Open → `bg-white/22 text-white`; closed → `text-white/85 hover:bg-white/10`. **Active dot** when a sub selected: `absolute end-2.5 top-2.5 h-1.5 w-1.5 rounded-full bg-emerald-400` (**6×6px emerald**, inset 10px). Tooltip "Subtitles".

**Popup** (inline variant `:128-132`): `absolute bottom-[calc(100%+10px)] end-0`, **500×400px**, `max-h-[72vh] max-w-[calc(100vw-32px)]`, `flex flex-col overflow-hidden rounded-2xl border border-edge bg-elevated/97 shadow-[0_24px_60px_-18px_rgba(0,0,0,0.8)] backdrop-blur-xl`. Dismiss on outside mousedown. Qt: `Qt::Popup` frameless `QWidget`, rounded + drop-shadow.

### 8.2 Subtitle menu body (`menu-body.tsx`)
- **Header**: `border-b border-edge-soft px-4 py-2.5`; "Subtitles" `text-[13.5px] font-semibold` + count badge; right = style-bar button (`SlidersHorizontal size=18`, `h-9 w-9 rounded-full` → opens style bar §8.6, closes menu) + close X (`X size=16 strokeWidth=2.2`).
- **Left language rail** (`aside`): fixed **128px** wide, `gap-0.5 overflow-y-auto border-e border-edge-soft bg-canvas/30 p-2`.
  - **Off/On row**: `offSelected = selectedId==null`. On → "On" + filled accent check (`bg-accent text-canvas`, `Check size=9 strokeWidth=3`), clicking → `onSelect(null)` + close. Off → "Off" disabled muted.
  - **"All languages"** row (shown if >1 group): `Languages size=14`, sets `activeLang="__all__"`, count=`tracks.length`.
  - **Per-language groups** (`groupByLang`): Flag + lang name + accent dot if selected track in group + variant count. Active group `bg-elevated text-ink ring-1 ring-edge`; inactive `text-ink-muted hover:bg-elevated/60`.
- **Right pane**:
  - **Filter bar**: 3 source tabs `All` / `Embedded` / `External` (pill `h-7 rounded-full px-2.5 text-[11.5px] font-semibold`; active `bg-elevated text-ink ring-1 ring-edge`; disabled if count 0). Toggle chips **HI** (`active=!hideHI`) + **Forced** (`forcedOnly`): `h-6 rounded-full px-2 text-[11px] font-semibold`; active `bg-accent text-canvas`, inactive `bg-raised text-ink-muted`.
  - **Filtering**: drop `embedded&&external`, `external&&!external`, `hideHI&&hearingImpaired`, `forcedOnly&&!forced`.
  - **Track list**: scrollable `VariantRow` per track; click → `onSelect(id)` + close.
  - **Bottom bar**: "Find more subtitles" (online search — EXTRA) + **"Load file"** (`FolderOpen size=12`).
  - **Delay row** at bottom (§8.4).
- **Load-from-file** (`loadLocal`, CORE): native dialog filtered `["srt","ass","ssa","vtt","sub"]`; on pick → `onAddSubtitle(path, undefined, basename)`; success → mark imported, switch to "all languages", show banner, auto-close after **1700ms**; failure → inline error. Qt: `QFileDialog::getOpenFileName` filter `Subtitles (*.srt *.ass *.ssa *.vtt *.sub)`.

### 8.3 VariantRow (`variant-row.tsx`)
Button row: selected check `h-4 w-4 rounded-full` (selected `bg-accent text-canvas` + `Check size=9`); row bg selected `bg-elevated ring-1 ring-edge`, imported `bg-accent/[0.07] ring-1 ring-accent/30`, hover `hover:bg-canvas/55`. Title `text-[12.5px] font-medium` (track.title || "External/Embedded track"). "Yours" badge (`Sparkles`) if imported. Meta line `langName · sourceLabel · CODEC` + tag chips: **Forced** (sky), **HI/SDH** (amber), **Default** (neutral). Codec uppercased.

### 8.4 Delay / sync rows
**Subtitle sync** (`delay-row.tsx`): `border-t border-edge-soft bg-canvas/30 px-3 py-2`; label "Sync"; **six step buttons −0.5, −0.1, −0.05, [value], +0.05, +0.1, +0.5 s**; center readout `+X.XXs`/`X.XXs` monospace (`delay.toFixed(2)`); Reset (`RotateCcw size=11`) when `delay≠0`. Rounding `round(v*100)/100`; **no clamp**. → `set_property sub-delay` (float s).

**Audio sync** (`audio-menu.tsx:247-296`): **only two steps −0.1, +0.1 s**; same readout/reset; disabled when html5. → `set_property audio-delay`.

### 8.5 Audio menu (`audio-menu.tsx`)
**Button** identical **48×48** circle but lucide `Languages size=19` (audio uses Languages; subs use Subtitles); no active dot; tooltip "Audio tracks". **Popup** `bottom-[calc(100%+10px)] end-0`, **360px wide** (narrower), `max-h-[400px]`, same chrome. Header "Audio" + count + close X (`size=13`, `h-7 w-7`). Track rows: selected check + Flag + title (`title` meaningful, else langName, else label) + subtitle line `langName · codec · channels · Default`. Click → `set_property aid` (numeric id). Empty: mpv → "This file has one audio track."

### 8.6 Sub styling → mpv properties (`sub-style.ts`) — THE CORE MAPPING
`applySubStyle(settings, assActive)` issues these `set_property` calls (recreate exactly):

| mpv property | value |
|---|---|
| `sub-font-size` | literal **32** (base; scaling via sub-scale) |
| `sub-font` | `mpvFontFor(subFontFamily)` |
| `sub-scale` | `clamp(subFontSize/32, 0.4, 4)` |
| `sub-color` | `hexToBgr(subFontColor)` |
| `sub-border-color` | `hexToBgr(subBorderColor)` |
| `sub-border-size` | `subBorderSize` (int) |
| `sub-margin-y` | `subMarginY` (int) |
| `sub-align-x` | `subAlignX` (left/center/right) |
| `sub-ass-override` | `subAssOverride` (no/scale/force) |
| `sub-ass-force-margins` | `assActive && override!=="no" ? "yes":"no"` |
| `sub-use-margins` | same as force-margins |
| `sub-spacing` | `subLineSpacing` |

`mpvFontFor`: inter→Inter, system→Segoe UI, serif→Times New Roman, rounded→Segoe UI, arabic→Noto Sans Arabic. `hexToBgr`: `#RRGGBB` passed through uppercase (mpv accepts `#RRGGBB`), else `#FFFFFF`. Bundle Inter + Noto Sans Arabic fonts. **Do not build any HTML/QPainter sub overlay** — libass draws into the GL surface.

### 8.7 Sub-style bar + presets + settings panel (CORE-lite)
- **Sub-style bar** (`sub-style-bar.tsx`): floating top-center toolbar opened from the subtitle menu's sliders button; auto-closes after `IDLE_MS=7000ms`; Escape closes. Controls L→R: FontMenu, SizeStepper (drag-scrub `±dx/6`, clamp **16–120**), ColorRow (6 swatches `#FFFFFF #FFE45E #9AE6B4 #93C5FD #FCA5A5 #C4B5FD` + custom), OutlineToggle (shadow→outline→box; outline shows thickness stepper **1–6**), SpacingStepper (**0–12**), PlacementPad (marginY ±2, **0–40**; align dots), AssOverrideToggle (no↔force), PresetCluster, Done X. Each change writes setting + re-runs `applySubStyle` (live).
- **Presets** (`sub-presets.ts`): cap **12**, key `harbor.sub.presets.v1`. SEED: **English** (shadow/inter/28/white/border0/marginY10/center/ass=scale), **Foreign** (outline/inter/40/white/border2/marginY14/center/scale), **Arabic** (outline/arabic/40/white/border2/marginY14/center/force). Each preset = 13-field snapshot; "active" = field-by-field equality.
- **Settings sub-style panel** (`subtitle-section.tsx`): full surface mirroring the bar — background mode (shadow/outline/box), ASS mode (no/scale/force), box opacity 0.2–1.0 step 0.05, outline 1–6, font picker, size 16–120, opacity 0.2–1.0, distance-from-bottom 0–40, alignment, text/outline/box colors, Reset to defaults (shadow/inter/**32**/white/border0/marginY**12**/center/box0.6/opacity1). Qt: standard settings form; each `onChange` writes setting + re-applies.

### 8.8 Track-list parse + autoload rules

**Parse** (`mpv.ts:101-152`) — observe `track-list` (`MPV_FORMAT_NODE`), per entry read `type, id, lang/language, title, codec-desc/codec, demux-channels, demux-channel-count, external, external-filename, forced, default, hearing-impaired, selected`. Split `type==="audio"` → audioTracks, `type==="sub"` → subtitleTracks. `label = baseLabel · CODEC(upper) · channels · Forced · SDH · External`. `baseLabel = title || lang || "<type> <id>"`. Chapters: `{title, startSec:time}`, finite ≥0, sorted. `groupByLang` keys by `languageName(lang).toLowerCase()` (""→"Unknown"), preserves discovery order.

**Selection commands:** `sid` (int or `"no"`), `aid` (int), `sub-visibility` (bool). `addSubtitle` → `sub-add` (remote downloaded first).

**Auto-select rules** (`use-track-autoload.ts:188-264`, CORE — online search EXTRA):
- `resolveLangPreference`: use `preferredSubLangs`/`preferredAudioLangs` if non-empty, else `preferredLanguages`, else `["English"]`. Per-show override prepends `prefs.subLang`/`prefs.audioLang`.
- **Anime detection**: `meta.id` starts `kitsu:`/`mal:` or genres include "anime". For non-anime, strip Japanese (`ja/jpn/jp/japanese`); for anime keep.
- **Block-words**: drop tracks whose title/label contains any `trackBlockWords` (case-insensitive); if that empties, keep original.
- **Audio**: `pickBestTrack(allow(audioTracks), audioLangs)`; if better than current → `setAudioTrack`.
- **Subtitles**: `subsOff` (prefs.subsOff || settings.subtitlesOffByDefault) → if a sub selected, turn off. Else if none selected & subs exist & pref list exists: **native-audio-forced case** (if `forcedSubsWhenNativeAudio` && chosen audio lang matches a preferred sub lang → pick best **forced** sub) else `pickBestTrack(allow(subtitleTracks), subLangs)`.
- `pickBestTrack` (`language.ts:76-93`): skip forced; score = `langScore*10 + (default?1:0)`; fallback `tracks[0]`. `langScore`: exact code match at pref index `i` → `(len-i)*2`; base-lang match → `(len-i)*2-1`; none → `-1`.
- **Per-show prefs applied once** on first track-load: `prefs.rate`→`setRate`, `prefs.subDelaySec`→`setSubDelay`. Sub-style applied on track-load (mpv only).

Qt: run this on every `track-list` update; port `langScore`/`pickBestTrack` directly to C++.

---

## 9. Behaviors

### 9.1 Complete hotkey table

Bindings from `hotkeys.ts:47-88` (user-overridable). Dispatcher = single global `keydown` (`use-keyboard-shortcuts.ts`). Matching: **letters lowercased; Shift NOT in the binding string for letter keys** (so `m`/`s`/`f` match regardless of Shift). Gate: skip if focus is in INPUT/TEXTAREA/SELECT/contentEditable/role textbox|searchbox|combobox.

| Key | Action | Magnitude / behavior | Source |
|---|---|---|---|
| **Space** | Play/pause toggle + hold-to-speed | tap → toggle; **hold 350ms** → speed (§9.1a); `preventDefault`; ignore `e.repeat` | `:105-118` |
| **Escape** | Close player | exits drawMode first if on, else `closePlayer()` | `:100-104` |
| **f** | Fullscreen toggle | `toggleFullscreen()` | `:160-164` |
| **i** | Stats overlay toggle | EXTRA HUD | `:210-214` |
| **v** | Cycle aspect/crop | `videoFill.cycle()` (§9.4) | `:165-169` |
| **=** | Zoom in | `videoFill.step(0.1)` | `:170-174` |
| **-** | Zoom out | `videoFill.step(-0.1)` | `:175-179` |
| **ArrowLeft** | Seek −10s | `seekStep(-10)` | `:119-123` |
| **ArrowRight** | Seek +10s | `seekStep(10)` | `:124-128` |
| **,** | Seek −30s | `seekStep(-30)` | `:129-133` |
| **.** | Seek +30s | `seekStep(30)` | `:134-138` |
| **Home** | Jump to start | `seekTo(0)` | `:200-204` |
| **End** | Jump to end | `if dur>0: seekTo(dur-0.5)` | `:205-209` |
| **0** | Seek to 0 | absolute | `:280-284` |
| **1–9** | Seek to N×10% | `seekTo(dur*digit/10)` if dur>0 | `:285-292` |
| **ArrowUp** | Volume up | `+0.05` (Shift `+0.5`), clamp [0,6], persist | `:139-146` |
| **ArrowDown** | Volume down | `-0.05` (Shift `-0.5`), floor 0, persist | `:147-154` |
| **m** | Mute toggle | `setMuted(!muted)` | `:155-159` |
| **s** / **c** | Cycle subtitles | `cycleSubtitles()` (OFF→sub0→sub1→…→OFF) | `:185-189` |
| **z** | Sub delay −0.1s | (Shift −0.05); `round2`; persist | `:229-236` |
| **x** | Sub delay +0.1s | (Shift +0.05); `round2`; persist | `:237-244` |
| **[** | Speed −0.25× | `max(0.25, rate-0.25)`; persist | `:215-221` |
| **]** | Speed +0.25× | `min(3, rate+0.25)`; persist | `:222-228` |
| **n** / **b** | Next / prev episode | only if available (EXTRA-gated) | `:190-199` |
| **MediaPlayPause / MediaTrackNext/Previous** | media keys | bypass registry | `:84-98` |

**Cycle subtitles** (`use-playback-controls.ts:45-63`): if none selected → `subs[0]`; else advance to `idx+1`; past end → `setSubtitleTrack(null)`. Order **OFF → sub0 → sub1 → … → OFF**. Each choice persisted via `rememberSubChoice` (`{subLang}` or `{subsOff:true}` per metaId).

**EXTRA keys (one line):** `p` screenshot, `o` GIF, `w` stream-switcher, `e` episode-panel, `g` TV-guide, `r` DVR, `l` sleep, `h` prev-channel, `/` global search (outside player).

**9.1a Hold-to-speed on Space** (`use-keyboard-shortcuts.ts:105-118,294-316`): on Space keydown (non-repeat) record `baseRate = snap.rate`, start a **350ms** singleShot timer. If still held at 350ms → `holdSpeedActive=true`, `setRate(max(2, baseRate))` (min **2.0×**). On keyup: if timer still pending (released <350ms) → it was a **tap** → cancel + `playPauseToggle()`; if engaged → restore `baseRate`, clear flag. Also released on window `blur`. While engaged, the hold-speed pill (§9.6) shows. Qt: press-timestamp + `QTimer(350ms, singleShot)`; release decides tap-vs-hold by whether the timer fired.

**Qt dispatcher:** one app-level `keyPressEvent`/event filter; skip when `focusWidget` is `QLineEdit`/`QTextEdit`/`QComboBox`. Load-bearing constants: seek ±10s/±30s; volume ±0.05/±0.5; speed ±0.25 [0.25,3]; sub-delay ±0.1/±0.05; hold-FF 350ms→max(2,base).

### 9.2 Chrome auto-hide (`use-chrome-visibility.ts`, `player-utils.ts:21-22`)
```
CHROME_HIDE_MS_PLAYING = 1800   // idle timeout while playing
CHROME_HIDE_MS_PAUSED  = 4500   // idle timeout while paused
```
**`wakeChrome()`**: show chrome, clear pending hide timer, then — **unless a menu is open OR the seek bar is hovered (`getSeekHovering()`)** — arm a hide timer for `playing && !drawMode ? 1800 : 4500` ms; on fire → hide.

**Wakes chrome (show + re-arm):** `mousemove`/`touchstart` anywhere; seek-bar hover begins (force-show, cancel timer; ends → wakeChrome); menu opens (force-show, cancel; closes → wakeChrome).
**Hides immediately:** mouse leaves document (`mouseout` no relatedTarget) only if playing && !drawMode && no menu && not seek-hovering; window `blur`.
**Cursor:** `cursor:none` when drawMode OR (`!chromeVisible && playing`); else default — cursor disappears with the chrome during playback.

Fade: `transition-opacity duration-300` (300ms; no separate constant — opacity toggled by `chromeVisible`).

**Gating (player.tsx):** `showChrome = !loaderActive && !loaderShowing && (chromeVisible || drawMode)` — chrome only shows when the loader is fully gone (it reports its 320ms fade-out via `loaderShowing`).

**Qt:** chrome overlay `QWidget`; opacity via `QGraphicsOpacityEffect` + `QPropertyAnimation` (300ms). One `QTimer` (singleShot, restarted on every `mouseMoveEvent`) interval 1800/4500. `setCursor(Qt::BlankCursor)`/`unsetCursor()`. `setMouseTracking(true)`.

### 9.3 Fullscreen (`use-fullscreen.ts`)
`toggleFullscreen()` desktop path: toggle a `bool fullscreen` + enter/exit native fullscreen. The `kickRepeatedly()` geometry resustain (`[60,160,320,640,1100,1700,2400,3200,4200]ms`) + 2s sustain loop are webview compositing hacks — **drop them**. Fullscreen does NOT itself hide chrome (same auto-hide logic in all states). On unmount while FS → exit (cleanup parity). **Qt:** `window->isFullScreen() ? showNormal() : showFullScreen()` on the top-level window; keep `bool m_fullscreen` + a signal so chrome reacts. No HWND/geometry kicking — QOpenGLWidget repaints on resize.

### 9.4 Video fill / zoom (`use-video-fill.ts`)
Six modes, cycled by `v` in this exact order; persisted to `settings.cropMode` (default `"fit"`):

| idx | id | pill label | panscan | `video-aspect-override` | `video-zoom` |
|---|---|---|---|---|---|
| 0 | fit | `Fit` | 0 | `-1` | 0 |
| 1 | fill | `Fill` | 1 | `-1` | 0 |
| 2 | zoom | `Zoom` | 0 | `-1` | variable |
| 3 | 16:9 | `16:9` | 0 | `16:9` | 0 |
| 4 | 4:3 | `4:3` | 0 | `4:3` | 0 |
| 5 | original | `2.39:1 (Original)` | 0 | `2.39:1` | 0 |

`apply(i, zoomLevel)` sets three mpv props: `panscan` (clamp 0–1), `video-aspect-override` (`-1` = native), `video-zoom` (`mode.id==="zoom" ? zoomLevel : 0`). `keepaspect` is **never** touched. Cycle `next=(index+1)%6`; leaving zoom resets `zoom.current=0`.

**Zoom step** (`=`/`-` → `step(±0.1)`): forces mode to `zoom`, `zoom.current = clamp(round((zoom+delta)*100)/100, 0, 1)` — **log2 range [0,1], step 0.1**. Pill text in zoom = `Zoom {round(2^zoomLevel * 100)}%` (zoom 0.1 → "Zoom 107%", 1.0 → "Zoom 200%"); else `mode.label`. Pill auto-dismiss **1200ms**.

**Qt:** `QVector<CropMode>`; `apply()` = three `mpv_set_property`; pill = `QLabel` overlay shown via `QTimer(1200ms, singleShot)`.

### 9.5 Click / double-click (`drag-click-stage.tsx`, `player.tsx:559-564`)
Transparent full-stage hit layer `z-[3]`, `onClick = playPauseToggle`, `onDoubleClick = toggleFullscreen`.
- mousedown: only bare surface (`e.target===e.currentTarget`), left button only, not drawMode/pipMode; record start x/y/time.
- mousemove: a **drag** is recognized only if elapsed ≥ **150ms** AND movement > **8px** (either axis) → start native window drag (**reference-only**, drop for a normal-titlebar Qt window).
- mouseup: if no drag → `onClick()` = **play/pause**.
- doubleclick → **toggle fullscreen**.

**Qt:** override `mousePressEvent`/`mouseMoveEvent`/`mouseReleaseEvent` on the stage widget; use `mouseDoubleClickEvent` for fullscreen and gate the single-click play/pause behind a short `QTimer` (`QApplication::doubleClickInterval()`) so a double-click doesn't also toggle play. Drop the window-drag-on-video behavior. There are two click paths in Harbor (videoMount div + DragClickStage z-3); the z-3 layer is primary — implement just that.

### 9.6 Stage overlays (`stage-overlays.tsx`) — pills + subtitle
All gated `!pipMode` (read as "always render"). Pill color tokens: `--color-canvas oklch(0.18 0.004 260)`, `--color-ink oklch(0.97 0.003 260)`, `--color-ink-muted oklch(0.72 0.003 260)`.

- **Hold-speed pill** (CORE): when `holdSpeedActive`. `absolute left-1/2 top-8 z-30 -translate-x-1/2 rounded-full bg-canvas/85 px-3.5 py-1.5 text-[13px] font-semibold text-ink backdrop-blur-md`. Content `{rate}x` + `<span font-normal text-ink-muted>speed</span>`. **Top 32px**, centered.
- **Video-fill pill** (CORE): when `videoFillPill && !holdSpeedActive`. Same position/style; content from `useVideoFill().pill` (§9.4).
- **Sub-drop toast** (CORE-adjacent): when `subDropToast`. `absolute bottom-28 left-1/2 z-30 -translate-x-1/2 rounded-full bg-canvas/90 px-4 py-2 text-[13px] font-medium text-ink backdrop-blur-md`. **Bottom 112px**, centered. From sub-drop (`use-sub-drop.ts`): drag-drop `.srt/.ass/.ssa/.vtt/.sub` → `addSubtitle(path, select:true)`, toast 2200ms.
- **Stats HUD** (EXTRA-optional): `i`-key toggle; monospace `QLabel` HUD top-corner.
- **SubtitleOverlay** (HTML5-only): NOT recreated for mpv.

Qt: each pill = frameless `QWidget`/`QLabel`, `WA_TransparentForMouseEvents`, rounded-rect (radius = height/2) filled canvas@0.85–0.90, ink text; top pills y=32 centered, bottom toast y=(height−112) centered; auto-dismiss on timer.

### 9.7 Persisted preferences
- **Per-show** (`player-prefs.ts`): key `harbor.player.prefs.v1`, `Record<metaId, {rate?, subDelaySec?, audioLang?, subLang?, subsOff?, updatedAt}>`, cap **200** (LRU by updatedAt). Qt: `QHash<QString, PerShowPrefs>` → JSON, trimmed to 200.
- **Global volume/mute** (`player-volume.ts`): key `harbor.player.volume.v1`, `{volume:0..6 default 1, muted:false}`, volume clamped [0,6] on read. Qt: two `QSettings` keys.
- **Crop mode**: `settings.cropMode` (default `"fit"`).
- Other defaults: `pauseMinimized:true`, `pauseUnfocused:false` (auto-pause on minimize/unfocus, resume on refocus if it auto-paused), `subFontSize:32`, `subMarginY:12`, `subAlignX:"center"`.

---

## 10. Visual Tokens (consolidated)

**Colors / opacities** (Tailwind `x/NN` = alpha NN%):
- Top gradient: black **0.55 → 0.15 → 0** (downward). Bottom gradient: black **0.70 → 0.25 → 0** (upward).
- Seek: bg track white **15%**; buffered white **28%**; fill **gold `oklch(0.78 0.13 60)`** (precompute sRGB); markers white **45%**; dot ring `rgba(0,0,0,0.45)`.
- Volume: track white **15%**; vertical rail white **25%**; fill solid `rgba(255,255,255,0.92)` ≤1×; boost lerp `#f97316`→`#dc2626`; vertical gradient `#ffffff → #fde68a@30% → #f59e0b@60% → #dc2626@100%`.
- Buttons: back `bg-black/55` hover `/80`; play-pause `bg-white/12` hover `/22`; BigButton default `text-white/85` hover `bg-white/10`, active `bg-white/22`.
- Menus/popovers: `bg-elevated/97`, `border-edge`, shadow `0_24px_60px_-18px_rgba(0,0,0,0.8)`, `backdrop-blur-xl`.
- Tooltip: `bg-black/90`, `border-white/10`, shadow `0_10px_30px_-12px_rgba(0,0,0,0.7)`.
- Pills: `--color-canvas oklch(0.18 0.004 260)` @85–90%; `--color-ink oklch(0.97 0.003 260)`; `--color-ink-muted oklch(0.72 0.003 260)`.
- Sub-active dot `bg-emerald-400`. Tags: Forced=sky, HI/SDH=amber, Default=neutral.
- Sub swatches: `#FFFFFF #FFE45E #9AE6B4 #93C5FD #FCA5A5 #C4B5FD`.

**Spacing / padding:**
- Top bar `px-7 pt-4 pb-8` (28/16/32). Bottom bar `px-7 pt-10 pb-5` (28/40/20) normal, `px-3 pt-6 pb-3` (12/24/12) tight.
- Row gaps: seek-row `gap-3` (12); rows `gap-2.5` (10); control-grid `gap-4` (16, compact `gap-2`=8); top clusters `gap-2` (8); bottom-center/right `gap-1.5` (6).
- Menu header `px-4 py-2.5`; lang rail `p-2`, width 128px; delay row `px-3 py-2`.

**Sizes:**
- Buttons: back 48, BigButton 48, sub/audio 48, play-pause **64/56/48**, seek-step 56, mute(vertical) 64, speed `h-11`(44), menu close `h-9 w-9`/`h-7 w-7`.
- Seek: hit-row 48; bar 6 (+2 hover, range 3–14); dot 16 (+4 scrub, range 8–64); ring 4px; tooltip 36px above.
- Volume: horizontal track 120×8, handle 14; vertical track 176×36, bar 6→13, handle 15→21.
- Popups: subtitle 500×400 (max-h 72vh), audio 360 (max-h 400), speed 400-wide; all 10px above, right-aligned (end-0).
- Thumb card (EXTRA) 192×108.

**Typography:**
- Title 19px semibold white; subtitle 13px white/70.
- Time 13px mono tabular-nums (start white/85, end white/65); stremio 14px.
- Tooltip 12px medium; seek-tooltip 12px mono semibold.
- Menu header 13.5px semibold; section header 10.5px uppercase tracking-0.18em; row 14px; track title 12.5px; meta 11–12px.
- Pills 13px (semibold/medium); volume pct 11.5–12px; speed inline 11px bold.
- Loader title 64px display serif (Sentient→Iowan→Georgia→serif); episode label 12.5px uppercase tracking-0.32em white/70.

**Radii:** buttons `rounded-full`; menus/popovers `rounded-2xl`; vertical-vol popup `rounded-[20px]`; tooltip/banner `rounded-lg`/`rounded-md`; seek elements `rounded-full`; style-bar `rounded-[14px]`.

**Animation:** chrome fade 300ms; tooltip fade 150ms; seek height 150ms / dot 200ms; vertical-vol popup 200ms; vol bar-width 150ms; pill auto-dismiss 1200ms; sub-drop toast 2200ms; loader pulse 0.42↔1 over 2.4s; backdrop blur 28px; loader fade 300ms (unmount 320ms).

---

## 11. Proposed Qt File Structure & Component Mapping

### 11.1 Harbor → Qt component map

| Harbor construct | Qt class / file |
|---|---|
| `mpv.rs` + `mpv.ts` + `bridge.ts` (engine) | `engine/MpvController.{h,cpp}` (one `mpv_handle*`, init/commands/properties/events, snapshot emit) |
| `mpv_render_mac/linux.rs` (render path) | `engine/MpvGlWidget.{h,cpp}` : `QOpenGLWidget` (render-context create/render/teardown, §2) |
| `bridge.ts` `PlayerSnapshot`/`TrackInfo` | `engine/PlayerSnapshot.h` (structs) |
| `playback-clock.ts` | `engine/PlaybackClock.{h,cpp}` : `QObject` singleton |
| `player.tsx` (orchestrator) + stage `<main>` | `player/PlayerView.{h,cpp}` (owns stage, layers, wiring) |
| `drag-click-stage.tsx` | `player/StageInputLayer.{h,cpp}` (click/dblclick) |
| `stage-overlays.tsx` (pills/subs) | `player/StageOverlays.{h,cpp}` (hold-speed, fill, sub-drop pills) |
| `loader-layer.tsx`/`cinematic-player-loader.tsx`/`loader-logo-or-text.tsx`/`harbor-loader.tsx` | `player/LoaderLayer.{h,cpp}` (CORE-lite) |
| `transport.tsx` + `player-chrome.ts` (slot config) | `chrome/TransportBar.{h,cpp}` + `chrome/ChromeConfig.h` (`ControlConfig` vector + `controlsInSlot`) |
| `control-renderer.tsx` | `chrome/ControlRenderer` (builds widgets per ControlId) |
| `big-button.tsx` / `tooltip.tsx` | `chrome/BigButton.{h,cpp}` / `chrome/Tooltip.{h,cpp}` |
| `seek-bar.tsx` + `seek-bar-visual.tsx` | `chrome/SeekBar.{h,cpp}` (custom paint + pointer) |
| `seek-step-btn.tsx` | `chrome/SeekStepButton.{h,cpp}` (tap/hold + options popup) |
| `time-display.tsx` + `transport-utils.ts` `fmtTime` | `chrome/TimeLabel.{h,cpp}` + `util/TimeFormat.{h,cpp}` |
| `volume-control.tsx` / `stremio-volume.tsx` + curve | `chrome/VolumeControl.{h,cpp}` + `util/VolumeCurve.{h,cpp}` |
| `speed-menu.tsx` | `chrome/SpeedMenu.{h,cpp}` |
| `audio-menu.tsx` | `chrome/AudioMenu.{h,cpp}` |
| `subtitle-menu.tsx` + `menu-body/variant-row/delay-row` | `chrome/SubtitleMenu.{h,cpp}` (+ rail/variant-row/delay-row) |
| `sub-style.ts` / `sub-style-bar.tsx` / `sub-presets.ts` | `subs/SubStyle.{h,cpp}` (applySubStyle) + `subs/SubStyleBar` + `subs/SubPresets` |
| `use-track-autoload.ts` + `language.ts` | `engine/TrackAutoload.{h,cpp}` (langScore/pickBestTrack) |
| `use-keyboard-shortcuts.ts` + `hotkeys.ts` | `player/HotkeyDispatcher.{h,cpp}` |
| `use-chrome-visibility.ts` | `player/ChromeVisibility.{h,cpp}` (1800/4500 timer + cursor) |
| `use-video-fill.ts` | `player/VideoFill.{h,cpp}` (6 modes + zoom) |
| `use-fullscreen.ts` | folded into `PlayerView` (showFullScreen/showNormal) |
| `player-prefs.ts` / `player-volume.ts` | `engine/PlayerPrefs.{h,cpp}` (QSettings/JSON) |

### 11.2 Suggested CORE build order
1. **`MpvController` + `MpvGlWidget`** — render context (§2), init options (§3.1), loadfile, observed props (§3.3), snapshot emit. Get a video on screen, playing.
2. **`PlaybackClock`** + basic `PlayerView` stage (video fills window, black until first frame).
3. **`StageInputLayer`** — click=play/pause, dblclick=fullscreen; **`HotkeyDispatcher`** (§9.1) — gets full transport control with zero chrome.
4. **`ChromeVisibility`** (auto-hide 1800/4500 + cursor) + chrome overlay container with gradients/fade (§5.2).
5. **`SeekBar`** (§6) + **`TimeLabel`** (§7.1) + **`PlaybackClock` wiring**.
6. **`TransportBar`** slot engine + **`ControlRenderer`** + `BigButton`/`Tooltip`; place back, play-pause, seek-step, fullscreen (§5.3–5.5).
7. **`VolumeControl`** (vertical primary + curve, §7.2) + **`SpeedMenu`** (§7.3).
8. **`AudioMenu`** + **`SubtitleMenu`** + delay rows + `SubStyle.applySubStyle` + **`TrackAutoload`** (§8).
9. **`VideoFill`** (fill/zoom + pill) + **`StageOverlays`** pills (§9.4, §9.6).
10. **`PlayerPrefs`** persistence + **`LoaderLayer`** (CORE-lite) + sub-drop/pause-on-inactive.

CORE source files a Qt engineer should read first: `bridge.ts`, `mpv.ts`, `playback-clock.ts`, `player-chrome.ts`, `transport.tsx`, `control-renderer.tsx`, `seek-bar.tsx`, `volume-control.tsx`, `transport-utils.ts`, `time-display.tsx`, `speed-menu.tsx`, `player.tsx`, `use-chrome-visibility.ts`, `use-keyboard-shortcuts.ts`, `use-playback-controls.ts`, `use-video-fill.ts`, `use-fullscreen.ts`, `hotkeys.ts`, `player-utils.ts`, `mpv.rs`, `mpv_render_mac.rs`/`mpv_render_linux.rs`.

---

## 12. Open Questions / Ambiguities for the Human Lead

1. **Volume widget primary style.** Two slices disagree on which volume widget is "primary": the seekvol slice recommends the **Stremio vertical pop-up** as Harbor's default feel, but the canonical default shell is `Transport` whose default `volumeStyle` is `"slider"` → the **horizontal 120×8 slider**. The default `Transport` bar literally renders the horizontal slider. **Recommendation: ship the horizontal slider as the CORE default (matches the default shell 1:1); add the vertical pop-up with the Stremio skin later.** Confirm.

2. **Playback clock: faithful relay vs 60Hz interpolation.** Harbor's literal behavior is push-only relay (seek bar updates at mpv's `time-pos` cadence, ~1Hz idle). Option (b) — a 60Hz interpolating `QTimer` — is smoother and arguably what users expect, but is a *deviation* from "1:1, no deviations." Which wins: literal fidelity (choppy-ish) or smooth-intent? **Recommendation: 60Hz interpolation snapping to real ticks** (intent-faithful), but flagging since the brief says no deviations.

3. **Buffered bar: dead vs revived.** Harbor's `bufferedSec`/`buffering` are declared but never populated (the buffered fill is inert; loader uses a separate torrent readiness score). Recreate the **dead** behavior (no buffered indicator, 1:1) or the **faithful improvement** (observe `demuxer-cache-time` + `paused-for-cache`)? For local/HTTP first-milestone playback the improvement is low-cost. Confirm.

4. **Subtitle rendering path.** Confirmed: mpv/libass draws subs natively into the GL surface (do NOT set `sub-visibility=no`, do NOT build an HTML/QPainter overlay). The `sub-text`/`sub-start` observed props become unnecessary. Confirm we drop the custom subtitle overlay entirely (keep only `applySubStyle` → `sub-*` properties).

5. **Seek-step button: simple ±10 vs tap/hold-options.** The chrome slice specs a plain seek-back/forward at fixed 10s; the seekvol slice details a richer `SeekStepBtn` (380ms hold → options popup `[5,10,15,30,60,90]`, right-click menu, per-direction persistence). Is the long-press options picker CORE or a phase-2 nicety? **Recommendation: ship plain ±10s buttons for CORE; add the hold-options popup later.** Confirm.

6. **Gold accent sRGB.** The seek-bar fill default is `oklch(0.78 0.13 60)`; Qt has no native OKLCH. Need a precomputed sRGB hex (warm gold ≈ `#E0A23B`-ish) — and confirm whether to wire it to Tankoban's own accent token (the project memory references an OKLCH gold ladder) rather than hardcode Harbor's exact value.

7. **`backdrop-blur` fidelity.** Harbor leans on `backdrop-blur-md`/`-xl` (frosted translucency) across pills, tooltips, menus, volume popup. True backdrop blur is expensive/awkward in Qt Widgets. Acceptable to approximate with a slightly more opaque solid fill, or is real blur (`QGraphicsBlurEffect`/compositor) required for visual parity? **Recommendation: approximate with opacity for CORE; revisit if it looks cheap.**

8. **Loader scope for milestone 1.** The cinematic loader (blurred backdrop, pulsing logo/title, torrent progress bar vs boat spinner, Cancel pill) is detailed but CORE-lite, and its torrent-progress branch (`readinessScore`) only matters for the torrent source. For local-file/direct-HTTP first milestone, is a minimal loading state sufficient (boat-equivalent spinner + cancel), deferring the cinematic backdrop polish? Confirm.

9. **Fullscreen via mpv property vs Qt window.** Qt should drive fullscreen on the top-level window (`showFullScreen`/`showNormal`). Do we *also* mirror mpv's `fullscreen` property (Harbor sets it), or is that redundant/harmful with QOpenGLWidget owning the surface? **Recommendation: drive the Qt window only; do not set mpv `fullscreen`.** Confirm.


---

# Appendix A — Completeness-Critic Gap Findings

_3 adversarial critics (controls / mpv-commands / behaviors) re-read Harbor against the spec to catch deviations._



################ CRITIC LENS: controls ################
I have enough to compile a thorough gap list. Let me verify two remaining items: the `download` gate (spec says mid vs the actual code) and the stremio-volume claim — but stremio-volume is only used in the stremio shell which is EXTRA, so it's not part of the default Transport bar. Let me quickly confirm the VolumeControl is what the default shell renders (it is, per control-renderer.tsx:200) and check the `dvr` button label/icon detail and download-button gate already seen.

I have all I need. The spec's claim that the vertical stremio pop-up is "primary" is wrong for the default `Transport` shell — but that's already flagged in the spec's own Open Questions #1, so I'll note it as confirmed-against-source rather than re-raise. Now the gap report.

# COMPLETENESS CRITIC — Controls & UI Lens

Re-read against source: `transport.tsx`, `player-chrome.ts`, `transport/control-renderer.tsx`, `transport/seek-step-btn.tsx`, `transport/time-display.tsx`, `transport/big-button.tsx`, `transport/tooltip.tsx`, `transport/speed-menu.tsx`, `transport/volume-control.tsx`, `transport/transport-utils.ts`, `transport/episode-nav-btn.tsx`, `transport/cast-button.tsx`, `subtitle-menu.tsx`, `audio-menu.tsx`, `views/player/shell-layer.tsx`.

Verdict: **GAPS FOUND.** The spec is mostly accurate on layout/order, but it gets the seek-step button, several visual states, and multiple icon/strokeWidth/size details **wrong or missing**. Prioritized list below (P1 = user-visible/load-bearing, P2 = visual fidelity, P3 = minor).

---

## P1 — Wrong or missing CORE behavior

**1. [seek-step-btn.tsx:103-111] Seek-step button has the inline second-count badge ON THE BUTTON FACE — spec MISSES it entirely.**
Harbor's seek-back/forward button is **56×56** (`h-14 w-14`) with the icon AND a superimposed `<span>` showing the step count: `absolute font-mono text-[10.5px] font-bold tabular-nums leading-none` rendering `{seconds}` (e.g. "10"). Icon is `RotateCcw`/`RotateCw size={32} strokeWidth={1.8}`. The spec §5.4/§6 describes a "custom-fallback box 56×56 … icon 28" — **wrong icon size (32 not 28), and it never mentions the centered numeric "10" overlay label**, which is a defining visual of this button. This is the most prominent miss in the control row.

**2. [seek-step-btn.tsx — whole file] The seek-step button's hold-to-open options picker is REAL and shipped in the DEFAULT shell, not an optional phase-2 nicety.** The spec's Open-Question #5 recommends "ship plain ±10s buttons for CORE; add hold-options later." But the default `Transport` literally renders `SeekStepBtn` (control-renderer.tsx:244,267), which includes: **380ms hold** → options popover with `SKIP_OPTIONS = [5,10,15,30,60,90]` (:6,:59-63); **right-click (`onContextMenu`)** also opens it (:98-101); **per-direction persistence** to `localStorage["harbor.seek-step.{direction}"]` (:20,:81-86); tooltip text `"{word} {n}s · hold for options"` (:91). If the recreation ships a plain button, it deviates from the actual default control. At minimum the spec must present this as the default behavior, not bury it as "EXTRA / confirm."

**3. [control-renderer.tsx:214] `download` control is gated by `ctx.mid` (width <1300), NOT `ctx.tight`.** Spec §1.4/§5.3 table treats download as a generic EXTRA but the spec body (§5.4 list and the slot table) implies tight-gating like its neighbors. Source: `if (ctx.mid || ctx.isLiveChannel) return null;`. Download disappears much earlier (at <1300px) than other bottom-left controls. Minor since download is EXTRA, but the gate threshold is stated/implied wrong.

**4. [control-renderer.tsx:336] `speed-menu` is hidden when `ctx.compact` (<1000), and the spec says this — but spec §7.3 Qt note and §5.4 are internally inconsistent: it's hidden at COMPACT, while audio/sub buttons hide at TIGHT.** Verify the recreation gates speed at the compact (1000px) breakpoint, not tight. (Spec states it correctly in one place; flagging because it's easy to conflate the three thresholds — `mid<1300 / compact<1000 / tight<600`.)

**5. [control-renderer.tsx:299-301] `audio-menu` hidden when `tight` OR `engine==="html5"`, AND on live channels hidden unless `audioTracks.length >= 2`.** Spec §5.4 captures the live rule but the §8.5 Qt mapping should confirm the html5 gate is dropped (mpv-only app → always show when not tight). OK in spec, low risk — included for completeness.

---

## P2 — Wrong visual tokens / sizes / icons

**6. [control-renderer.tsx:130] Back-button icon strokeWidth = 2.2 — spec §5.4 says `strokeWidth=2.2` ✓ but size: `ChevronLeft size={26}` ✓.** No gap. (Verified correct.)

**7. [big-button.tsx:23] BigButton transition is `transition-[background-color,color,opacity]` and disabled state is `text-white/30` (not `/40`), active is `bg-white/22 … hover:bg-white/30`.** Spec §5.5 says disabled `text-white/30` ✓, active `bg-white/22` ✓. **No active `scale` on BigButton** (only play-pause has `active:scale-95`). Spec is correct here. No gap — verified.

**8. [control-renderer.tsx:251] Play-pause hover is `hover:bg-white/22` and has `active:scale-95`; transition is `transition-[background-color,transform]`.** Spec §5.4 ✓. Icon `strokeWidth={1.5}`, sizes 36/32/28 ✓. Verified correct.

**9. [speed-menu.tsx:59] Speed trigger is `h-11 min-w-11 … gap-1 rounded-full px-2` — spec §7.3 says `h-11 min-w-11 rounded-full px-2 gap-1` ✓.** Accent condition `open || |rate-1|>0.01 || sleepActive` ✓. **GAP: spec §7.3 says inline label class is `text-[11px] font-bold tabular-nums tracking-wider`** — correct (:65,:70). But spec omits that when sleep is active the trigger shows a **Clock icon (`Clock size={11} strokeWidth={2.4}`) + remaining-time `m:ss`** instead of the rate label (:64-68). Sleep is EXTRA, so acceptable to defer, but the spec should note the trigger's inline slot is shared between rate-label and sleep-countdown.

**10. [volume-control.tsx — whole file] The DEFAULT shell's volume control is the HORIZONTAL slider (`VolumeControl`, style `"slider"`), NOT the Stremio vertical pop-up.** Confirmed against source: `DEFAULT_DEFAULT_CONFIG.options.volumeStyle = "slider"` (player-chrome.ts:186), control-renderer.tsx:200-206 renders `<VolumeControl style="slider">`. The spec's §7.2 declares the **vertical pop-up "primary"** and relegates the horizontal slider to "alternate" — this is **backwards for the default shell**. (The spec's own Open-Question #1 flags the conflict and recommends shipping the horizontal slider; treat #1's recommendation as the resolution.) Concrete horizontal-slider facts the spec must lock as CORE: mute button **48×48** (`h-12 w-12`), icons `Volume2`/`VolumeX size={24} strokeWidth={1.75}`; track `relative h-2 … bg-white/15` width **120px**; **no inline pct% readout unless boosting** (`boosting && <span>`, :159), and when shown it's `text-[12px] font-semibold tabular-nums` colored by `boostColor` with `minWidth:36`. Handle **14×14**, white or `boostColor` when boosting, `boxShadow 0 2px 8px rgba(0,0,0,0.6)`. Fill background at v≤1 is solid `rgba(255,255,255,0.92)`; above 1 it's a 3-stop `linear-gradient(to right, white 0%, white {breakpoint}%, #f97316 {breakpoint}%, {boostColor} 100%)` where breakpoint = `(breakPct/filledPct)*100` (:72-77). **Wheel step 0.05 / 0.5 shift** (:56-60), `preventDefault`. The spec's vertical-popup numbers (176×36 track, 64×64 mute, rounded-[20px] popup, gradient `#fde68a@30%…`) belong to the EXTRA Stremio shell, not the default bar.

**11. [audio-menu.tsx:115] Audio button icon is `Languages size={19} strokeWidth={2}`; subtitle button is `Subtitles size={19} strokeWidth={2}`.** Spec §5.4/§8.1/§8.5 says sub icon `size=19 strokeWidth=2` ✓ and audio uses `Languages size=19` ✓. Verified — no gap. **BUT** note both buttons' OPEN state is `bg-white/22 text-white` (audio :112, sub :119) — spec §8.1 says sub open is `bg-white/22` ✓, audio open also `bg-white/22` ✓. Correct.

**12. [subtitle-menu.tsx:124] Sub active-dot is `absolute end-2.5 top-2.5 h-1.5 w-1.5 rounded-full bg-emerald-400` — that's 6×6px, inset 10px (`2.5`=0.625rem=10px).** Spec §8.1 says "6×6px emerald, inset 10px" ✓. Verified.

**13. [episode-nav-btn.tsx:23-34] Episode prev/next buttons (CORE chrome, disabled when no series) — spec mentions they exist but OMITS their exact spec.** They are `h-11`, `rounded-full`; iconOnly → `w-11` else `mx-1 gap-1.5 px-3.5` with a `text-[14px] font-medium` label; icons `ChevronsLeft`/`ChevronsRight size={iconOnly?22:20} strokeWidth={2.2}` with `dir-icon` RTL mirror; disabled → `text-white/25 cursor-not-allowed`, enabled → `text-white/90 hover:bg-white/10`. The `iconOnly` decision is `mid`-driven (control-renderer.tsx:231,272: `condensed→true, full→false, auto→ctx.mid`). Even though episode-nav is EXTRA-conditional, the spec's "prev/next buttons are CORE chrome" claim (§1.4) needs these values or the recreation can't render the disabled-greyed buttons it promises.

---

## P3 — Minor / icon & label nuances

**14. [control-renderer.tsx:288-295] `pick-another` shows `Tv size={22}` icon (label "TV Guide") on live channels, else `Replace size={22}` (label "Switch stream").** Spec §5.3 labels it "Switch stream" only. EXTRA, low risk, but the dual-icon/dual-label is a detail.

**15. [cast-button.tsx:12-25] Cast button is **disabled** when `!(airplay||chromecast)` with tooltip "Casting comes with the mpv backend" / enabled tooltip "Cast to a device".** Spec lists cast as EXTRA ✓; no action needed, but note it renders as a disabled BigButton in the default bar rather than being absent (it's only fully removed at `tight`, :366).

**16. [tooltip.tsx:17] Tooltip exact classes confirmed: `rounded-lg border border-white/10 bg-black/90 px-2.5 py-1 text-[12px] font-medium text-white … shadow-[0_10px_30px_-12px_rgba(0,0,0,0.7)] backdrop-blur-md`, 150ms, 8px offset (`calc(100%+8px)`), `align end` → `end-0`.** Spec §5.5 matches exactly. Verified — no gap.

**17. [time-display.tsx:45,72] TimeStart `text-white/85`, TimeEnd `text-white/65`, both `font-mono text-[13px] tabular-nums drop-shadow-[0_1px_3px_rgba(0,0,0,0.7)]`.** Spec §7.1 matches ✓. Note `time-end` reads `timeFormat` and renders nothing for `elapsed-only` (:67) ✓. Verified.

**18. [transport.tsx:208] Chrome config also live-reacts to a `storage` event keyed `"harbor.player.chrome.profiles.v1"` in addition to the custom event.** Spec §5.6 calls the live-update events EXTRA and says hardcode the default — fine. No gap, noting for completeness.

**19. [transport.tsx:284] `onTitleClick` opens the **CastModal** (`setCastModalOpen(true)`), not an info panel.** Spec §5.4 says title-info trailing `Info` icon and "clickable (has meta, not live)" but doesn't say what the click does — in the default shell it opens the cast/details modal (transport.tsx:284,390-397). EXTRA target, but the spec implies an info action; the actual wiring is the CastModal.

---

## Summary of the most important fixes the spec needs

1. **Seek-step button: 56×56, icon `RotateCcw/CW size=32`, with a centered `{seconds}` numeric badge** (`text-[10.5px] font-mono font-bold`) — and the 380ms-hold / right-click options picker (`[5,10,15,30,60,90]`, per-direction localStorage) IS the default behavior, not a deferred extra. (P1)
2. **Default volume control is the HORIZONTAL 120×8 slider with 48×48 mute (icons size 24 sw 1.75) and boost-only pct readout** — not the vertical Stremio pop-up. (P1/P2)
3. **Download gate is `mid` (<1300px), not tight.** (P2)
4. **Episode prev/next button exact spec** (h-11, ChevronsLeft/Right size 20/22 sw 2.2, label text-[14px], disabled white/25) is missing. (P2)
5. **Speed trigger's inline slot doubles as a sleep-countdown (Clock + m:ss)**; pick-another has Tv-vs-Replace dual icon; cast renders disabled (not absent) in default bar. (P3)

Everything else in the spec's control section (slot order, the `1fr_auto_1fr`/`auto_1fr_auto` grid, gradients, padding `px-7 pt-10 pb-5`/`px-3 pt-6 pb-3`, play-pause sizes 64/56/48, BigButton 48, tooltip tokens, time-label colors, fmtTime, breakpoints mid/compact/tight = 1300/1000/600) **matches source and is verified correct.**


################ CRITIC LENS: mpv ################
I've now read all three files in full. Let me cross-check the spec against the exact code. I have enough to produce the gap report.

# COMPLETENESS CRITIC — LENS: MPV COMMANDS, OPTIONS & PROPERTIES

I re-read `mpv.rs`, `mpv.ts`, and `bridge.ts` in full and compared every option/command/property/bridge-method against the spec. **Gaps found.** Prioritized below.

---

## P0 — WRONG VALUE / WILL BREAK 1:1 BEHAVIOR

### G1. `vo` for the render-API path is WRONG. Spec says `vo = libmpv`; the fresh-start (non-embed) path Harbor actually uses is `vo = "gpu-next,"`.
- `[mpv.rs:307-317]` →
  ```rust
  let use_render_api = (cfg!(macos) || cfg!(linux)) && want_embed;
  if !use_render_api {
      mpv.set_property("vo", "gpu-next,")   // <-- non-embed default
  } else {
      mpv.set_property("vo", "libmpv");
      mpv.set_property("force-window", "no");
  }
  ```
- **Spec status: WRONG.** The spec (§3.1 "Render-API path") states `vo = libmpv` for "all platforms" and cites `:313`. But `:313` is only reached when `use_render_api` is true (mac/linux + embed). The Tankoban analogue of "we own the GL surface and use the render API" is structurally the `use_render_api` branch → `vo=libmpv` is correct **for Tankoban's render-context path**, so the spec's *conclusion* (`vo=libmpv`) is right for our case. **However the spec fails to record that Harbor's default `vo` value is `gpu-next,` (with the trailing comma — a deliberate fallback-chain syntax).** A Qt engineer reading the spec will never see `gpu-next,` and won't know Harbor's actual primary VO. Note both: render-context path → `vo=libmpv`; Harbor's own non-render path → `vo=gpu-next,`. Minor for our build, but it's a factual omission the spec presents as settled.

### G2. PRE-INIT `force-window` matrix is mis-stated.
- `[mpv.rs:177-187]`: Linux non-embed → `force-window=yes`; Windows/other → `force-window=immediate`; mac-embed → `force-window=no`.
- `[mpv.rs:316]`: render-API path additionally sets `force-window=no` *post-init*.
- **Spec status: PARTIAL/WRONG.** Spec §3.1 says under "Render-API path (Tankoban — all platforms): `force-window = no` (`mpv.rs:316`)" and separately lists `force-window=immediate` as "webview hack only, do NOT set." Net guidance (`force-window=no` for our render path) is correct, but the spec never states the **pre-init** value is set to `immediate`/`yes`/`no` per-OS *before* init and then overridden — and it mislabels `immediate` as purely a webview hack when it's actually the Windows/default **pre-init** value for the non-render path. For our render-context build, `force-window=no` is right; flag that the pre-init/post-init interaction exists.

### G3. `mute` write format mismatch — bridge sends a **JSON boolean**, not `yes`/`no` string.
- `[mpv.ts:372-374]` `setMuted(m)` → `invoke("mpv_set_property", { name: "mute", value: m })` where `m` is a JS `boolean`.
- `[mpv.rs:565-573]` `value_to_arg`: `Value::Bool(b) => "yes"/"no"`. So the boolean is coerced to `"yes"`/`"no"` **in Rust**, not JS.
- **Spec status: OK but under-documented.** Spec §4.2 says `setMuted(m) → mute → bool`. The load-bearing detail the spec omits: **all set_property values cross as JSON and are stringified by `value_to_arg` (`yes/no` for bool, `n.to_string()` for number, `""` for null).** In a collapsed Qt class there's no JSON hop, so the engineer must replicate this coercion *manually* before `mpv_set_property_string`. The spec mentions arg coercion only for **commands** (§3.2), not for **set_property**. Add: `set_property` values use the same coercion (bool→`yes`/`no`, number→decimal string, null→`""`).

---

## P1 — OMITTED COMMANDS / PROPERTIES

### G4. `["af", "set", <chain>]` audio-filter command — spec marks EXTRA but the **`dynaudnorm` normalize chain is CORE-adjacent** and the exact filter string is omitted.
- `[mpv.ts:68]` normalize filter = `"dynaudnorm=f=200:g=15:m=10:r=0.6"`.
- `[mpv.ts:55-60]` `AUDIO_PROFILE_AF` map (bass/voice/bass-reduce/night) with exact `lavfi=[...]` strings.
- `[mpv.ts:66-71]` `applyAudioFilters()` joins `[normalize?, profile?]` with `","` and sends `["af","set", parts.join(",")]`.
- **Spec status: OMITTED VALUE.** Spec §3.2 lists `["af","set",<filters>]` as "(normalize/profile — EXTRA)" with no string. Even if deferred, the **exact** filter strings should be captured verbatim now (they're load-bearing constants that would otherwise need re-derivation). At minimum record `dynaudnorm=f=200:g=15:m=10:r=0.6`.

### G5. `ab-loop-a` / `ab-loop-b` set_property values — spec marks EXTRA but omits the exact null-sentinel.
- `[mpv.ts:455-458]` `setAbLoop(a,b)`: `ab-loop-a = a==null ? "no" : a`; `ab-loop-b = b==null ? "no" : b`.
- **Spec status: OMITTED (acceptable as EXTRA).** The `"no"` sentinel for clearing the loop is the only non-obvious bit; fine to defer, noting the value.

### G6. `force-media-title` (setMediaInfo) is listed but the spec doesn't flag it as **CORE for the title chrome**.
- `[mpv.ts:444-446]` `setMediaInfo(info) → force-media-title = info.title`.
- **Spec status: PRESENT but mis-tiered.** Spec §4.2 maps it; §4.1 bolds `setMediaInfo` is NOT bolded (left non-CORE). But the title-info chrome control (§5.4) displays a media title — if Tankoban shows the file's title via mpv, `force-media-title` is the write path. Minor; the chrome title may be app-supplied instead. Flag the coupling.

### G7. Screenshot property triad omitted (EXTRA, but values are load-bearing).
- `[mpv.rs:718-721]` PNG path: `screenshot-format=png`, `screenshot-png-compression=3`, `screenshot-sw=yes`, then `["screenshot-to-file", path, "video"]`.
- `[mpv.rs:921-924]` JPEG data-url path: `screenshot-format=jpg`, `screenshot-jpeg-quality=72`, `screenshot-sw=yes`.
- `[mpv.rs:784-786]` GIF path: `screenshot-format=jpg`, `screenshot-jpeg-quality=92`, `screenshot-sw=yes`.
- **Spec status: OMITTED (acceptable as EXTRA)** — but note the `"video"` 2nd-arg of `screenshot-to-file` (captures video-only, no OSD/subs) and `screenshot-sw=yes` (software render — required because the render-API/libmpv VO can't do GPU screenshots). The spec's §3.2 row gives the command but not these prerequisite properties.

---

## P2 — BRIDGE-MODEL / EVENT-MAPPING GAPS

### G8. Event payload includes `playback-restart` and `seek` events that the spec never lists.
- `[mpv.rs:515-516]` emits `{event:"playback-restart"}` and `{event:"seek"}`.
- `[mpv.ts:41]` `MpvEvent` type includes `playback-restart`; `handleEvent` does **not** branch on it (no-op) — and `seek` falls through to the generic `else` (no-op).
- **Spec status: OMITTED but harmless.** Harbor ignores both. Spec §3.3 "Event-pump non-property mapping" covers only `end-file` + `file-loaded`. Add a line: `playback-restart`, `seek`, `shutdown`, `log` events are emitted but **not consumed** by the bridge (no-ops) — so Qt can skip them. Worth stating so the engineer doesn't hunt for missing handlers. (`Event::Shutdown` does break the event loop in Rust `[mpv.rs:455-456,476]`, and `[mpv.ts]` has no shutdown handler — the JS bridge never sees a shutdown-driven state change.)

### G9. `end-file` reason decoding — spec's reason set is slightly off and omits the **integer→string mapping** plus that the JS lowercases.
- `[mpv.rs:503-512]` integer→string map: `0=eof, 2=stop, 3=quit, 4=error, 5=redirect, _=other`. **Note: `1` is NOT mapped (no "restart"/"watch-later"); reason `1` falls to `"other"`.**
- `[mpv.ts:175-181]` lowercases reason; `stop|quit|redirect → return` (ignored); within `suppressEndFileUntil` → ignored; `reason && reason!=="eof"` → `status=error, errorCode="decode", errorMessage=\`mpv ended playback: ${reason}\``; else `status=ended`.
- **Spec status: MOSTLY OK, one error.** Spec §3.3 lists reasons as `EOF/STOP/QUIT/ERROR/REDIRECT` (uppercase) and says other→`errorCode="decode"`. Correct on errorCode. But: (a) Harbor emits **lowercase** strings (`"eof"`, `"stop"`, …) — the Qt comparison must be lowercase; (b) the spec's set omits `"other"` (the catch-all for unmapped reasons incl. integer `1`); (c) `errorMessage` exact format `"mpv ended playback: {reason}"` is omitted. These are load-bearing for faithful error UX.

### G10. `noAudio` snapshot field exists but is **never written** — spec should mark it dead, like `buffering`.
- `[bridge.ts:46]` `noAudio?: boolean` declared.
- Grep of `mpv.ts`: **never assigned.** (Same dead-field class as `bufferedSec`/`buffering`.)
- **Spec status: PARTIAL.** Spec §4.3 includes `bool noAudio;` in the struct but doesn't flag it as never-populated by the mpv bridge. Mark it dead (default false, never set by mpv path) for parity with the buffering note.

### G11. `chapter-list` parse reads `c.time` (NOT `startSec`) and the spec's field source is slightly imprecise.
- `[mpv.ts:163-172]` chapters: `title = c.title (string else "")`, `startSec = c.time (number else 0)`, filter finite & `>=0`, sort ascending by startSec.
- **Spec status: OK** — §8.8 says `{title, startSec:time}`. Correct. (Verifying, not a gap. The mpv node key is `time`, mapped to `startSec`.)

### G12. `setVolume` rounding and the `volume/100` read direction are correct, but the **`volume-max=600` is the gate that makes >100 possible** — spec has it, ✓. However spec omits that `volume` is observed as `Double` and read **raw mpv units** then `/100`; if `volume-max` is NOT set, mpv silently clamps writes at 100 and the snapshot read-back caps at 1.0. Already covered in §3.1 ("required, else mpv clamps"). ✓ No gap.

---

## P3 — SMALL OMISSIONS / VERIFICATIONS

### G13. `msg-level` — spec correctly drops d3d11/win32 (`all=warn,vo=v,gpu=v`). Harbor's actual value `[mpv.rs:171]` is `all=warn,vo=v,d3d11=v,gpu=v,win32=v`. Spec's pruning is intentional and correct. ✓ No gap (verifying).

### G14. `mpv_request_log_messages(mpv, "warn")` `[mpv.rs:303-306]` is called post-init in addition to `terminal=no`. Cosmetic (drives the `mpv://log` channel which the bridge no-ops at `[mpv.ts:238]`). Spec omits it; **acceptable to drop** — record one line that log messages are requested at "warn" but consumed nowhere.

### G15. Initial-subs flag differs between boot paths — spec captured it but verify exact flags:
- Fresh boot `[mpv.rs:412]`: `["sub-add", url, "auto"]` (no title, no select → flag `auto`).
- Reuse path `[mpv.ts:261-266]`: `mpv_sub_add(url, lang, title=null, select=false)` → `[mpv.rs:978]` flag = `select.unwrap_or(true)==false → "auto"`.
- `addSubtitle` default `[mpv.ts:427]`: `select ?? true` → flag `select`.
- **Spec status: OK.** §3.2/§8.8 capture this. (Verifying — initial subs always `auto`, user-added default `select`.)

### G16. `sub-add` 4-arg argv with optional 5th `lang` — spec §3.2 row shows `["sub-add", url, flag, title, [lang]]`. Harbor's title-fallback logic `[mpv.rs:981-987]` (title → lang → `"Subtitle"`) is omitted.
- **Spec status: PARTIAL.** The mpv `sub-add` *title* slot is never empty in Harbor: if no title and no lang, it sends literal `"Subtitle"`. Qt must replicate or external tracks show a blank label. Minor but real.

---

## SUMMARY (prioritized)

| # | Gap | Severity | Fix |
|---|---|---|---|
| G1 | Harbor's default `vo=gpu-next,` never recorded (only `libmpv` shown) | P0-doc | Record both VO values + which path each belongs to |
| G2 | pre-init `force-window` per-OS matrix (`immediate`/`yes`/`no`) mis-stated as pure webview hack | P0-doc | State pre-init value + post-init `no` override |
| G3 | `set_property` value coercion (bool→`yes/no`, num→str, null→`""`) only documented for commands | P0 | Add to §4.2; collapsed Qt class must coerce manually |
| G4 | `dynaudnorm` + 4 profile `af` filter strings omitted | P1 | Capture verbatim (esp. `dynaudnorm=f=200:g=15:m=10:r=0.6`) |
| G5 | `ab-loop-a/b` `"no"` sentinel omitted | P1(EXTRA) | Note value when built |
| G7 | screenshot property triad (`screenshot-sw=yes`, format, quality) + `"video"` arg | P1(EXTRA) | Capture; `screenshot-sw=yes` mandatory for libmpv VO |
| G8 | `playback-restart`/`seek`/`shutdown`/`log` events emitted but un-consumed | P2 | One line: Qt may ignore |
| G9 | `end-file` reasons are lowercase, include `"other"`, exact errorMessage format | P2 | Fix reason set + message string |
| G10 | `noAudio` field never written (dead, like `buffering`) | P2 | Mark dead |
| G16 | `sub-add` title fallback (`title→lang→"Subtitle"`) omitted | P3 | Add fallback |
| G14 | `mpv_request_log_messages("warn")` post-init, consumed nowhere | P3 | One line |

**No mis-named observed properties** — all 15 in `OBSERVED_PROPS [mpv.rs:86-102]` match the spec's table exactly (ids 1–15, formats correct). **No mis-named control properties** — every `set_property` name in `mpv.ts` (`pause/volume/mute/speed/aid/sid/sub-visibility/sub-delay/audio-delay/panscan/video-zoom/video-aspect-override/force-media-title/ab-loop-a/ab-loop-b/fullscreen`) matches §4.2. The spec's **observed-property table and control-verb table are accurate**; the gaps are in (a) init-option VO/force-window values, (b) the cross-process value-coercion contract, (c) omitted exact filter/screenshot constants, and (d) event-reason fidelity.


################ CRITIC LENS: behaviors ################
I now have full ground truth. Comparing against the spec adversarially.

# COMPLETENESS CRITIC — Behaviors / Hotkeys / Track-Autoload

I re-read all six assigned hooks plus `hotkeys.ts`, `player-utils.ts`, and `use-playback-controls.ts`. The spec's hotkey table and autohide constants are mostly correct, but there are several **real gaps and one outright error**. Prioritized below.

---

## HIGH severity

### G1 — Hold-to-speed magnitude is WRONG (`max(2, baseRate)`, not `max(2, base)` floor-as-described — but the spec's prose then mislabels it)
[`use-keyboard-shortcuts.ts:115`] -> Harbor: `bridgeRef.current?.setRate(Math.max(2, h.baseRate))` where `h.baseRate = snap.rate` captured at keydown (`:110`).
-> **Spec status: PARTIALLY WRONG.** §9.1a says "record `baseRate = snap.rate` … `setRate(max(2, baseRate))` (min 2.0×)" — that part is right. BUT the hotkey TABLE row for Space (§9.1) says "hold 350ms → speed (§9.1a)" and §9.1a's Qt note says "release decides tap-vs-hold by whether the timer fired" — correct. The genuinely missing detail: **`baseRate` is `snap.rate` at the moment of keydown, so if the user is already at 3× the hold gives `max(2,3)=3×` (no change)**, and on release it restores to that captured 3×, NOT to 1×. The spec never states the restore target is the *captured* rate (it says "restore `baseRate`" but doesn't flag that baseRate can be ≠1). Minor, but load-bearing for someone already at non-1× speed. Acceptable as-is; flag for the engineer.

### G2 — `e.repeat` guard is ONLY on Space, and the spec generalizes it incorrectly
[`use-keyboard-shortcuts.ts:107`] -> Harbor: `if (e.repeat) return;` appears **only inside the `playerPlayPause` branch**, AFTER `e.preventDefault()` and AFTER recording `h.key`/`h.baseRate` would-be… actually it returns before starting the timer. No other key checks `e.repeat`.
-> **Spec status: CORRECT but under-specified.** The table's Space row says "ignore `e.repeat`" — good. But note the ordering: `preventDefault()` runs, THEN `if(e.repeat) return`. So a held Space still suppresses the browser default on every repeat but only arms the timer once. The Qt engineer using `keyPressEvent` auto-repeat must replicate: on auto-repeat Space, still accept/consume the event but do NOT re-arm the 350ms timer. Not in spec. Add it.

---

## MEDIUM severity

### G3 — Volume-up has a redundant inner clamp the spec drops; volume-down does NOT have an upper clamp
[`use-keyboard-shortcuts.ts:142`] -> Harbor volume-UP: `Math.min(6, Math.max(0, snap.volume + step))` (clamps BOTH ends).
[`use-keyboard-shortcuts.ts:150`] -> Harbor volume-DOWN: `Math.max(0, snap.volume - step)` (floor 0 only, **no `min(6)`**).
-> **Spec status: CORRECT** (§9.1 table says up "clamp [0,6]", down "floor 0"). Verified accurate. No gap. (Listed for completeness so the engineer doesn't "helpfully" add a `min(6)` to the down path and call it parity.)

### G4 — Volume keys persist via `writePlayerVolume({volume})` — GLOBAL store, not per-show; spec is right but the asymmetry with speed/sub-delay is worth flagging
[`use-keyboard-shortcuts.ts:144,152`] -> Harbor: volume → `writePlayerVolume({ volume: next })` (global `harbor.player.volume.v1`).
[`:219,226`] speed → `writePlayerPrefs(metaId, {rate})` (per-show). [`:234,242`] sub-delay → `writePlayerPrefs(metaId, {subDelaySec})` (per-show).
-> **Spec status: CORRECT** (§9.7 captures both stores). No gap — verified the routing matches.

### G5 — Mute hotkey does NOT persist, and toggles raw `!snap.muted`
[`use-keyboard-shortcuts.ts:157`] -> Harbor: `bridgeRef.current?.setMuted(!snap.muted)` — **no `writePlayerVolume` call** (unlike volume up/down).
-> **Spec status: GAP.** §9.1 table row `m` says "Mute toggle | `setMuted(!muted)`" — correct on the action, but the spec's §9.7 says the volume store is `{volume, muted}` and implies mute persists. **Mute via hotkey is NOT persisted to disk in Harbor** (only the volume slider/mute-button path may write `muted`; the keyboard `m` does not). The Qt engineer should NOT write the mute state to QSettings on the `m` key. Add this exclusion.

### G6 — Speed-up/down clamp is `[0.25, 3]` and uses `.toFixed(2)` rounding — spec says "[0.25,3]" but omits the rounding
[`use-keyboard-shortcuts.ts:217`] -> `Math.max(0.25, +(snap.rate - 0.25).toFixed(2))`.
[`:224`] -> `Math.min(3, +(snap.rate + 0.25).toFixed(2))`.
-> **Spec status: MINOR GAP.** §9.1 table says "speed ±0.25 [0.25,3]" — correct bounds. But the `+(x).toFixed(2)` round-to-2-decimals is what keeps `1.25 - 0.25 = 1.00` clean (float drift). The Qt engineer must round to 2 decimals (`std::round(r*100)/100`) on each step or accumulate drift. Not stated. Add it. (Note this is DIFFERENT from the §7.3 speed-MENU which has discrete fixed values [0.5…2]; the `[`/`]` hotkeys go up to **3×**, beyond the menu's 2× ceiling — the spec DOES note "speed menu … no >2", good, but doesn't explicitly contrast that hotkeys reach 3×. Worth an explicit callout.)

### G7 — Sub-delay step is `0.1` default / `0.05` Shift — spec's table has these REVERSED-looking but is actually correct; the `round2` helper is `Math.round(v*100)/100`
[`use-keyboard-shortcuts.ts:231,239`] -> `const step = e.shiftKey ? 0.05 : 0.1;`
[`player-utils.ts:24-26`] -> `round2(v) = Math.round(v*100)/100`.
-> **Spec status: CORRECT** (§9.1: "z Sub delay −0.1s (Shift −0.05)"). Verified. The `round2` = round to 2 decimals is captured. No gap.

### G8 — `End` key target is `durationSec - 0.5`, gated on `durationSec > 0`
[`use-keyboard-shortcuts.ts:207`] -> `if (snap.durationSec > 0) seekTo(snap.durationSec - 0.5)`.
-> **Spec status: CORRECT** (§9.1: "End … if dur>0: seekTo(dur-0.5)"). Verified.

---

## MEDIUM — TRACK AUTOLOAD gaps (the spec's §8.8 has real omissions)

### G9 — Spec OMITS `preferEmbeddedSubs` from the auto-SELECT decision (network/online-search path)
[`use-track-autoload.ts:149-151`] -> Harbor: `embeddedPreferred = settings.preferEmbeddedSubs && snapRef.current.subtitleTracks.some(t => !t.external)`; and `shouldSelect = … && !embeddedPreferred && …` (`:158-163`).
-> **Spec status: GAP.** §8.8 lists `subsOff`, `blocked`, `nativeForced`, `firstAdded` as the select gates but **never mentions `preferEmbeddedSubs`** — when the file already has an embedded sub track and the user prefers embedded, an externally-downloaded sub must NOT auto-select. This entire `shouldSelect` 4-condition gate (`!subsOff && !blocked && !embeddedPreferred && !nativeForced && !firstAdded`) is part of the ONLINE-SEARCH autoload path (EXTRA), so it's arguably out of CORE scope — BUT the spec's §8.8 presents the auto-select rules as if they're the embedded-track rules, conflating the two effects. The CORE embedded-track autoload is the SECOND effect (`:228-253`); the spec's bullet list mixes conditions from both. Clarify which path each gate belongs to.

### G10 — Spec's autoload omits the `firstAdded` / `PER_LANG_MAX=6` cap and "only the FIRST added sub auto-selects" rule
[`use-track-autoload.ts:137,142-145,163,167`] -> `PER_LANG_MAX = 6`; `firstAdded` flips true after the first accepted add, and `shouldSelect` requires `!firstAdded` -> **only one** downloaded sub is ever auto-selected per session.
-> **Spec status: GAP (but EXTRA-path).** This belongs to the online-search downloader (EXTRA). Not CORE. Noted so the engineer knows the §8.8 "auto-select" prose is describing two systems. Defer.

### G11 — CORE embedded-track audio autoload: spec says "if better than current → setAudioTrack" but omits the exact better-test
[`use-track-autoload.ts:229-233`] -> Harbor: `want = pickBestTrack(allow(audioTracks), audioLangs); cur = audioTracks.find(t=>t.selected) ?? null; effAudio = want ?? cur; if (want && (!cur || cur.id !== want.id)) setAudioTrack(want.id)`.
-> **Spec status: MINOR GAP.** Spec §8.8 says "if better than current → setAudioTrack" — but the real condition is **`want exists AND (no current selection OR current.id ≠ want.id)`** — it switches whenever the picked track differs by ID, it does NOT compare scores against the current. Also `effAudio = want ?? cur` is what feeds the `nativeAudio` forced-sub decision downstream (`:237-240`). The spec's phrasing "better than current" is imprecise — it's "different id than current." Correct it.

### G12 — `subsOffFor` precedence is exact and the spec's `subsOff` rule is slightly off
[`use-track-autoload.ts:282-287`] -> Harbor: `if (prefs?.subsOff != null) return prefs.subsOff;` (per-show override wins, including explicit `false`), THEN `if (s.subtitlesOffByDefault) return true;`, THEN `if (prefs?.subLang) return false;`, else `false`.
-> **Spec status: GAP.** §8.8 says "`subsOff` (prefs.subsOff || settings.subtitlesOffByDefault)". That's WRONG precedence — it's not an OR. **Per-show `subsOff` (if not null) overrides the global default entirely**, even to force subs ON (`prefs.subsOff === false` beats `subtitlesOffByDefault === true`). The spec's `||` would incorrectly force-off whenever the global default is on, ignoring a per-show "on" override. Fix to the 4-step precedence.

### G13 — `nativeAudio` forced-sub branch sorting is omitted
[`use-track-autoload.ts:244-248`] -> Harbor: when `nativeAudio`, pick forced subs: `snap.subtitleTracks.filter(isForcedTrack).sort((a,b)=>langScore(b.lang,subLangs)-langScore(a.lang,subLangs))[0]` and `setSubtitleTrack(forced.id)`.
-> **Spec status: MINOR GAP.** §8.8 says "pick best **forced** sub" — but the exact rule is: filter by `isForcedTrack` (regex `/\bforced\b/i` on title+label, `:278-280`), then sort **descending by langScore against subLangs**, take `[0]`. It does NOT use `pickBestTrack` here (which would skip forced tracks — `pickBestTrack` explicitly skips forced per the spec's own §8.8). Different selection function. Spell out the regex + sort.

### G14 — `nativeAudio` condition also requires `effAudio != null`, and uses `effAudio.lang`, not "chosen audio lang"
[`use-track-autoload.ts:237-240`] -> `nativeAudio = settings.forcedSubsWhenNativeAudio && effAudio != null && langScore(effAudio.lang ?? "", subLangs) >= 0`.
-> **Spec status: CORRECT-ish.** §8.8 says "if `forcedSubsWhenNativeAudio` && chosen audio lang matches a preferred sub lang." Accurate in spirit. The `effAudio = want ?? cur` detail (G11) is what "chosen audio" resolves to — make sure the engineer threads that variable through. No new gap beyond G11.

### G15 — Per-show prefs apply-once guard uses `prefsAppliedRef` keyed by `src.meta.id` — spec says "applied once on first track-load," correct, but omits the rate/delay ≠ guard
[`use-track-autoload.ts:255-263`] -> Harbor only writes rate if `prefs.rate !== snap.rate` and sub-delay if `prefs.subDelaySec !== snap.subDelaySec`, AND only when `prefsAppliedRef.current !== src.meta.id` (once per show).
-> **Spec status: CORRECT** (§8.8 "Per-show prefs applied once … prefs.rate→setRate, prefs.subDelaySec→setSubDelay"). The `!==` equality guards are minor but prevent redundant writes; mention them. Sub-style on track-load (`if engine==="mpv" applySubStyle`, `:197`) also correct.

### G16 — Block-words helper details omitted (CORE)
[`use-track-autoload.ts:221-226, 269-276`] -> `allow(tracks)`: `words = trackBlockWords.map(trim().toLowerCase()).filter(Boolean)`; if empty return tracks; filter out tracks where `\`${title} ${label}\`.toLowerCase().includes(word)`; **if that empties the list, return the ORIGINAL** (`kept.length > 0 ? kept : tracks`).
-> **Spec status: CORRECT** (§8.8 captures the "if empties, keep original" fallback). Verified. Applies to BOTH audio and subtitle picks (spec implies subs only by placement — clarify it wraps `allow(audioTracks)` too, `:230`).

---

## LOW severity / verified-correct (no gap, listed to prove coverage)

- **Seek hotkeys**: ±10 (Arrow L/R), ±30 (`,`/`.`) -> `seekStep(±N)` -> `seek(max(0, pos+delta))` absolute. [`use-keyboard-shortcuts.ts:119-138`, `use-playback-controls.ts:85-97`] -> **Spec CORRECT.** Note: `seekStep` reads `getPlaybackPosition()` (the clock), NOT `snap.positionSec` — spec §3.2 says "read from the live clock" — correct.
- **Digit seek**: `0`→`seekTo(0)` (hardcoded, NOT via registry — `:280`); `1-9`→`seekTo(dur*digit/10)` if `dur>0`. [`:285-292`] -> **Spec CORRECT.** Note `0` bypasses the hotkey-override system entirely (always `0`), as do `1-9` — these are NOT remappable. Spec doesn't say they're non-overridable; minor.
- **Escape**: drawMode→exit draw first, else `closePlayer()`. [`:100-104`] -> **Spec CORRECT.**
- **Subtitle cycle**: `s` AND `c` both bound (`playerSubtitleCycle` + `…Alt`, `hotkeys.ts:71-72`); order OFF→sub0→…→last→OFF. [`use-playback-controls.ts:45-63`] -> **Spec CORRECT.** Confirmed `idx===-1`(off)→`subs[0]`; else `next=idx+1`; `next>=length`→`null`(off). Each persists via `rememberSubChoice` writing `{subLang, subsOff:false}` or `{subsOff:true}`.
- **Media keys**: `MediaPlayPause`→toggle (always); `MediaTrackNext/Previous`→ep change **gated on `hasNextEp`/`hasPrevEp`**. [`:84-98`] -> **Spec CORRECT** (these bypass the override registry — confirmed, checked via raw `e.key`).
- **`n`/`b` episode, `w/e/g/r/l/h/p/o` extras** -> all gated on their optional callbacks. **Spec CORRECT** (one-line EXTRA noted).
- **Typing-target gate**: INPUT/TEXTAREA/SELECT/contentEditable/role=textbox|searchbox|combobox. [`hotkeys.ts:111-119`] -> **Spec CORRECT.**
- **Letter-key Shift-agnostic matching**: `eventToBinding` only adds `shift` to the binding string when **`!isLetter`** (`hotkeys.ts:99`), so `m`/`s`/`f` match with or without Shift; but `z`/`x` letter keys ALSO match Shift-agnostically AND read `e.shiftKey` inside the handler for fine-step — both true simultaneously (Shift+Z matches `z` binding AND triggers the 0.05 fine step). [`:99,231`] -> **Spec CORRECT** (table notes "Shift NOT in binding string for letters"). Good catch already present.
- **Chrome autohide constants**: `CHROME_HIDE_MS_PLAYING=1800`, `CHROME_HIDE_MS_PAUSED=4500`. [`player-utils.ts:21-22`] -> **Spec CORRECT (exact).**
- **`wakeChrome` re-arm gate**: skip re-arming hide timer if `anyMenuOpenRef.current || getSeekHovering()`; wait = `playing && !drawMode ? 1800 : 4500`. [`use-chrome-visibility.ts:21-31`] -> **Spec CORRECT.**
- **Cursor hide**: `drawMode || (!chromeVisible && playing)` → `cursor:none`. [`:93-97`] -> **Spec CORRECT.**
- **Blur hides chrome immediately**; `mouseout` with no `relatedTarget` hides only if `playing && !drawMode && !menu && !seekHovering`. [`:46-67`] -> **Spec CORRECT.**
- **Video-fill 6 modes / order / panscan / aspect / zoom**: `fit/fill/zoom/16:9/4:3/2.39:1(Original)`; `apply` sets panscan+aspect+`video-zoom = (id==="zoom"?zoomLevel:0)`. [`use-video-fill.ts:16-23,43-57`] -> **Spec CORRECT (exact).** Pill auto-dismiss 1200ms (`:40`) ✓. Zoom clamp [0,1] step round-to-2 (`:87`) ✓. Pill text `Zoom {round(2^zoom*100)}%` (`:53`) ✓. `cycle` leaving zoom resets `zoom.current=0` (`:76`) ✓. `step` forces mode→zoom (`:82-86`) ✓.

---

## ONE NOTE on §9.4 video-fill labels (verify-correct, but spec adds detail Harbor lacks)
[`use-video-fill.ts:17-22`] -> Harbor mode labels are literally `"Fit"`, `"Fill"`, `"Zoom"`, `"16:9"`, `"4:3"`, `"2.39:1 (Original)"`.
-> **Spec status: CORRECT.** The spec's §9.4 pill-label column matches exactly. No gap. (The "zoom" mode's `aspect` is `"-1"` not a real ratio — spec's table correctly shows `-1` for idx 2.)

---

## SUMMARY — actionable gaps to fix in the spec

| # | Severity | Gap |
|---|---|---|
| **G12** | **HIGH** | `subsOff` precedence is NOT `prefs.subsOff \|\| settings.subtitlesOffByDefault` — per-show `subsOff` (incl. explicit `false`) overrides global. Fix to 4-step precedence (`use-track-autoload.ts:282-287`). |
| **G5** | MED | Mute hotkey (`m`) does NOT persist to disk; spec's `{volume,muted}` store implies it does. Exclude `m` from QSettings write. |
| **G11** | MED | CORE audio autoload switches when picked track **id ≠ current id**, not "better than current" (no score compare vs current). |
| **G13** | MED | `nativeAudio` forced-sub pick = `filter(isForcedTrack).sort(desc langScore)[0]`, NOT `pickBestTrack`. Regex `/\bforced\b/i` on title+label. |
| **G2** | MED | `e.repeat` guard is Space-only and runs AFTER `preventDefault()`; Qt auto-repeat must consume but not re-arm the 350ms timer. |
| **G9** | MED | Auto-select gate omits `preferEmbeddedSubs` (don't auto-select external sub when embedded preferred + present). (online-search EXTRA path — clarify scope.) |
| **G6** | LOW | `[`/`]` speed reaches **3×** (beyond menu's 2× ceiling) and must round-to-2-decimals per step. |
| **G16** | LOW | `allow()` block-words wraps BOTH audio and subtitle picks; "if filter empties list, return original." |
| **G10** | LOW (EXTRA) | online-autoload `PER_LANG_MAX=6` + `firstAdded` (only first downloaded sub auto-selects). |
| G1/G15 | INFO | `baseRate`=`snap.rate` at keydown (restore target may be ≠1×); per-show prefs apply-once uses `!==` equality guards. |

The spec's CORE hotkey magnitudes, autohide timings (1800/4500), and video-fill modes are **accurate**. The real risk area is **track-autoload §8.8**, where G12 (precedence bug) is the one that would produce visibly wrong behavior (subs forced off when a user set a per-show "subs on" override against a global "off-by-default").
