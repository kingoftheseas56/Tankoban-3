# Batch 9 Roadmap: External Subtitles

Planning date: 2026-06-16

Purpose: expand Tankoban 3's subtitle plan beyond the baseline Stremio `subtitles` endpoint. This batch converts Harbor's subtitle search/import system into native C++/Qt6: embedded stream subtitles, installed subtitle addons, external providers, local subtitle files, language ranking, dedupe, and player handoff to Agent 4's libmpv player.

Downloads are deferred to a separate owner plan. This batch does not include video downloads.

## Harbor Diagnosis

Primary Harbor sources:

- `src/lib/subtitles/types.ts`: shared subtitle result/search query shape.
- `src/lib/subtitles/search.ts`: provider orchestration, failure isolation, dedupe, ranking, source interleaving.
- `src/lib/subtitles/providers/addons.ts`: installed Stremio subtitle addon calls.
- `src/lib/subtitles/addon-source.ts`: gather installed subtitle-capable addons.
- `src/lib/subtitles/providers/opensubtitles-v3.ts`: OpenSubtitles V3 via Stremio-compatible endpoints.
- `src/lib/subtitles/providers/wyzie.ts`: Wyzie API search.
- `src/lib/subtitles/language.ts`: language normalization, display names, preferred-language scoring.
- `src/lib/subtitles/parser.ts`: SRT/VTT/ASS/SSA parsing for HTML player path.
- `src/components/player/subtitle-menu/*`: player menu, embedded/external filtering, search tab, import selected subtitle, local file import, delay controls.
- `src/views/settings/player-panel/subtitle-section.tsx`: subtitle appearance controls and ASS override options.
- `src/lib/player/bridge.ts`, `src/lib/player/mpv.ts`, `src/lib/player/html5/bridge.ts`: player track model and external subtitle load path.

Harbor behavior to preserve:

- Player receives embedded subtitles from stream objects before any external search.
- Subtitle menu groups tracks by language and separates embedded/external tracks.
- User can turn subtitles off, choose a track, adjust delay, and search for more.
- Search queries external providers in parallel and tolerates partial provider failure.
- Subtitle results are deduped by normalized language and URL.
- Results are ranked by preferred subtitle languages, stream/release match, hearing-impaired preference, downloads, and source priority.
- Results are interleaved by provider so one source does not flood the list.
- Search can include installed Stremio subtitle addons.
- User can import a chosen subtitle into the player as an external track.
- User can load a local `.srt`, `.ass`, `.ssa`, `.vtt`, or `.sub` file.

Tankoban 3 trims inside this batch:

- No subtitle download-to-disk feature; downloads are deferred.
- No account-authenticated OpenSubtitles workflow unless explicitly accepted later.
- No Jimaku implementation unless a concrete public API path is chosen later.
- No full subtitle style editor unless subtitle styling is explicitly kept. Basic track selection/delay remains in scope.
- No web/HTML player subtitle parser requirement for the main playback path if Agent 4's libmpv handles external subtitle rendering. Keep a parser only if needed for preview/validation.

## Product Scope

Keep:

- Embedded subtitles from selected stream.
- Installed Stremio subtitle addon search.
- OpenSubtitles V3 public Stremio-compatible endpoints.
- Wyzie provider.
- Provider enable/disable settings.
- Preferred subtitle languages.
- Local subtitle file import.
- Subtitle delay.
- Search/refine UI in player subtitle menu.
- Language grouping, source filter, HI/SDH filter, forced-only filter.
- Ranking/deduping/interleaving.
- Player handoff to libmpv: external subtitle URL/path + language/title.

Defer:

- Jimaku provider until API contract is clear.
- Subtitle styling editor.
- Subtitle file download/export.
- Auto-sync/timing correction.
- Hash/size-based perfect matching until the player/local-file layer exposes stable video hash/size.

Cut for this batch:

- Any dependency on TMDB.
- Any dependency on Stremio account sync.
- Downloads-to-disk.

## Qt File Map

Core:

```text
src/core/subtitles/SubtitleModels.h
src/core/subtitles/SubtitleService.h/.cpp
src/core/subtitles/SubtitleProvider.h
src/core/subtitles/StremioSubtitleProvider.h/.cpp
src/core/subtitles/OpenSubtitlesV3Provider.h/.cpp
src/core/subtitles/WyzieSubtitleProvider.h/.cpp
src/core/subtitles/SubtitleRanker.h/.cpp
src/core/subtitles/SubtitleLanguage.h/.cpp
src/core/subtitles/SubtitleFileImport.h/.cpp
src/core/subtitles/SubtitleParser.h/.cpp        # optional, only if validation/preview needs it
```

UI:

