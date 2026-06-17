# Tankoban 3 - Playback Spine Progress

## Milestone 1: StreamModels.h
- Status: Done
- Built: StreamModels.h - Stream -> ParsedStream -> ScoredStream three-tier types
- Files: src/core/StreamModels.h
- Decisions: C++ uses explicit sentinels for Harbor's nullable/undefined TS fields: Resolution::None, HdrFormat::None, Codec/AudioCodec/Source::Unknown, Container::Unknown, empty QString for nullable strings, size=0, seeders=-1, year/yearStart/yearEnd=0, season/episode=0, discIndex=-1, and std::optional primary. Safe ONLY because StreamParser/StreamScorer treat these as unknown/no-op, never as meaningful values.
- Smoke: build.bat + compile-probe both exit 0
- Known issues: none
- Next: M2 - StreamService::buildStreamIds + fetchStreams skeleton

## Milestone 2: StreamService
- Status: Done
- Built: StreamService - buildStreamIds + fetchStreams raw addon stream service
- Files: src/core/StreamService.h, src/core/StreamService.cpp, CMakeLists.txt
- Decisions: V1 adapter uses only current MetaDetail/EpisodeItem fields, so Harbor defaultVideoId, mapped IMDb episode fields, per-episode videoId, and Kitsu stream id are deferred; anime fallback uses current season/episode fields. Harbor's slow-addon timeout markers are preserved for Torrentio-class addons; addonRanked remains false until Harbor addon-detect is ported.
- Smoke: build.bat + buildStreamIds probe both exit 0
- Known issues: fetchStreams end-to-end smoke deferred until a later wired milestone with a running stream addon
- Next: M3 - StreamParser label parsing and ParsedStream population

