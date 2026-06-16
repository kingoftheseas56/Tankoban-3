# Tankoban 3 - Playback Spine Progress

## Milestone 1: StreamModels.h
- Status: Done
- Built: StreamModels.h - Stream -> ParsedStream -> ScoredStream three-tier types
- Files: src/core/StreamModels.h
- Decisions: none
- Smoke: build.bat + compile-probe both exit 0
- Known issues: none
- Next: M2 - StreamService::buildStreamIds + fetchStreams skeleton

## Milestone 2: StreamService
- Status: Done
- Built: StreamService - buildStreamIds + fetchStreams raw addon stream service
- Files: src/core/StreamService.h, src/core/StreamService.cpp, CMakeLists.txt
- Decisions: v1 adapter keeps anime fallback on current EpisodeItem fields only; 8s flat timeout; addonRanked remains false
- Smoke: build.bat + buildStreamIds probe both exit 0
- Known issues: fetchStreams end-to-end smoke deferred until a later wired milestone with a running stream addon
- Next: M3 - StreamParser label parsing and ParsedStream population

## Milestone 3: StreamParser
- Status: Done
- Built: StreamParser - parseStream pipeline, filename selection, PTT-equivalent extraction, resolution/source/codec/HDR/audio/language/metadata parsing
- Files: src/core/StreamParser.h, src/core/StreamParser.cpp, CMakeLists.txt
- Decisions: parser-cache-flags.ts deferred per no-debrid v1; cached and inLibrary remain empty. Qt accepted the TS/TC lookbehind translations, so no source regex deviation was required.
- Smoke: build.bat + StreamParser probe both exit 0
- Known issues: Harbor parity still needs review against parser/*.ts before merge safety is self-certified
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
- Decisions: M5 is shell-only; stream list, primary card, tier strip, modals, and real stream pipeline remain deferred. DetailPage was not edited; MainWindow owns play/episode signal wiring. The M5 shell uses a fixed content layer rather than a scroll viewport until source rows create real overflow.
- Smoke: build.bat exit 0; Hemanth eye-smoke accepted after backdrop/full-page layering fix
- Known issues: Source list and Harbor stream-control parity remain for M6+ when Torrentio/stream rows are wired
- Next: M6 - Stream list / Stremio rows

## Token Ledger
- M1 estimate: 9k tokens
- M2 estimate: 16k tokens
- M3 estimate: 31k tokens
- M4 estimate: 24k tokens
- M5 estimate: 23k tokens