```text
src/ui/player/SubtitleMenu.h/.cpp
src/ui/player/SubtitleSearchPanel.h/.cpp
src/ui/player/SubtitleTrackList.h/.cpp
src/ui/player/SubtitleDelayControl.h/.cpp
src/ui/widgets/subtitles/SubtitleResultRow.h/.cpp
src/ui/widgets/subtitles/SubtitleLanguageTabs.h/.cpp
```

Settings:

```text
src/ui/pages/SettingsPage.h/.cpp
src/ui/settings/SubtitleSettingsSection.h/.cpp
```

Player seam:

```text
src/ui/player/VideoPlayerPage.h/.cpp
src/core/StreamModels.h
```

## Data Models

```cpp
enum class SubtitleSource {
    Embedded,
    StreamObject,
    StremioAddon,
    OpenSubtitles,
    Wyzie,
    Jimaku,
    LocalFile
};

enum class SubtitleFormat {
    Unknown,
    Srt,
    Vtt,
    Ass,
    Ssa,
    Sub
};

struct SubtitleResult {
    QString id;
    QUrl url;
    QString localPath;
    QString language;
    QString languageName;
    QString title;
    SubtitleSource source = SubtitleSource::StremioAddon;
    SubtitleFormat format = SubtitleFormat::Unknown;
    QString encoding;
    double fps = 0.0;
    bool hearingImpaired = false;
    bool forced = false;
    QString release;
    int downloads = 0;
    QString providerName;
};

struct SubtitleSearchQuery {
    QString imdbId;
    QString stremioId;
    QString type;          // movie | series
    QString title;
    int season = 0;
    int episode = 0;
    QStringList preferredLanguages;
    QString videoHash;
    qint64 videoSize = 0;
    QString filename;
};

struct SubtitleStreamHints {
    QString release;
    QString source;
    QString resolution;
    bool preferHearingImpaired = false;
};

struct SubtitleProviderOptions {
    bool useInstalledAddons = true;
    bool useOpenSubtitles = true;
    bool useWyzie = true;
    bool useJimaku = false;
};

struct SubtitleTrack {
    QString id;
    QString language;
    QString title;
    QUrl url;
    QString localPath;
    SubtitleSource source = SubtitleSource::Embedded;
    SubtitleFormat format = SubtitleFormat::Unknown;
    bool external = false;
    bool hearingImpaired = false;
    bool forced = false;
};
```

Extend existing `StreamSubtitle` in `StreamModels.h` instead of creating a parallel player-only type:

```cpp
struct StreamSubtitle {
    QString id;
    QString language;
    QString title;
    QUrl url;
    QString localPath;
    QString mimeType;
    SubtitleFormat format = SubtitleFormat::Unknown;
    SubtitleSource source = SubtitleSource::StreamObject;
};
```

## Service Interfaces

### SubtitleProvider

```cpp
class SubtitleProvider : public QObject {
    Q_OBJECT
public:
    explicit SubtitleProvider(QObject* parent = nullptr);
    virtual QString providerId() const = 0;
    virtual QString displayName() const = 0;
    virtual void search(const SubtitleSearchQuery& query) = 0;

signals:
    void resultsReady(QString providerId, QVector<SubtitleResult> results);
    void failed(QString providerId, QString message);
};
```

### SubtitleService

Owns provider orchestration and exposes the app-facing API.

```cpp
class SubtitleService : public QObject {
    Q_OBJECT
public:
    explicit SubtitleService(
        AddonRegistry* registry,
        AppRepository* repo,
        QObject* parent = nullptr);

    QVector<SubtitleTrack> embeddedTracksFromStream(const Stream& stream) const;
    void search(const SubtitleSearchQuery& query, const SubtitleStreamHints& hints = {});
    void importLocalFile(QString path);
    void addExternalSubtitle(SubtitleResult result);

signals:
    void searchStarted(QString requestKey);
    void providerResults(QString requestKey, QString providerId, QVector<SubtitleResult> results);
    void searchFinished(QString requestKey, QVector<SubtitleResult> rankedResults);
    void externalSubtitleReady(SubtitleTrack track);
    void importFailed(QString message);
};
```

Rules:

- Always include embedded/stream-object subtitles in the initial `PlayerRequest`.
- Search is an explicit player-menu action, not required before playback starts.
- Provider failures should log and continue. A failed provider must not blank successful results.
- `AddonRegistry` supplies enabled installed addons that expose `subtitles`.
- The service dedupes/ranks all provider results before emitting final results.

### SubtitleRanker

Responsibilities:

- Normalize language.
- Deduplicate by `language + url` or `language + localPath`.
- Score preferred languages.
- Score stream match:
  - release group match
  - source match: BluRay/WebDL/HDTV/DVD
  - resolution match
  - hearing-impaired preference
