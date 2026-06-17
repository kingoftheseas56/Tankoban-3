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