## Milestone 3: StreamParser
- Status: Done
- Built: StreamParser - parseStream pipeline, filename selection, PTT-equivalent extraction, resolution/source/codec/HDR/audio/language/metadata parsing
- Files: src/core/StreamParser.h, src/core/StreamParser.cpp, CMakeLists.txt
- Decisions: parser-cache-flags.ts deferred per no-debrid v1; cached and inLibrary remain empty (anitomy.ts is not part of parseStream — only pipeline.ts uses it — so it is out of M3 scope). parsePtt() is a QRegularExpression approximation of the parse-torrent-title npm library (the single sanctioned adaptation) covering title/year/season/episode/group/edition flags/channels/bitdepth; the sub-parsers (resolution/source/codec/hdr/audio/language/metadata/trusted-groups) are faithful 1:1 ports. Qt accepted the TS/TC source lookbehinds, so no source regex deviation was required. TORRENTIO_NOISE / line-start-emoji filters use Unicode \p{So}\p{Cf} property classes as a broader-but-benign approximation of Harbor's literal emoji set (these only strip/penalize, never extract data). isTrustedGroup + the 50-group table live in StreamParser per the parser folder, but parseStream does not call them (Harbor's parser does not either — scoring consumes trust); StreamScorer keeps its own copy.
- Smoke: build.bat + StreamParser probe both exit 0
- 2026-06-17 Opus M3 audit (vs Harbor parser/*.ts): two parity corrections applied — (1) parseSeeders restored to Harbor's exact seeder-emoji set (U+1F465/U+1F464 via \x{} escapes) instead of a generic \p{So} fallback that misread size/other emoji-tagged numbers as seeders; (2) computeScamScore now penalizes only Resolution::SD (not 480p), matching Harbor exactly. parsedTitle's text[:100] fallback when filenameLine is empty is per the M3 spec (Harbor returns "" there) and is left as a benign intentional deviation.
- Known issues: Harbor parity reviewed by Opus; end-to-end addon-data parsing still smoked in a later wired milestone
- Next: M4 - StreamScorer

## Milestone 4: StreamScorer
- Status: Done
- Built: StreamScorer - scoreStream, rankAndPick, computeCorpusStats, and Harbor scoring helper functions with v1 data gates
- Files: src/core/StreamScorer.h, src/core/StreamScorer.cpp, CMakeLists.txt
- Decisions: debrid/cache slug scoring remains deferred; direct URL ranking and direct-url scoring are active. Corpus, release-date, runtime, and bandwidth rules are implemented but no-op when their inputs are absent.
- Smoke: build.bat + StreamScorer probe both exit 0
- Known issues: Harbor parity still needs review against scoring/*.ts before merge safety is self-certified
- Next: M5 - PlayPickerPage

## Milestone 5: Play Picker Page Shell
- Status: Done
- Built: PlayPickerPage overlay shell, BackdropLayer, PickerHeader, fixed back button, loading spinner, detail-metadata backdrop refresh, and maximized app startup
- Files: src/ui/PlayPickerPage.h, src/ui/PlayPickerPage.cpp, src/ui/PickerHeader.h, src/ui/PickerHeader.cpp, src/ui/BackdropLayer.h, src/ui/BackdropLayer.cpp, src/ui/MainWindow.h, src/ui/MainWindow.cpp, src/main.cpp, CMakeLists.txt
- Decisions: M5 is shell-only; stream list, primary card, tier strip, modals, and real stream pipeline remain deferred. DetailPage untouched; MainWindow owns play/episode wiring (episode copied by value via std::optional). Picker is a direct full-rect child of MainWindow (covers the sidebar) and hides on back without changing the content route. Backdrop is a fixed sibling behind a TRANSLUCENT content layer (no scroll viewport — see audit note) so it shows through full-frame; image is scale-105 / object-cover / opacity-0.5 with vertical+horizontal canvas gradients, not blurred. The picker runs a guarded MetaService /meta refresh (keyed on m_currentId, so stale replies can't update the wrong picker) to upgrade the backdrop/genres after open. BackdropLayer.upgradeArt() rewrites background/small|medium -> large for a crisper full-screen image. Maximized startup (main.cpp showMaximized) is a Hemanth-requested product decision accepted during the M5 smoke.
- 2026-06-17 Opus M5 audit (vs Harbor play-picker / backdrop-layer / picker-header): PATCHED PickerHeader typography only — (1) moved from QSS to QFont, because Qt Style Sheets silently ignore letter-spacing / line-height / text-transform on QLabel (the tracking-0.32em kicker was rendering with no tracking); (2) kicker separators changed from " | " pipe bars to Harbor's " · " middot, with the movie kicker row gated on releaseInfo. A QScrollArea (Harbor's overflow-y-auto root, also in the original M5 spec) was TRIED and REVERTED: the scroll viewport caches its composite and does NOT repaint when the sibling backdrop's async image lands, so the first open showed the backdrop only as a strip behind the header (second open, image cached, painted full). This is the exact reason the original M5 wake removed the scroll viewport. The fixed translucent content layer is kept; a real scrollable shell is deferred to M6 where the stream list creates actual overflow and the scroll architecture can be solved against real content (e.g. a custom scroll that repaints the backdrop, or scrolling the content while the backdrop stays painted on the page itself). Remaining Qt limitation: the title's leading-[0.96] line-height is not expressible via QSS/QFont (single-line titles unaffected; multi-line wrap is slightly looser than Harbor) — NOT 1:1 on that one detail.
- Smoke: build.bat exit 0 (with typography patch); app launches maximized, Home renders, Detail opens, no crash. Hemanth eye-smoke caught the scroll-induced first-open backdrop strip -> scroll reverted; typography dot-separators + letter-spacing confirmed correct in his screenshot.
- Known issues: typography patch awaits cross-engine review before M6; a proper scrollable shell + source list + Harbor stream-control parity remain for M6+ when Torrentio/stream rows are wired.
- Next: M6 - Stream list / Stremio rows (solve the scrollable shell against real content there)

## Milestone 6: Play Picker Stream List + Stremio Rows
- Status: Done
- Built: StremioRow (addon tile, headline/description, FormatBadge-equivalent text chips, copy-link, circular play button, unsupported/failed states) + StreamList (addon/source filter popup, quality-tier pills, ranked rows, pending-addons pill) + PlayPickerPage::setPicker/setLoading + streamSelected signal. Added the scrollable picker shell (Harbor overflow-y-auto) and fixed the M5 backdrop-striping root cause in the same change.
- Files: src/ui/StremioRow.h, src/ui/StremioRow.cpp, src/ui/StreamList.h, src/ui/StreamList.cpp, src/ui/PlayPickerPage.h, src/ui/PlayPickerPage.cpp, CMakeLists.txt
- Decisions: badge chips translate Harbor format-badge.tsx streamBadges()/qualityConfidence() exactly (text chips, NOT the image pack) — source-capture override, no-label/unknown confidence, then release/codec/HDR/audio. One additive deviation: SCR renders an "SCR" chip although Harbor's sourceBadge() omits SCR (it tiers SCR as ROUGH in the scorer; the chip is honest and prompt-specified). StreamList preserves StreamScorer ranking order (no re-score/re-sort); addon-filter key = addonUrl||addonId; quality groups 4K/1080p/720p/SD shown only when >=2 groups. Direct-playable = url non-empty && url != "#"; unsupported rows (hash-only / external / yt / nzb / "#") are visible but muted with a reason and emit nothing. No network addon logos in v1 (initial-letter tile). Scroll: the backdrop stays a lowered SIBLING (not inside the viewport) and the scroll viewport is WA_TranslucentBackground, so it shows the backdrop AND recomposites when the backdrop's async image lands — that missing-translucent-viewport was the M5 first-open striping cause, now fixed. Committed normal flow still shows the M5 loading placeholder; setPicker is the M9 entry point.
- Smoke: build.bat exit 0. Temporary fixture probe (3 ScoredStreams, deleted before commit, clean rebuild exit 0) verified: addon filter, quality pills + counts (All 3 / 4K 2 / 1080p 1), ranked rows at full height, rich badge chips (4K REMUX HEVC DV ATMOS), CAM red danger chip + muted/disabled play + "Torrent source — resolver later", gold play on direct rows, copy buttons, pending-addons pill, and a bright backdrop rendering full-frame through the scroll with NO striping/banding.
- Known issues: backdrop-through-scroll verified with a local bright test image (no striping); a real-title Hemanth eye-smoke is still the gate before M7+. PrimaryCard / TierStrip / SourceDrawer / empty-state ladder / modals / autoplay and real StreamService wiring remain for M7-M9. streamSelected is emitted but intentionally not connected to a resolver/player (M9).
- Next: M7 - TierStrip / PrimaryCard / SourceDrawer (per next prompt)

## Playback Spine: In-app Player Seam + Frameless Shell
- Status: Done (Hemanth-smoked 2026-06-17)
- Built: Wired the Play Picker -> in-app player seam and made the app window frameless
  (Harbor-faithful, no OS title bar). Direct-link playback only.
- Files: src/ui/MainWindow.h/.cpp, src/ui/VideoPlayerPage.h/.cpp (new),
  src/player/PlayerView.h/.cpp, src/player/HotkeyDispatcher.cpp, src/main.cpp, CMakeLists.txt
- Decisions:
  - MainWindow owns QNetworkAccessManager + StreamService. openPlayPicker -> setLoading +
    fetchStreams; streamsPartial/Ready -> buildPicker (StreamParser -> StreamScorer
    computeCorpusStats/scoreStream/rankAndPick) -> PlayPickerPage::setPicker; streamsFailed ->
    "No stream addons configured".
  - PlayPickerPage::streamSelected -> MainWindow::openDirectPlayer: guards url (non-empty,
    != "#"); lazy-creates VideoPlayerPage as a content-stack PAGE (not an overlay); hides
    sidebar + top bar + the picker overlay; switches the stack to the player BEFORE play()
    so the GL surface is exposed when the first mpv frame lands; plays. Back -> closePlayer:
    stop, restore chrome, re-show the picker.
  - VideoPlayerPage (new) wraps the player track's PlayerView + a tiny PlayerRequest
    {url,title,subtitle,startSec}.
  - PlayerView/HotkeyDispatcher back-safety: Back button + Esc now emit
    PlayerView::backRequested() instead of window()->close() (embedding otherwise closed the
    whole app); Esc exits fake-fullscreen first. main.cpp standalone demo connects
    backRequested -> close to preserve old behavior. (Touches the player track; Agent 4's
    "ask, don't grab" fullscreen refactor builds on top.)
  - Frameless shell (Harbor topbar.tsx + tauri.conf decorations:false): FramelessWindowHint +
    WS_THICKFRAME re-added (Tankoban 2 technique) for native snap/resize/drag/shadow;
    WM_NCCALCSIZE=0 (full client); WM_NCHITTEST -> HTCAPTION on empty top-bar via
    QCursor::pos() (DPI-correct; raw lParam mismatched on scaled displays and killed the
    control-button clicks); custom min/max/close cluster (hand-painted glyphs); DWM
    immersive-dark frame + border-color none.
  - Maximize geometry: WM_GETMINMAXINFO constrains the maximized window to the monitor WORK
    AREA (not full monitor), so the real taskbar stays visible and the client is full-bleed
    with no white rim. DPI-agnostic (no GetSystemMetrics frame math, which mismatched on
    scaled displays and had left an ~11px white top/left rim + a white strip over the taskbar).
  - Render-freeze root cause: MpvGlWidget's redraw-pending coalescing flag sticks true if
    update() doesn't reach paintGL while the GL surface is unexposed (as a raised overlay).
    Hosting the player as a content-stack page (deterministic expose) fixes it.
- Smoke: build.bat exit 0 (clean, no scaffolding). Hemanth eye-smoke: video plays in-app
  (motion+audio), fills window, Back returns to picker, no white frame, taskbar visible.
  White-frame removal verified via pixel-sampled screenshots (all edges dark, #fff gone).
- Deferred: maximize/restore button (frameless window-state tracking), one-time first-open
  warm-up flash, Harbor transparent-floating top bar -> shell-polish pass. Real fullscreen
  seam (player asks, shell drives window) -> Agent 4. No real direct-link stream addon yet,
  so the committed app shows "No stream addons configured" on Play until that milestone.
- Next: real direct-link source addon (self-hosted direct-HTTP Stremio addon) so real titles
  play; then torrents/debrid resolver; M7 TierStrip/PrimaryCard/SourceDrawer picker polish.

## Token Ledger
- M1 estimate: 9k tokens
- M2 estimate: 16k tokens
- M3 estimate: 31k tokens
- M4 estimate: 24k tokens
- M5 estimate: 23k tokens
- M6 estimate: 42k tokens