- Prefer provider order:
  - installed addon
  - OpenSubtitles/Wyzie
  - Jimaku
  - local/imported stays where user added it
- Interleave provider buckets after per-provider sorting.

## Provider Contracts

### StremioSubtitleProvider

Endpoint:

```text
{base}/subtitles/{type}/{id}.json
{base}/subtitles/{type}/{id}/videoHash=...&videoSize=...&filename=....json
```

Rules:

- Build id from Stremio id or IMDb id:
  - movie: `tt1234567`
  - episode: `tt1234567:season:episode`
- Query only addons whose manifest accepts `subtitles` for the type/id.
- Normalize raw fields:
  - `url`
  - `lang`
  - `id`
  - `m`
  - `SubFormat`

### OpenSubtitlesV3Provider

Endpoints, in fallback/merge order:

```text
https://opensubtitles.stremio.homes/subtitles/{type}/{id}.json
https://opensubtitles-v3.strem.io/subtitles/{type}/{id}.json
https://opensubtitles.strem.io/subtitles/{type}/{id}.json
```

Rules:

- Requires IMDb id.
- Episode id becomes `tt...:season:episode`.
- Query all endpoints in parallel.
- Dedupe by `lang + url`.
- No user account/API key in this roadmap.

### WyzieSubtitleProvider

Endpoint:

```text
https://sub.wyzie.io/search?id={imdbId}&season={s}&episode={e}&source=all&language=en,es
```

Fallback:

- If no IMDb id exists, use title query only when reliable enough.

Fields:

- `url`
- `display`
- `language`
- `format`
- `encoding`
- `isHearingImpaired` / `hi`
- `release`
- `fps`
- `downloads`

### JimakuProvider

Defer until explicitly accepted and a stable API is chosen.

## Player Integration

`PlayerRequest` should carry initial subtitles:

```cpp
struct PlayerRequest {
    QUrl url;
    QMap<QString, QString> headers;
    QVector<StreamSubtitle> subtitles;
    MetaDetail meta;
    std::optional<Video> episode;
    Stream source;
};
```

Agent 4 player contract needs these methods or equivalents:

```cpp
class VideoPlayerPage : public QWidget {
    Q_OBJECT
public:
    void load(PlayerRequest request);
    void addSubtitleTrack(SubtitleTrack track);
    void selectSubtitle(QString trackId);
    void setSubtitleDelayMs(int delayMs);

signals:
    void subtitleTracksChanged(QVector<SubtitleTrack> tracks);
    void selectedSubtitleChanged(QString trackId);
};
```

libmpv mapping:

- External URL/path: `sub-add`.
- Select: set `sid`.
- Delay: set `sub-delay`.
- Embedded tracks come from mpv `track-list`.

If Agent 4 exposes different names, keep this roadmap as the app-side semantic contract and adapt in `VideoPlayerPage`.

## UI Composition

### SubtitleMenu

Native player overlay menu:

- Header: `Subtitles` + count + close button.
- Left column:
  - Off/On control.
  - Language tabs.
  - All languages tab when multiple languages exist.
- Main panel:
  - Source filter: All / Embedded / External.
  - HI/SDH toggle.
  - Forced-only toggle.
  - Track rows with language, source, codec/format, forced/default/HI tags.
- Footer:
  - Find more subtitles.
  - Load file.
  - Delay control.

### SubtitleSearchPanel

Composition:

- Search/refine input.
- Search button.
- Provider result groups by language.
- Result rows:
  - title/release
  - source
  - format
  - downloads
  - HI/SDH tag
  - forced tag
  - Add button

Behavior:

- If metadata has IMDb id, run an initial search automatically after menu opens and installed subtitle addons are loaded.
- If no IMDb id exists, require manual title query.
- Adding a result calls `SubtitleService::addExternalSubtitle`, then `VideoPlayerPage::addSubtitleTrack`.

### Local Subtitle Import

Use `QFileDialog::getOpenFileName` with:

- `.srt`
- `.ass`
- `.ssa`
- `.vtt`
- `.sub`

After import:

- Add external subtitle track with local path.
- Select it immediately.
- Show short imported confirmation.

## Settings

QSettings:

```text
settings/player/preferredSubtitleLanguages
settings/subtitles/provider/addons
settings/subtitles/provider/openSubtitles
settings/subtitles/provider/wyzie
settings/subtitles/provider/jimaku
settings/subtitles/startOff
settings/subtitles/preferEmbedded
settings/subtitles/preferForcedWhenAudioMatches
settings/subtitles/autoSelectBlocklist
```

Optional styling settings if kept later:

