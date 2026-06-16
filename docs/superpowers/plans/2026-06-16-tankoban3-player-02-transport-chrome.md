# Tankoban 3 Player — Plan 2: Transport Chrome Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans (inline) to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax.

**Goal:** Recreate Harbor's player **transport chrome 1:1** as Qt overlay widgets on top of the Plan 1 video stage — seek bar, time, transport buttons, volume, and the speed / audio / subtitle menus — driven by the existing `MpvController` + `PlaybackClock`.

**Architecture:** A frameless chrome overlay (`TransportBar`) sits above `MpvGlWidget` inside `PlayerView`. It's a thin, **config-driven** recreation of Harbor's `transport.tsx` + `control-renderer.tsx`: a fixed `DEFAULT_DEFAULT_CONFIG` of controls placed into slots, each control a focused Qt widget. Custom-painted widgets (`SeekBar`, `VolumeControl`) port Harbor's exact geometry/math; menus (`SpeedMenu`/`AudioMenu`/`SubtitleMenu`) are frameless popups. Auto-hide + gradients match Harbor.

**Tech Stack:** C++20, Qt6 Widgets (custom `paintEvent`, `QPropertyAnimation`, `QGraphicsOpacityEffect`), the Plan 1 engine.

**Spec source of truth:** `docs/harbor-player-spec.md` — **read §0 (decisions/corrections) first**, then §5 (chrome), §6 (seek bar), §7 (time/volume/speed), §8 (tracks), §9.2 (auto-hide), §9.4/§9.6 (fill/pills), §10 (visual tokens), §11 (Qt component map). When this plan and the spec body conflict, **§0 wins**. Every task cites the spec section carrying the exhaustive token values; those values are committed + exact — treat a `(see §X)` as a precise instruction, not a placeholder.

**Verification model:** build + watch-it-look-right smoke (eyes-on, the project standard; pywinauto is disabled). Pure-logic helpers (`VolumeCurve`, gold color, fmtTime already done) get CTest self-test entries. Each task ends green + a visual smoke against the running player.

**Branch/worktree:** all work on `feat/player` in `C:\Users\Suprabha\Desktop\Tankoban-3-player`. Pre-rebuild discipline (Plan 1): `Stop-Process Tankoban3`; after CMakeLists changes delete `out/CMakeCache.txt`+`out/build.ninja`; verify exe mtime advanced. New files → add to `qt_add_executable` under the `src/player/` group.

---

## File Structure

