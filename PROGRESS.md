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

## Token Ledger
- M1 estimate: 9k tokens
- M2 estimate: 16k tokens