```text
settings/subtitles/style
settings/subtitles/fontFamily
settings/subtitles/fontSize
settings/subtitles/fontColor
settings/subtitles/borderColor
settings/subtitles/borderSize
settings/subtitles/marginY
settings/subtitles/alignX
settings/subtitles/assOverrideMode
```

## Phase Plan

### Phase 9.1: Subtitle Models and PlayerRequest Handoff

Goal: make subtitle data first-class before adding providers.

Deliverables:

- `SubtitleModels.h`.
- Extend `StreamSubtitle`.
- Extend `PlayerRequest`.
- `VideoPlayerPage` semantic methods for add/select/delay.

Smoke:

- A stream with embedded `subtitles` reaches `PlayerRequest`.
- Player placeholder displays subtitle count.
- No provider search required.

### Phase 9.2: Stremio Subtitle Addons

Goal: installed subtitle addons work through the Stremio protocol.

Deliverables:

- `StremioSubtitleProvider`.
- `SubtitleService` provider orchestration with only addon provider enabled.
- Manifest capability filter using `AddonRegistry`.
- Subtitle id builder for movie/episode.

Smoke:

- Install a subtitle addon.
- Open player subtitle menu.
- Search finds addon subtitles for an IMDb-backed movie/episode.
- Selecting a result adds it to the player.

### Phase 9.3: Language Normalization and Ranking

Goal: useful ordering instead of raw provider dump.

Deliverables:

- `SubtitleLanguage`.
- `SubtitleRanker`.
- Preferred language settings.
- Dedupe by language + URL/path.
- Stream match hints from `StreamScorer`/selected `Stream`.

Smoke:

- Preferred language appears first.
- Duplicate provider URLs collapse.
- Release/source match ranks above unrelated subtitles.

### Phase 9.4: OpenSubtitles V3 Provider

Goal: public fallback subtitles without user API keys.

Deliverables:

- `OpenSubtitlesV3Provider`.
- Parallel endpoint querying.
- Result merge/dedupe.
- Provider enable/disable setting.

Smoke:

- Search works with no subtitle addon installed.
- Endpoint failure still returns results from other endpoints.
- Turning provider off removes those results.

### Phase 9.5: Wyzie Provider

Goal: broaden coverage for foreign-language and release-matched subtitles.

Deliverables:

- `WyzieSubtitleProvider`.
- Preferred-language query parameter.
- HI/SDH, release, format, downloads metadata.
- Provider enable/disable setting.

Smoke:

- Wyzie results appear when enabled.
- HI/SDH filter hides hearing-impaired results.
- Downloads/release labels display.

### Phase 9.6: Native Subtitle Menu

Goal: Harbor-faithful player subtitle UX.

Deliverables:

- `SubtitleMenu`.
- `SubtitleTrackList`.
- `SubtitleSearchPanel`.
- `SubtitleDelayControl`.
- Embedded/external/all filters.
- Language grouping.
- Forced-only and HI toggles.

Smoke:

- Open subtitle menu during playback.
- Toggle subtitles off.
- Pick embedded track.
- Search and add external track.
- Adjust subtitle delay.

### Phase 9.7: Local Subtitle File Import

Goal: support user-provided subtitle files.

Deliverables:

- `SubtitleFileImport`.
- File picker integration.
- Validate extension.
- Add/select local track.

Smoke:

- Load `.srt` file.
- Track appears as local/imported.
- Player selects it immediately.
- Bad file path shows error.

### Phase 9.8: Settings and Polish

Goal: make behavior persistent and resilient.

Deliverables:

- `SubtitleSettingsSection`.
- Provider toggles.
- Preferred subtitle language editor.
- Start-off/prefer-embedded settings.
- Empty/loading/error states.
- Very-new-title message.
- Provider timeout defaults.

Smoke:

- Settings persist after restart.
- Disabled providers stay disabled.
- No subtitles found state is clear.
- Provider timeout does not freeze player UI.

## Dependencies

- Requires `AddonRegistry` from the addon-store batch for installed subtitle addons.
- Requires `StreamService`/`StreamScorer` for selected stream hints.
- Requires Agent 4 player seam for actual libmpv add/select/delay calls.
- Can be developed with a placeholder `VideoPlayerPage` that records added subtitle tracks.

## Acceptance Criteria

Batch 9 is complete when:

- Player receives embedded subtitles from selected streams.
- Player subtitle menu can list embedded and external tracks.
- Installed subtitle addons can be searched.
- OpenSubtitles V3 public endpoints can be searched.
- Wyzie can be searched.
- Results are deduped and ranked by preferred language and stream match.
- User can add an external subtitle to the current player session.
- User can import a local subtitle file.
- User can turn subtitles off, select a track, and adjust delay.
- Provider toggles and preferred languages persist.
- No TMDB/API-key, Stremio account-sync, or downloads-to-disk dependency is introduced.