| File | Responsibility |
|---|---|
| `src/util/Theme.h` | Shared chrome tokens: gold accent (`oklch(0.78 0.13 60)`→sRGB), white-alpha helpers, gradient stops (§10) |
| `src/chrome/SeekBar.{h,cpp}` | Custom-painted seek bar — value state machine, pointer→time, commit-on-release, gold fill+dot, hover tooltip (§6) |
| `src/chrome/TimeLabel.{h,cpp}` | `fmtTime` label bound to `PlaybackClock` (start/end/remaining) (§7.1) |
| `src/chrome/BigButton.{h,cpp}` | Reusable 48px circular tool button, 3 states + optional tooltip (§5.5) |
| `src/chrome/Tooltip.{h,cpp}` | Hover tooltip popup (NOT `QToolTip`), 150ms fade, 8px offset (§5.5) |
| `src/chrome/SeekStepButton.{h,cpp}` | 56px ±Ns button w/ numeric badge; 380ms-hold/right-click options popover (§0.1 #5, §6.2) |
| `src/chrome/PlayPauseButton.{h,cpp}` | Hero 64/56/48 circular button, PlayCircle/PauseCircle (§5.4) |
| `src/chrome/VolumeControl.{h,cpp}` + `src/util/VolumeCurve.{h,cpp}` | Horizontal 120×8 slider + mute, boost curve (§7.2, §0.1 #1) |
| `src/chrome/SpeedMenu.{h,cpp}` | Gauge trigger + 7-row popover [0.5..2] (§7.3) |
| `src/chrome/AudioMenu.{h,cpp}` | Languages trigger + 360px track popup (§8.5) |
| `src/chrome/SubtitleMenu.{h,cpp}` | Subtitles trigger + 500×400 popup (rail/tabs/variant rows/delay) + active dot (§8.1–8.4) |
| `src/subs/SubStyle.{h,cpp}` | `applySubStyle` → `sub-*` mpv props (§8.6) |
| `src/engine/TrackAutoload.{h,cpp}` | `langScore`/`pickBestTrack` + subsOff precedence + autoload (§8.8, §0.2) |
| `src/chrome/ChromeConfig.h` | `ControlId`/`Slot`/`ControlConfig` + `DEFAULT_DEFAULT_CONFIG` + `controlsInSlot` (§5.3, §5.6) |
| `src/chrome/TransportBar.{h,cpp}` | The overlay: top + bottom bars, gradients, slot layout, control renderer, auto-hide visibility (§5.2, §5.7, §9.2) |
| `src/player/VideoFill.{h,cpp}` | 6 fill/zoom modes + pill (§9.4) |
| `src/player/StageOverlays.{h,cpp}` | Hold-speed / video-fill pills (§9.6) |
| `src/player/PlayerView.{h,cpp}` (modify) | Host `TransportBar` over the video; wire onPlayPause/onSeek/onFullscreen + menu-open→auto-hide; hotkeys call VideoFill |

---

## Task 1: Theme tokens + `SeekBar` (seek bar first)

**Files:** Create `src/util/Theme.h`, `src/chrome/SeekBar.h`, `src/chrome/SeekBar.cpp`; modify `CMakeLists.txt`, temporarily `src/player/PlayerView.cpp` (drop the seek bar at the bottom to smoke it).

- [ ] **Step 1: `src/util/Theme.h`** — the gold accent + alpha helpers (§10).

```cpp
#pragma once
#include <QColor>
namespace theme {
// Harbor seek/accent gold = oklch(0.78 0.13 60) -> sRGB (computed). Matches our OKLCH gold ladder.
inline QColor accentGold()      { return QColor(244, 162, 92); }   // #F4A25C
inline QColor white(double a)   { QColor c(255,255,255); c.setAlphaF(a); return c; }
inline QColor black(double a)   { QColor c(0,0,0);       c.setAlphaF(a); return c; }
}
```

- [ ] **Step 2: `SeekBar.h`** — three-layer value state machine + clock-driven repaint (§6.1).

```cpp
#pragma once
#include <QWidget>
#include <optional>
class SeekBar : public QWidget {
    Q_OBJECT
public:
    explicit SeekBar(QWidget* parent = nullptr);
    void setDuration(double sec);                  // from snapshot
signals:
    void seekRequested(double sec);                // commit on release
    void hoveringChanged(bool hovering);           // -> chrome stays visible (§9.2)
protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void leaveEvent(QEvent*) override;
private slots:
    void onClock();                                // PlaybackClock::changed
private:
    double timeAt(int x) const;                    // clamp((x)/w,0,1)*dur
    double m_dur = 1.0;
    std::optional<double> m_hover, m_scrub, m_pending;
    bool m_dragging = false;
};
```

- [ ] **Step 3: `SeekBar.cpp`** — port §6.2 math + §6.3 visual layers exactly.

Key requirements (cite §6):
- ctor: `setMouseTracking(true)`, fixed height 48 (`setMinimumHeight(48)`), `connect(&PlaybackClock::instance(), &PlaybackClock::changed, this, &SeekBar::onClock)`.
- `timeAt(x) = clamp(double(x)/width(),0,1) * m_dur`.
- **value** = `m_scrub ? *m_scrub : (m_pending ? *m_pending : PlaybackClock::instance().position())`.
- `pct = clamp(value/dur,0,1)`; `bufferedPct = clamp((pos + PlaybackClock::buffered())/dur,0,1)` (dead→just pos).
- `mousePress` (left): `grabMouse()`, `m_pending.reset()`, `m_scrub = timeAt(x)`, `m_dragging=true`, update.
- `mouseMove`: `m_hover = timeAt(x)`; if `m_dragging` `m_scrub = timeAt(x)`; emit `hoveringChanged(true)`; update.
- `mouseRelease`: `releaseMouse()`; if `m_scrub` { `emit seekRequested(*m_scrub)`; `m_pending = *m_scrub`; } `m_scrub.reset(); m_dragging=false; update`.
- `leaveEvent`: `m_hover.reset()`; emit `hoveringChanged(false)`; update.
- `onClock`: if `m_pending && abs(pos - *m_pending) < 0.75` clear pending; update.
- **paintEvent (bottom→top, §6.3):** track rect centered, height 6 (8 if hover/scrub), rounded; bg `theme::white(0.15)`; buffered rect `white(0.28)` width=bufferedPct; played fill `accentGold()` width=pct; handle: filled gold circle Ø16 (20 if scrubbing) at x=pct·w, with a 4px `black(0.45)` ring. (Animate height 6→8 / dot 16→20 via `QVariantAnimation`, 150/200ms — optional in this task, can land as polish.)
- **hover tooltip (§6.4):** when `m_hover` set, draw a small `fmtTime(*m_hover)` bubble 36px above at the hover x (black(0.9) bg, white text, mono). Can be a child `QLabel` or painted.

- [ ] **Step 4: Smoke the seek bar.** Temporarily in `PlayerView` ctor: create a `SeekBar`, place it as a child along the bottom (`setGeometry(40, height()-60, width()-80, 48)` in resizeEvent), `connect(seek,&SeekBar::seekRequested, m_controller,&MpvController::seek)`, and `connect(snapshotChanged, set m_dur)`. Build + run the demo.
Expected (eyes-on): a **gold seek bar** at the bottom; the gold fill + dot **advance with playback**; **click-drag scrubs**, releasing **seeks** (video jumps); hovering shows a time bubble.

- [ ] **Step 5: Commit.**
```bash
git add src/util/Theme.h src/chrome/SeekBar.h src/chrome/SeekBar.cpp src/player/PlayerView.cpp CMakeLists.txt
git commit -m "feat(chrome): SeekBar — Harbor seek-bar geometry + scrub/commit-on-release (§6)"
```

---

## Task 2: `TimeLabel` (start / end / remaining)

**Files:** Create `src/chrome/TimeLabel.{h,cpp}`; modify `CMakeLists.txt`, `PlayerView.cpp` (place start/end around the seek bar to smoke).

- [ ] **Step 1: `TimeLabel`** — a `QLabel` bound to `PlaybackClock`, formatted via `fmtTime` (§7.1). Modes: `Start` (position), `End` (duration), `Remaining` (`-fmtTime(max(0,dur-pos))`). Font: monospace 13px, tabular, `white(0.85)` start / `white(0.65)` end; painted drop-shadow optional. `setDuration(double)` from snapshot. Connect `PlaybackClock::changed` → refresh.
- [ ] **Step 2: Smoke.** Place `TimeLabel(Start)` left of the seek bar, `TimeLabel(End)` right (temporary layout in PlayerView). Build + run: left shows elapsed counting up, right shows total. (eyes-on)
- [ ] **Step 3: Commit.** `feat(chrome): TimeLabel bound to PlaybackClock (fmtTime §7.1)`

---

## Task 3: Chrome overlay container + auto-hide (`TransportBar` skeleton)

**Files:** Create `src/chrome/TransportBar.{h,cpp}`, `src/chrome/ChromeConfig.h`; modify `CMakeLists.txt`, `PlayerView.{h,cpp}`.

- [ ] **Step 1: `ChromeConfig.h`** — port §5.3/§5.6: `enum ControlId`, `enum Slot { TopLeft, TopRight, SeekLeading, SeekTrailing, BottomLeft, BottomCenter, BottomRight }`, `struct ControlConfig { ControlId id; Slot slot; int order; bool hidden; }`, the **`DEFAULT_DEFAULT_CONFIG`** vector (exact order from §5.3), and `controlsInSlot(slot)` (filter+sort by order).
- [ ] **Step 2: `TransportBar`** — a `QWidget` overlay (child of PlayerView, fills it, `WA_TransparentForMouseEvents` on the frame; real control clusters get mouse). Two bands:
  - **Top band**: `paintEvent` gradient black `0.55→0.15→0` downward; padding 28/16/32; left=back slot, right=title-info slot (§5.2).
  - **Bottom band**: gradient black `0.70→0.25→0` upward; padding 28/40/20; a vertical stack: seek row (`TimeLabel | SeekBar(expanding) | TimeLabel`, `QHBoxLayout` gap 12) then control row (`QGridLayout` stretch `1,0,1`: left/center/right clusters) (§5.2, §5.7).
  - **Auto-hide (§9.2):** opacity via `QGraphicsOpacityEffect` + `QPropertyAnimation` (300ms). One `QTimer` (singleShot) restarted on `wakeChrome()`; interval `playing ? 1800 : 4500`. Don't re-arm while a menu is open or the seek bar is hovered. `setVisible`-style fade. Expose `wakeChrome()` + `setMenuOpen(bool)`.
- [ ] **Step 3: Wire into `PlayerView`** — replace the temporary Task-1/2 placements: PlayerView owns a `TransportBar` over the video; forward `mouseMove` on the video → `bar->wakeChrome()`; connect SeekBar inside the bar to the controller; cursor hides with chrome when playing (§9.2).
- [ ] **Step 4: Smoke.** Build + run: gradients top+bottom, seek row centered in the bottom band; chrome **fades out ~1.8s after the mouse stops** while playing, **reappears on mouse move**; hovering the seek bar keeps it up. (eyes-on)
- [ ] **Step 5: Commit.** `feat(chrome): TransportBar overlay — gradients + slot layout + auto-hide (§5.2/§9.2)`

---

## Task 4: Shared primitives — `BigButton` + `Tooltip`

**Files:** Create `src/chrome/BigButton.{h,cpp}`, `src/chrome/Tooltip.{h,cpp}`; modify `CMakeLists.txt`.

- [ ] **Step 1: `Tooltip`** — frameless popup `QWidget` (NOT QToolTip), 150ms fade, anchored 8px above/below an anchor widget; style per §5.5 (black(0.9) bg, white(0.1) border, 12px text). API: `showFor(QWidget* anchor, QString text, Side)`.
- [ ] **Step 2: `BigButton`** — `QToolButton` subclass, 48×48 circle, three visual states (default/active/disabled) per §5.5, optional tooltip text + icon (SVG name). Painted via stylesheet or `paintEvent`.
- [ ] **Step 3: Build** (compile-only; exercised in Task 5). `BUILD OK`.
- [ ] **Step 4: Commit.** `feat(chrome): BigButton + Tooltip primitives (§5.5)`

---

## Task 5: Transport control row — back, play-pause, seek-step, fullscreen

**Files:** Create `src/chrome/PlayPauseButton.{h,cpp}`, `src/chrome/SeekStepButton.{h,cpp}`; modify `TransportBar.cpp` (render these controls into slots), `CMakeLists.txt`. Bundle lucide-style SVG icons under `resources/icons/` (ChevronLeft, PlayCircle, PauseCircle, RotateCcw, RotateCw, Maximize, Minimize, Gauge, Languages, Subtitles, Volume2, VolumeX).

- [ ] **Step 1: `PlayPauseButton`** — hero circular button 64/56/48 by width breakpoint (§5.4), PlayCircle/PauseCircle icon, `bg-white/12 hover/22`, press `active:scale-95` via `QPropertyAnimation`. Reflects snapshot status; emits `clicked`.
- [ ] **Step 2: `SeekStepButton`** — 56×56, `RotateCcw`/`RotateCw` icon size 32 + a **centered numeric badge** (`{seconds}`, mono bold 10.5px) painted on the face (§0.1 #5). 380ms-hold OR right-click → options popover `[5,10,15,30,60,90]`; per-direction persist (`QSettings harbor.seek-step.{back|fwd}`). Default 10s. Emits `seekStep(±n)`.
- [ ] **Step 3: Render into `TransportBar`** — populate slots from `DEFAULT_DEFAULT_CONFIG`: top-left **back** (`BigButton` ChevronLeft 26 → `onBack`), bottom-center **seek-back / play-pause / seek-forward**, bottom-right **fullscreen** (`BigButton` Maximize/Minimize 22 → `onFullscreen`). Prev/next-episode = disabled placeholders (EXTRA). Wire signals up to `PlayerView` (onBack=close, onFullscreen=toggleFullscreen, play-pause=playPauseToggle, seek-step=`controller->seek(clock.pos±n)`).
- [ ] **Step 4: Smoke.** Build + run: the **control row** shows back (top-left), and centered **⟲10 ▶/⏸ 10⟳** + **fullscreen** (bottom-right). All click correctly; play-pause icon reflects state; fullscreen button toggles (blink-free). (eyes-on)
- [ ] **Step 5: Commit.** `feat(chrome): transport row — back/play-pause/seek-step/fullscreen (§5.3-5.4)`

---

## Task 6: `VolumeControl` (horizontal slider + boost curve)

**Files:** Create `src/util/VolumeCurve.{h,cpp}`, `src/chrome/VolumeControl.{h,cpp}`; modify `TransportBar.cpp` (bottom-left slot), `CMakeLists.txt`, `src/selftest_main.cpp` (curve cases).

- [ ] **Step 1: `VolumeCurve`** (pure logic, §7.2) — port the 4 helpers verbatim: `fractionFromValue(v)`, `valueFromFraction(f)`, `boostColor(v)` (lerp `#f97316`→`#dc2626` above 1×), constants `VOL_MAX=6`, `NORMAL_FRACTION=0.6`. Add self-test cases (e.g. `fractionFromValue(1.0)==0.6`, `valueFromFraction(0.6)==1.0`).
- [ ] **Step 2: `VolumeControl`** — **horizontal** (§0.1 #1): mute `BigButton` 48×48 (Volume2/VolumeX 24), track 120×8 `white(0.15)`, fill width `fractionFromValue(v)`, handle Ø14, boost-only pct readout. Wheel ±0.05 (±0.5 shift). Reads snapshot volume/mute; emits `setVolume(0..6)` / `toggleMute`.
- [ ] **Step 3: Render into TransportBar** bottom-left slot; wire to `controller->setVolume`/`setMuted`.
- [ ] **Step 4: Smoke + self-test.** Build; `out/Tankoban3SelfTest.exe` → `SELFTEST OK`. Run: volume slider in bottom-left, drag changes volume (audible), wheel works, boost tints above 100%. (eyes/ears-on)
- [ ] **Step 5: Commit.** `feat(chrome): VolumeControl horizontal slider + boost curve (§7.2, self-tested)`

---

## Task 7: `SpeedMenu`

**Files:** Create `src/chrome/SpeedMenu.{h,cpp}`; modify `TransportBar.cpp` (bottom-right), `CMakeLists.txt`.

- [ ] **Step 1: `SpeedMenu`** (§7.3) — Gauge trigger (`h-11`, accent when rate≠1, shows `{rate}×` inline); click → frameless popover 400px above-right with 7 rows `[0.5,0.75,1,1.25,1.5,1.75,2]`, selected highlighted; dismiss on outside-click/Escape. Emits `setRate(r)` + `menuOpenChanged(bool)` (→ TransportBar auto-hide suppression).
- [ ] **Step 2: Render** into bottom-right slot; wire `setRate`→controller, `menuOpenChanged`→`bar->setMenuOpen`.
- [ ] **Step 3: Smoke.** Build + run: speed button bottom-right; opens 7-row menu; selecting changes playback speed; menu keeps chrome visible. (eyes-on)
- [ ] **Step 4: Commit.** `feat(chrome): SpeedMenu — 7-step popover (§7.3)`

---

## Task 8: Track menus + sub styling + autoload — `AudioMenu`, `SubtitleMenu`, `SubStyle`, `TrackAutoload`

**Files:** Create `src/chrome/AudioMenu.{h,cpp}`, `src/chrome/SubtitleMenu.{h,cpp}`, `src/subs/SubStyle.{h,cpp}`, `src/engine/TrackAutoload.{h,cpp}`; modify `TransportBar.cpp`, `CMakeLists.txt`, `selftest_main.cpp` (langScore cases).

- [ ] **Step 1: `TrackAutoload`** (§8.8, §0.2) — pure logic: `langScore(lang, prefs)`, `pickBestTrack(tracks, langs)` (skip forced), the **subsOff 4-step precedence** (§0.2), audio switch-on-id-difference, native-audio forced-sub pick (`isForcedTrack` regex). Self-test `langScore`/`pickBestTrack`. Run on each `track-list` snapshot.
- [ ] **Step 2: `SubStyle`** (§8.6) — `applySubStyle(settings, assActive)` issues the 12 `sub-*` `set_property` calls per the table; `mpvFontFor`/`hexToBgr` helpers. (Subs render via libass into the GL surface — no overlay.)
- [ ] **Step 3: `AudioMenu`** (§8.5) — Languages trigger; 360px popup; track rows (flag/title/lang·codec·channels), audio sync ±0.1 row; click → `setAudioTrack(id)`.
- [ ] **Step 4: `SubtitleMenu`** (§8.1–8.4) — Subtitles trigger + **emerald active-dot** when a sub is selected; 500×400 popup: 128px language rail (Off/On + per-lang groups), source tabs (All/Embedded/External) + HI/Forced chips, scrollable variant rows, **Load file** (`QFileDialog`, srt/ass/ssa/vtt/sub), delay row ±. Click → `setSubtitleTrack(id|"")`; load → `controller->addSubtitle` (Plan 3 can add online search). Sliders button → sub-style bar (CORE-lite; can stub to settings).
- [ ] **Step 5: Render** audio + subtitle into bottom-right slot (order: audio, subtitle, speed); wire menu-open→auto-hide; apply `SubStyle` on track-load; run `TrackAutoload` on track-list.
- [ ] **Step 6: Smoke + self-test.** Build; `SELFTEST OK`. Run with a file that has audio + sub tracks (e.g. the Bad Sisters mkv): audio menu lists tracks + switches; subtitle menu lists + toggles, active-dot shows, **subs render on the video**; Load-file works. (eyes-on)
- [ ] **Step 7: Commit.** `feat(chrome): Audio/Subtitle menus + SubStyle + TrackAutoload (§8)`

---

## Task 9: `VideoFill` + `StageOverlays` pills + hotkey wiring

**Files:** Create `src/player/VideoFill.{h,cpp}`, `src/player/StageOverlays.{h,cpp}`; modify `HotkeyDispatcher.cpp` (wire `v`/`=`/`-`), `PlayerView.cpp`, `CMakeLists.txt`.

- [ ] **Step 1: `VideoFill`** (§9.4) — 6 modes `fit/fill/zoom/16:9/4:3/2.39:1`, `cycle()` (`v`) + `step(±0.1)` (`=`/`-`); `apply()` sets `panscan`/`video-aspect-override`/`video-zoom` on the controller. Pill text per mode.
- [ ] **Step 2: `StageOverlays`** (§9.6) — top-center pills: **hold-speed** (`{rate}x speed`, while hold-FF active) and **video-fill** (mode label, 1200ms auto-dismiss). Frameless `QLabel`-style, `canvas(0.85)` bg.
- [ ] **Step 3: Wire** — `HotkeyDispatcher`: `v`→`videoFill.cycle()`, `=`/`-`→`videoFill.step(±0.1)`, and the hold-speed pill on Space-hold (already detected in Plan 1). PlayerView owns `VideoFill` + `StageOverlays`.
- [ ] **Step 4: Smoke.** Build + run: `v` cycles aspect (pill shows Fit/Fill/Zoom/…), `=`/`-` zoom, hold-Space shows the speed pill. (eyes-on)
- [ ] **Step 5: Commit.** `feat(player): VideoFill modes + StageOverlays pills (§9.4/§9.6)`

---

## Task 10: Final assembly + milestone smoke

**Files:** Modify `PlayerView.cpp`, `README.md`.

- [ ] **Step 1: Final wiring pass** — confirm every `DEFAULT_DEFAULT_CONFIG` core control is placed + wired; menu-open suppresses auto-hide; seek-bar hover keeps chrome up; cursor hides with chrome.
- [ ] **Step 2: Milestone smoke (acceptance, eyes-on).** Run the demo on a multi-track file. Verify against Harbor: bottom chrome = `[time] [seek bar] [time]` over `[volume] · [⟲10 ▶ 10⟳] · [audio sub speed fullscreen]`; top = back + title; gradients + auto-hide feel right; all controls work; subs render; fullscreen blink-free. Compare side-by-side with Harbor's player screenshot.
- [ ] **Step 3: README** note: chrome controls list. Commit. `feat(chrome): Plan 2 transport chrome complete — Harbor 1:1`

---

## Self-Review (spec coverage)

§5.2 container/gradients → T3 ✓ · §5.3 layout/slots → T3 (ChromeConfig) ✓ · §5.4 controls → T5 ✓ · §5.5 BigButton/Tooltip → T4 ✓ · §5.6 slot engine + menu-open hook → T3/T7 ✓ · §6 seek bar → T1 ✓ · §7.1 time → T2 ✓ · §7.2 volume → T6 ✓ · §7.3 speed → T7 ✓ · §8 tracks/substyle/autoload → T8 ✓ · §9.2 auto-hide → T3 ✓ · §9.4 fill / §9.6 pills → T9 ✓ · §10 tokens → T1 Theme ✓ · §11 component map → file structure ✓.

**§0 corrections honored:** horizontal volume (T6), seek-step badge+hold-options (T5), subsOff precedence (T8). **Deferred (EXTRA, not Plan 2):** cast/together/pip/dvr/draw/gif/multiview, thumbnail trickplay, online subtitle search, chrome profile customizer, alt shells, stats HUD.

---

## Execution Handoff
Inline execution (continuing this session), task-by-task, smoke after each. Seek bar (Task 1) first per Hemanth.
