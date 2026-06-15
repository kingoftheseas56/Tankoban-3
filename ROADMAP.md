# Tankoban 3 Groundwork Roadmap

Planning date: 2026-06-16

Tankoban 3 is a fresh native C++/Qt6 app that recreates Harbor's video mode. Harbor remains the UX spec. Stremio's addon protocol remains the content and playback data contract. This roadmap covers the rest of the build after the current app shell, Harbor dark theme, Cinemeta catalog seed, Home page rows, lazy poster loading, and sidebar navigation.

## Scope Boundaries

In scope:

- Stremio-compatible addon install, persistence, catalog aggregation, detail metadata, stream resolution, and subtitles.
- Harbor-faithful screens: Detail, Play Picker, Search, Addons, Library, Settings, Discover, Movies, Shows, Anime.
- The navigation path: Home or row page -> Detail -> Play Picker -> Player.
- Home hero carousel as a required Harbor identity feature, not polish.
- Advanced stream scoring as a required picker feature: parse, classify, and rank streams by source quality, resolution, HDR/DV, audio, codec, size, language, addon priority, and suspicious/CAM markers.
- Local files as a supported Library source after the Stremio playback spine is working.
- Player integration seam to Agent 4's libmpv `VideoPlayer` once available.
- Local persistence for installed addons, watchlist, continue-watching, playback history, local files, playback settings, and source settings.

Out of scope:

- Live TV/IPTV, Playlists, Multiview, Calendar, award browsers, Browse by Award, award pages, award tiles, Critics Pick, Discovery Queue, Watch Together, multi-profile, parental controls, Trakt, AniList, Simkl, Discord/webhooks, theme presets, relay/account/cloud sync.
- People search for v1. Search should cover movies, shows, anime, addons, and local library first.
- Home row customization for v1. Rows are fixed and curated by the app.
- Full addon app store/community marketplace. This is kept scope, not a trim candidate. Paste-manifest install is an additional path, not a replacement.

## Locked Product Decisions

- Keep: Home hero carousel.
- Keep: local files in Library, after the Stremio addon playback spine works.
- Keep: advanced stream scoring. A picker that merely groups by resolution is not sufficient.
- Cut: Browse by Award and all award-specific pages/tiles/rails.
- Cut: Critics Pick, Discovery Queue, people search, and Home row customization for v1.
- Keep: Addons app store/community marketplace with Discover, Browse, Installed, categories, addon detail, and manifest paste install.
- Preserve: Harbor's layout language, navigation behavior, dark visual system, and Stremio-like playback flow.
- Trim: editorial/external-account/social features that do not serve the core video playback path.

## References Consulted

Local Tankoban 3:

- `src/ui/MainWindow.*`: shell and top-level `QStackedWidget`.
- `src/ui/Sidebar.*`: Harbor-style sidebar route ids.
- `src/ui/HomePage.*`, `CatalogRow.*`, `PosterCard.*`: current row/card rendering and lazy image loading.
- `src/core/AddonClient.*`, `MetaItem.h`: current Stremio catalog seed.
- `src/ui/Theme.h`: current Harbor dark QSS tokens.

Harbor spec:

- `src/App.tsx`: provider shell, frame stack, chrome hiding for picker/player, route mounting.
- `src/lib/view.tsx`: Harbor push-stack model, `Frame`, `openMeta`, `openPicker`, `openPlayer`, back/exit behavior.
- `src/lib/cinemeta.ts`: baseline `Meta`, Cinemeta catalog/meta calls.
- `src/lib/addons.ts`, `src/lib/addon-store.ts`: installed addon model, manifest fetch, catalog/meta aggregation, addon capability checks.
- `src/views/detail.tsx`: backdrop hero, title plate, pills, action bar, episodes, synopsis.
- `src/views/play-picker.tsx`, `src/views/play-picker/*`: stream list, quality tiers, source drawer, empty states.
- `src/lib/streams/types.ts`, `src/lib/streams/addons.ts`, `src/lib/streams/stream-ids.ts`, `src/lib/streams/resolve.ts`: stream data surface and addon stream endpoint usage.
- `src/views/addons.tsx`, `src/views/library.tsx`, `src/views/settings.tsx`, `src/components/search/search-overlay.tsx`.
- `src/views/home.tsx`, `src/views/home/home-rows.ts`, `src/views/movies.tsx`, `src/views/shows.tsx`, `src/views/anime.tsx`, `src/views/discover.tsx`.

Protocol references:

- Stremio addon protocol docs: https://github.com/Stremio/stremio-addon-sdk/blob/master/docs/protocol.md
- Stremio addon SDK repository: https://github.com/Stremio/stremio-addon-sdk

## Stremio Protocol Baseline

The native addon engine should treat Stremio addon URLs as the stable contract:

- Manifest: `{base}/manifest.json`
- Catalog: `{base}/catalog/{type}/{id}.json` plus extra path segments such as `/skip=30`, `/genre=Action` when supported.
- Meta: `{base}/meta/{type}/{id}.json`
- Stream: `{base}/stream/{type}/{id}.json`
- Subtitles: `{base}/subtitles/{type}/{id}/{videoHash}.json` when an addon exposes subtitle resources. Also accept subtitles embedded on stream objects.

Manifest capability rules:

- `resources` may be strings, or objects with `name`, `types`, and `idPrefixes`.
- `types` and `idPrefixes` gate whether an addon should be queried for a resource.
- `catalogs` define row candidates. Catalogs with required `extra` are not Home rows until the UI can provide that extra.
- `behaviorHints.configurable` and `configurationRequired` route the user to addon details/configuration instead of blind install.

## Target Architecture

### Core Modules

Keep the current `src/core/AddonClient` as the low-level HTTP client, then layer services above it. Do not let UI pages construct raw URLs directly.

Target files:

```text
src/core/
  AddonClient.h/.cpp          # HTTP transport, JSON fetch, timeout, user agent
  AddonModels.h               # Manifest, CatalogDef, AddonInstall, AddonCapability
  MetaModels.h                # MetaPreview, MetaDetail, Video
  StreamModels.h              # Stream, StreamSubtitle, StreamRequest, ResolvedStream
  AddonRegistry.h/.cpp        # install/persist/order/enable addons
  CatalogService.h/.cpp       # aggregate catalogs into rows
  MetaService.h/.cpp          # fetch detail metadata
  StreamService.h/.cpp        # fetch streams across addons and resolve direct URLs
  StreamScorer.h/.cpp         # advanced parsing/ranking for picker ordering
  SubtitleService.h/.cpp      # fetch subtitles and normalize embedded subtitles
  AppRepository.h/.cpp        # QSettings + SQLite facade
  LocalFileService.h/.cpp     # index and play local video files after the core spine lands
  ImageCache.h/.cpp           # later extraction from PosterCard shared NAM/cache
```

### UI Modules

Target files:

```text
src/ui/
  AppContext.h/.cpp           # owns services and repositories
  AppNavigator.h/.cpp         # Harbor-like push stack over Qt widgets
  RouteTypes.h                # ShellRoute, Frame, EpisodeRef, PlayerRequest
  MainWindow.h/.cpp           # shell composition only, no data fetching
  Sidebar.h/.cpp
  TopBar.h/.cpp               # search button/back affordance if needed

  pages/
    HomePage.h/.cpp
    RowPage.h/.cpp            # Movies/Shows/Anime/Discover reuse row pattern
    DetailPage.h/.cpp
    PlayPickerPage.h/.cpp
    SearchOverlay.h/.cpp
    AddonsPage.h/.cpp
    LibraryPage.h/.cpp
    SettingsPage.h/.cpp
    DownloadsPage.h/.cpp      # placeholder unless downloads are later in scope

  widgets/
    CatalogRow.h/.cpp
    PosterCard.h/.cpp
    HeroCarousel.h/.cpp       # required Home feature
    DetailHero.h/.cpp
    EpisodeList.h/.cpp
    StreamList.h/.cpp
    QualityTierStrip.h/.cpp
    AddonCard.h/.cpp
    SegmentedTabs.h/.cpp
    EmptyState.h/.cpp
```

## Data Models

Use value structs in headers, pass by value or `QVector<T>`, and keep JSON parsing centralized in core mappers.

### Addon Models

```cpp
struct AddonResource {
    QString name;                 // catalog, meta, stream, subtitles
    QStringList types;
    QStringList idPrefixes;
    bool wildcard = false;         // resource was a plain string
};

struct CatalogExtra {
    QString name;
    bool required = false;
    QVector<QString> options;
};

struct CatalogDef {
    QString id;
    QString type;                  // movie, series, anime, tv, channel
    QString name;
    QVector<CatalogExtra> extra;
};

struct ManifestBehaviorHints {
    bool adult = false;
    bool p2p = false;
    bool configurable = false;
    bool configurationRequired = false;
};

struct Manifest {
    QString id;
    QString name;
    QString version;
    QString description;
    QString logo;
    QString background;
    QVector<AddonResource> resources;
    QVector<CatalogDef> catalogs;
    QStringList types;
    QStringList idPrefixes;
    ManifestBehaviorHints behaviorHints;
};

struct InstalledAddon {
    QString id;
    QUrl manifestUrl;
    QUrl baseUrl;
    Manifest manifest;
    QDateTime installedAt;
    bool enabled = true;
    int order = 0;
};
```

### Metadata Models

```cpp
struct AddonOrigin {
    QString id;
    QString name;
    QString logo;
    QUrl baseUrl;
};

struct MetaPreview {
    QString id;
    QString type;                  // movie, series, anime, tv, channel, other
    QString name;
    QUrl poster;
    QUrl background;
    QString releaseInfo;
    QString imdbRating;
    AddonOrigin origin;
};

struct Video {
    QString id;                    // Stremio video id, often tt...:season:episode
    int season = 0;
    int episode = 0;
    int number = 0;
    QString name;
    QString title;
    QString overview;
    QUrl thumbnail;
    QDateTime released;
};

struct MetaDetail : MetaPreview {
    QUrl logo;
    QString description;
    QString runtime;
    QStringList genres;
    QStringList directors;
    QStringList cast;
    QString defaultVideoId;
    QVector<Video> videos;
};
```

### Stream Models

```cpp
struct StreamSubtitle {
    QString id;
    QUrl url;
    QString language;
};

struct StreamBehaviorHints {
    QString bingeGroup;
    QString videoHash;
    qint64 videoSize = 0;
    QString filename;
    bool notWebReady = false;
    QHash<QString, QString> requestHeaders;
};

enum class QualityTier {
    FourK_DV,
    FourK_HDR,
    FourK,
    FullHD_HDR,
    FullHD,
    HD,
    SD,
    Rough,
    Unknown
};

struct Stream {
    QString addonId;
    QString addonName;
    QUrl addonUrl;
    QString name;
    QString title;
    QString description;
    QUrl url;
    QString ytId;
    QString externalUrl;
    QString infoHash;
    int fileIdx = -1;
    QStringList sources;
    QVector<StreamSubtitle> subtitles;
    StreamBehaviorHints behaviorHints;

    QString parsedTitle;
    QualityTier tier = QualityTier::Unknown;
    QString resolution;
    QString sourceKind;
    qint64 size = 0;
    int seeders = -1;
    int addonOrder = 0;
};

struct StreamRequest {
    QString type;                  // movie or series for first pass
    QStringList candidateIds;       // e.g. tt123, tt123:1:2, kitsu:...
};

struct ResolvedStream {
    QUrl url;
    QString title;
    QString subtitle;
    QVector<StreamSubtitle> subtitles;
    QHash<QString, QString> requestHeaders;
    bool notWebReady = false;
    Stream source;
};
```

## Service Interfaces

### AddonClient

Low-level network utility. It should not know persistence or UI.

Responsibilities:

- GET JSON with timeout and cancellation token/request id.
- Normalize base URLs and manifest URLs.
- Parse protocol response envelopes: `{ metas: [...] }`, `{ meta: {...} }`, `{ streams: [...] }`, `{ subtitles: [...] }`.
- Emit one signal per request, never block UI.

Signals:

```cpp
void jsonReady(QString requestId, QJsonObject object);
void requestFailed(QString requestId, QString error);
```

Current `fetchCatalog` can remain temporarily, but phase 1 should migrate callers to typed service methods.

### AddonRegistry

Owns installed addons and manifest refresh.

Public surface:

```cpp
class AddonRegistry : public QObject {
    Q_OBJECT
public:
    explicit AddonRegistry(AppRepository* repo, AddonClient* client, QObject* parent = nullptr);

    QVector<InstalledAddon> installedAddons() const;
    QVector<InstalledAddon> enabledAddons() const;
    QVector<InstalledAddon> addonsForResource(QString resource, QString type, QString id) const;

    void load();
    void installFromManifestUrl(QUrl manifestUrl);
    void uninstall(QString addonId, QUrl manifestUrl = {});
    void setEnabled(QUrl manifestUrl, bool enabled);
    void setOrder(QVector<QUrl> manifestUrls);
    void refreshManifests();

signals:
    void addonsChanged();
    void installSucceeded(InstalledAddon addon, bool replaced);
    void installFailed(QUrl manifestUrl, QString error);
};
```

Install rules:

- Accept `https://.../manifest.json` and `stremio://.../manifest.json`.
- If the user pastes a base URL, append `/manifest.json`.
- Validate at least `id` and `name`.
- Persist slim manifest fields only.
- Preserve order. Catalog and stream services query in registry order.
- Seed Cinemeta as a built-in readonly addon or a registry entry with `enabled=true`, `order=0`.

### CatalogService

Aggregates catalog rows across all enabled addons.

Public surface:

```cpp
class CatalogService : public QObject {
    Q_OBJECT
public:
    void loadHomeRows();
    void loadRowsForRoute(QString routeId);     // movies, shows, anime, discover
    void fetchCatalogPage(QString rowKey, int skip);

signals:
    void rowsReady(QString routeId, QVector<CatalogRowModel> rows);
    void rowPageReady(QString rowKey, QVector<MetaPreview> items, bool hasMore);
    void rowFailed(QString rowKey, QString error);
};
```

Row behavior:

- Built-in Home first uses Cinemeta rows already present: Popular Movies, Popular Series.
- Next grow to Harbor's Cinemeta fallback rows from `home-rows.ts`: top movies, popular movies, trending series, genre rows.
- Then merge addon catalog rows from installed addons, deduping by normalized row name plus type like Harbor.
- Filter out cut scope types (`tv`, `channel`) from Home and video mode pages.
- Only include catalogs without required extras until filter UI exists.
- Cap first-page rows at 30 items to match current row behavior.

### MetaService

Fetches detail metadata and resolves episode lists.

Public surface:

```cpp
class MetaService : public QObject {
    Q_OBJECT
public:
    void fetchDetail(MetaPreview preview);

signals:
    void detailReady(QString metaId, MetaDetail detail);
    void detailFailed(QString metaId, QString error);
};
```

Rules:

- If `preview.origin.baseUrl` exists and the meta is addon-native, call that addon's meta endpoint first.
- For IMDB/Cinemeta ids, call Cinemeta first: `/meta/{type}/{id}.json`.
- Preserve preview fields as fallback while loading.
- For series, use `meta.videos` as the first-pass episode source. Do not plan TMDB/TVDB enrichment until the Stremio path is complete.

### StreamService

Fetches and ranks playable streams from enabled addons.

Public surface:

```cpp
class StreamService : public QObject {
    Q_OBJECT
public:
    QStringList buildStreamIds(MetaDetail meta, std::optional<Video> episode) const;
    void fetchStreams(MetaDetail meta, std::optional<Video> episode);
    void resolve(Stream stream);

signals:
    void streamsPartial(QString requestKey, QVector<Stream> streams);
    void streamsReady(QString requestKey, QVector<Stream> streams);
    void streamsFailed(QString requestKey, QString error);
    void resolved(QString requestKey, ResolvedStream stream);
    void resolveFailed(QString requestKey, QString error);
};
```

First-pass stream rules:

- Build candidate ids like Harbor: movie `tt123`, series `tt123:season:episode`, default video id, explicit video id, and addon-native ids.
- Query addons that expose `stream` and accept the requested type/id.
- Timeout fast addons around 8 seconds, known slow stream addons around 22 seconds.
- Emit partial results so Play Picker can render while slow addons continue.
- Normalize direct streams with `url` first.
- Display but do not auto-play unsupported `externalUrl`, `ytId`, `nzbUrl`, and bare `infoHash` until a torrent/debrid/local-engine path exists.
- Run every stream through advanced scoring before display. The first implementation does not need debrid integration, but it must parse and rank source quality, resolution, HDR/DV, codec, audio, file size, language markers, release/source terms, addon order, and suspicious/CAM/TS markers.
- Group by quality tiers after scoring, so a fake-looking `4K CAM` does not outrank a clean `1080p WEB-DL`.

Resolve rules:

- If `stream.url` is present, return it as `ResolvedStream`.
- Preserve `behaviorHints.proxyHeaders.request` and `behaviorHints.headers`.
- Attach embedded subtitles from the stream object.
- If only `infoHash` exists, return a clear unsupported state in the picker unless a later torrent/debrid phase is explicitly added.

### SubtitleService

Public surface:

```cpp
class SubtitleService : public QObject {
    Q_OBJECT
public:
    void fetchSubtitles(MetaDetail meta, std::optional<Video> episode, Stream stream);

signals:
    void subtitlesReady(QString requestKey, QVector<StreamSubtitle> subtitles);
    void subtitlesFailed(QString requestKey, QString error);
};
```

Rules:

- Start with embedded `stream.subtitles`.
- Add protocol subtitle endpoint support after player handoff works.
- Query addons exposing `subtitles` only when a video hash/default id exists.

## Navigation Architecture

Harbor uses a push stack over top-level shell views. Tankoban 3 should mirror that with a native `AppNavigator`.

### Route Types

```cpp
enum class ShellRoute {
    Home,
    Discover,
    Movies,
    Shows,
    Anime,
    Library,
    Downloads,
    Addons,
    Settings
};

enum class FrameKind {
    Shell,
    Detail,
    PlayPicker,
    Player,
    AddonDetail
};

struct EpisodeRef {
    int season = 0;
    int episode = 0;
    QString videoId;
    QString name;
    QUrl still;
};

struct Frame {
    FrameKind kind;
    ShellRoute shellRoute;
    MetaPreview preview;
    MetaDetail detail;
    std::optional<EpisodeRef> episode;
    std::optional<ResolvedStream> playerStream;
};
```

### AppNavigator

Public surface:

```cpp
class AppNavigator : public QObject {
    Q_OBJECT
public:
    explicit AppNavigator(QStackedWidget* contentStack, Sidebar* sidebar, AppContext* context);

    void setShellRoute(ShellRoute route);
    void openDetail(MetaPreview preview);
    void openPicker(MetaDetail detail, std::optional<Video> episode = std::nullopt, bool autoPlay = false);
    void openPlayer(ResolvedStream stream);
    void openAddonDetail(QString addonId);
    void goBack();
    void exitPlayback();

signals:
    void routeChanged(Frame frame);
    void chromeHiddenChanged(bool hidden);
};
```

Qt behavior:

- `MainWindow` keeps `[Sidebar | ContentHost]`.
- `ContentHost` remains a `QStackedWidget` or becomes a `QStackedLayout` host controlled by `AppNavigator`.
- Shell routes replace the root frame and clear detail/picker/player stack, matching Harbor's `setView`.
- Detail is pushed over the current shell route and keeps the sidebar visible, matching Harbor where detail is a content overlay but picker/player hide chrome.
- PlayPicker hides sidebar/topbar and shows its own fixed back button.
- Player hides sidebar/topbar completely.
- `goBack()` pops one frame.
- `exitPlayback()` removes player and picker frames and returns to detail.
- `exitPickerToDetail()` returns to existing detail or pushes detail if opened directly.

Handoff:

```text
PosterCard clicked
  -> AppNavigator::openDetail(MetaPreview)
  -> DetailPage calls MetaService::fetchDetail
  -> Play button or episode row
  -> AppNavigator::openPicker(MetaDetail, optional Video)
  -> PlayPickerPage calls StreamService::fetchStreams
  -> user chooses Stream
  -> StreamService::resolve(Stream)
  -> AppNavigator::openPlayer(ResolvedStream)
  -> VideoPlayerPage/Agent 4 contract receives URL + headers + subtitles
```

## Player Integration Seam

Agent 4 owns the libmpv render API and `QOpenGLWidget` player. Tankoban 3 should only define the contract and provide a placeholder until that lands.

Target file:

```text
src/ui/player/VideoPlayerPage.h/.cpp
```

Expected contract:

```cpp
struct PlayerRequest {
    QUrl url;
    QString title;
    QString subtitle;
    QHash<QString, QString> httpHeaders;
    QVector<StreamSubtitle> subtitles;
    MetaDetail meta;
    std::optional<Video> episode;
    Stream source;
    qint64 resumeMs = 0;
};

class VideoPlayerPage : public QWidget {
    Q_OBJECT
public:
    explicit VideoPlayerPage(QWidget* parent = nullptr);
    void load(PlayerRequest request);

signals:
    void backRequested();
    void playbackPositionChanged(QString mediaKey, qint64 positionMs, qint64 durationMs);
    void playbackFinished(QString mediaKey);
};
```

Initial seam:

- `StreamService::resolved` produces `ResolvedStream`.
- `AppNavigator::openPlayer` wraps it into `PlayerRequest`.
- `VideoPlayerPage::load` accepts a direct URL and metadata.
- Continue-watching persistence subscribes to `playbackPositionChanged`.

## Persistence

Use both `QSettings` and SQLite. Add `Qt6::Sql` only when the SQLite phase starts.

### QSettings

Use for small app preferences:

- `settings/player/instantPlay`
- `settings/player/preferredAudioLanguages`
- `settings/player/preferredSubtitleLanguages`
- `settings/sources/showAdultAddons`
- `settings/sources/streamFilterLevel`
- `settings/ui/lastLibraryTab`
- `settings/ui/lastSettingsSection`

### SQLite

Target file:

```text
src/core/AppRepository.h/.cpp
```

Database path:

```text
QStandardPaths::AppDataLocation/tankoban3.sqlite
```

Tables:

```sql
installed_addons(
  manifest_url TEXT PRIMARY KEY,
  addon_id TEXT NOT NULL,
  name TEXT NOT NULL,
  base_url TEXT NOT NULL,
  manifest_json TEXT NOT NULL,
  enabled INTEGER NOT NULL DEFAULT 1,
  display_order INTEGER NOT NULL,
  installed_at TEXT NOT NULL,
  updated_at TEXT NOT NULL
);

watchlist(
  meta_id TEXT PRIMARY KEY,
  type TEXT NOT NULL,
  name TEXT NOT NULL,
  poster TEXT,
  background TEXT,
  added_at TEXT NOT NULL
);

continue_watching(
  media_key TEXT PRIMARY KEY,
  meta_id TEXT NOT NULL,
  type TEXT NOT NULL,
  season INTEGER,
  episode INTEGER,
  video_id TEXT,
  name TEXT NOT NULL,
  poster TEXT,
  position_ms INTEGER NOT NULL,
  duration_ms INTEGER NOT NULL,
  updated_at TEXT NOT NULL
);

playback_history(
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  media_key TEXT NOT NULL,
  meta_id TEXT NOT NULL,
  type TEXT NOT NULL,
  season INTEGER,
  episode INTEGER,
  position_ms INTEGER NOT NULL,
  duration_ms INTEGER NOT NULL,
  watched INTEGER NOT NULL DEFAULT 0,
  played_at TEXT NOT NULL
);
```

Media key:

- Movie: `{metaId}`
- Episode: `{metaId}:{season}:{episode}`
- If Stremio video id exists, store it as secondary lookup.

## Screen Build Order

### 1. Service Groundwork

Goal: move data ownership out of widgets.

Composition:

- `AppContext` creates `AppRepository`, `AddonClient`, `AddonRegistry`, `CatalogService`, `MetaService`, `StreamService`, `SubtitleService`.
- `HomePage` receives `CatalogService*` instead of constructing `AddonClient`.
- `HeroCarousel` is introduced as the required Home hero surface once CatalogService owns Home rows.
- `PosterCard` emits `activated(MetaPreview)`.

Smoke:

- App launches.
- Sidebar still navigates.
- Home still shows Cinemeta Popular Movies and Popular Series.
- Clicking a poster can log or emit navigation without crashing.

### 2. Navigation Stack

Goal: Harbor-style push stack before building heavy pages.

Composition:

- Add `AppNavigator`.
- Wire sidebar `viewActivated` to `AppNavigator::setShellRoute`.
- Wire poster click to `openDetail`.
- Detail, picker, and player can be placeholders at this phase.

Smoke:

- Home -> placeholder Detail -> Back returns to Home.
- Detail -> placeholder Picker hides sidebar.
- Picker -> placeholder Player keeps sidebar hidden.
- Back behavior matches the stack.

### 3. Detail Page

Goal: first real `Home -> Detail` experience.

Composition:

- `DetailPage`
- `DetailHero`
- `EpisodeList`
- action bar: Play, Add to Watchlist, Favorite optional if favorites are retained, trailer/download buttons omitted unless backed by scope.

Faithful Harbor layout:

- Full-width backdrop hero with dark gradients.
- Title/logo or large text title.
- Metadata pill row: year, rating if present, runtime, up to three genres, addon origin chip when addon-native.
- Primary Play button.
- Watchlist button.
- Synopsis below hero.
- Series episode section below synopsis, using Stremio `videos`.

Smoke:

- Open a movie poster: backdrop/title/synopsis render.
- Open a series poster: episodes render from Cinemeta `videos`.
- Play on movie opens picker.
- Episode click opens picker with correct season/episode.

### 4. Addon Registry and Addons Page

Goal: install and manage addon manifests locally.

Composition:

- `AddonsPage` with Harbor tabs:
  - Discover
  - Browse
  - Installed
- `AddManifestBar` for paste-manifest.
- `AddonCard` and `AddonDetailPage`.

Lean adaptation:

- Recreate Harbor's Addons app-store shape in Qt: Discover, Browse, Installed, categories, addon cards, addon detail, install/remove, and manifest paste install.
- Discover should show curated/featured addon rails once the public index source is wired.
- Browse should support public/community addon browsing plus manifest URL search/install.
- Installed shows order, enable/disable, remove.
- Adult toggle only if a manifest has `behaviorHints.adult`.

Smoke:

- Paste a manifest URL.
- Manifest fetch validates id/name and persists.
- Installed tab survives app restart.
- Home rows update after install if the addon exposes catalog resources.

### 5. Catalog Aggregation for Home and Row Pages

Goal: every top sidebar content route becomes useful with the existing row/card pattern.

Composition:

- `RowPage` parameterized by route id.
- `CatalogService::loadRowsForRoute("movies" | "shows" | "anime" | "discover")`.
- Shared `CatalogRow` and `PosterCard`.

Route mapping:

- Home: required hero carousel, continue watching, Cinemeta rows plus addon rows.
- Movies: Cinemeta movie top and genre rows first.
- Shows: Cinemeta series top and genre rows first.
- Anime: addon rows where type/name indicate anime, plus static anime catalogs only if Stremio-compatible sources exist.
- Discover: lean mixed catalog rows only. Do not include Browse by Award, award tiles, Critics Pick, or Discovery Queue.

Smoke:

- Movies/Shows/Anime/Discover are no longer placeholders.
- Rows lazy-load posters.
- Poster click opens Detail from every route.
- App does not query cut-scope `tv` or `channel` catalogs.

### 6. Play Picker and Advanced Stream Scoring

Goal: stream selection UI, advanced stream ranking, and direct URL handoff.

Composition:

- `PlayPickerPage`
- `PickerHeader`
- `QualityTierStrip`
- `StreamList`
- `PickerEmptyState`
- `StreamScorer`

Faithful Harbor layout:

- Full-page overlay, shell chrome hidden.
- Blurred/backdrop image layer.
- Back button fixed top-left.
- Header shows title and selected episode.
- Loading state while addons respond.
- Primary source card or Stremio-style stream list.
- Quality tiers: 4K DV, 4K HDR, 4K, 1080p HDR, 1080p, 720p, SD, Rough.
- Advanced scoring ranks streams inside and across tiers by source quality, codec, audio, size, language, addon priority, and suspicious/fake markers.
- Empty states: no addons, no stream ids, no results, unsupported stream.

Smoke:

- Detail -> Picker fetches streams from installed addons.
- Direct `url` streams appear in ranked order and can be selected.
- Unsupported torrent-only/hash streams appear disabled with reason.
- Back returns to Detail.

### 7. Player Seam

Goal: pass a resolved URL to Agent 4's player contract.

Composition:

- `VideoPlayerPage` placeholder or Agent 4 widget if ready.
- `PlayerRequest` and `ResolvedStream` handoff.
- `LibraryService` or repository writes continue-watching on progress.

Smoke:

- Selecting a direct stream opens Player page.
- Player page receives URL, title, episode info, subtitles, and headers.
- Back exits to Picker or Detail according to navigator rule.
- Resume row appears after playback progress is recorded.

### 8. Global Search

Goal: Harbor-style modal overlay.

Composition:

- `SearchOverlay`
- `SearchService` or `CatalogService::search(query)`.
- Topbar or shortcut opens overlay.

First-pass sources:

- Cinemeta search if available through catalog extras or a direct supported catalog.
- Installed addon catalogs with search extra only where the manifest advertises it.
- Installed addons by manifest name.
- Local watchlist/history.

Faithful Harbor behavior:

- Fullscreen modal.
- Search input focused.
- Esc closes.
- Top match first.
- Movies and Series sections.
- Addon hits.
- Empty state with no results.

Smoke:

- Ctrl+F or topbar search opens overlay.
- Typing a title shows results.
- Enter or click top match opens Detail.
- Esc closes without changing route.

### 9. Library

Goal: local watchlist, continue-watching, playback history, and local files.

Composition:

- `LibraryPage`
- tabs: Watchlist, History, Local Files.
- `ContinueCard`
- `LocalFileService`

Lean adaptation:

- Omit Trakt/AniList/Simkl tabs.
- Watchlist is local SQLite.
- History and continue-watching come from player progress.
- Local files are indexed locally and route through the same Detail/Player seam where practical.
- Continue-watching row appears on Home once repository has entries.

Smoke:

- Add to Watchlist from Detail.
- Library Watchlist shows item and opens Detail.
- Progress writes show in Continue Watching.
- Local file can be added, listed, and sent to player.
- Removing from Watchlist persists.

### 10. Settings

Goal: source and player settings only.

Composition:

- `SettingsPage`
- left nav, content panel like Harbor.
- Sections:
  - Sources: installed addons shortcut, adult toggle if retained, source timeout defaults.
  - Player: instant play, preferred audio/subtitle language, hardware decode once Agent 4 exposes it.
  - Library: clear history, clear poster cache, database path.
  - Advanced: diagnostics/log export only if needed.

Cut:

- Account, Trakt, AniList, Simkl, Relay, theme studio, webhooks, parental controls.

Smoke:

- Changing instant play persists through restart.
- Preferred language settings persist.
- Clear history removes Library history and Home continue-watching rows.

### 11. Subtitles

Goal: player receives subtitles from stream objects and subtitle addons.

Composition:

- `SubtitleService`
- player subtitle menu once Agent 4 exposes it.

Order:

1. Embedded stream subtitles.
2. Protocol subtitle addon endpoint.
3. Preferred subtitle language from Settings ranks first.

Smoke:

- Stream with embedded subtitles sends them in `PlayerRequest`.
- Subtitle addon response is merged without duplicates.
- Preferred language appears first.

### 12. Polish and Parity Pass

Goal: tighten Harbor fidelity after the full spine works.

Tasks:

- Row scroll memory.
- Back-to-top affordance.
- Better empty/loading states.
- Addon detail page refinements.
- Stream parser refinements beyond the required scorer: better language detection, release group normalization, duplicate clustering, and later debrid/cache signals.
- Keyboard shortcuts: Esc/back, search, space/play handled in player.
- Error surfaces for addon timeouts and malformed manifests.

Smoke:

- Fresh install path works from no addons to playback.
- Restart preserves installed addons, settings, watchlist, progress.
- Every sidebar route can open Detail.
- Detail -> Picker -> Player works with a direct stream addon.

## Dependencies by Phase

```text
1 Service Groundwork
  -> 2 Navigation Stack
    -> 3 Detail Page
      -> 6 Play Picker
        -> 7 Player Seam
          -> 9 Library continue-watching

4 Addon Registry
  -> 5 Catalog Aggregation
  -> 6 Play Picker
  -> 11 Subtitles

5 Catalog Aggregation
  -> 8 Search

9 Library
  -> Home continue-watching row
  -> Settings clear history
```

## Build Verification Policy

Each phase should end with:

- `build.bat`
- manual smoke against `out/Tankoban3.exe`
- no unrelated source reshuffles
- one feature slice per commit

The app should remain useful at every step. Avoid building a large invisible backend before the user can click through a new slice.

## Next Implementation Slices

These are the first six implementation slices after this roadmap. They are designed to preserve a faithful, least-compromise Qt recreation of Harbor while keeping every step buildable and smoke-able. Implement them in order. Do not combine slices unless the earlier slice is already merged and green.

### Slice 1: AppContext, Typed Models, and CatalogService Wrapper

Purpose:

- Move the current working Cinemeta Home behavior behind the service architecture the rest of the app will use.
- Keep the visible UI unchanged.
- Establish the data model names that Detail, Picker, Addons, Search, and Library will reuse.

Harbor references:

- `src/lib/cinemeta.ts`
- `src/lib/addons.ts`
- `src/views/home/home-rows.ts`
- `src/views/home.tsx`

Tankoban 3 files to add:

```text
src/core/AddonModels.h
src/core/MetaModels.h
src/core/StreamModels.h
src/core/CatalogService.h
src/core/CatalogService.cpp
src/ui/AppContext.h
src/ui/AppContext.cpp
```

Tankoban 3 files to change:

```text
CMakeLists.txt
src/main.cpp
src/ui/MainWindow.h
src/ui/MainWindow.cpp
src/ui/HomePage.h
src/ui/HomePage.cpp
src/ui/CatalogRow.h
src/ui/CatalogRow.cpp
src/ui/PosterCard.h
src/ui/PosterCard.cpp
src/core/AddonClient.h
src/core/AddonClient.cpp
src/core/MetaItem.h
```

Exact work:

- Introduce `MetaPreview` as the replacement for the current `MetaItem`.
- Keep `MetaItem.h` temporarily as a compatibility include or rename in one controlled commit.
- Add `CatalogRowModel`:

```cpp
struct CatalogRowModel {
    QString key;
    QString title;
    QString type;
    QVector<MetaPreview> items;
    bool hasMore = false;
};
```

- Add `AppContext` as a QObject owned by `main.cpp` or `MainWindow`.
- `AppContext` owns one `AddonClient` and one `CatalogService`.
- `MainWindow` passes `AppContext*` to pages.
- `HomePage` calls `CatalogService::loadHomeRows()` instead of constructing `AddonClient`.
- `CatalogService` initially wraps the exact two existing Cinemeta calls:
  - `https://v3-cinemeta.strem.io/catalog/movie/top.json`
  - `https://v3-cinemeta.strem.io/catalog/series/top.json`
- Preserve the existing poster upgrade behavior from `AddonClient`.
- `PosterCard` stores the full `MetaPreview`, not just URL/title.
- `PosterCard` emits `activated(MetaPreview)` on mouse click, but the first slice may connect it only to a debug log or placeholder signal.

Interfaces to lock:

```cpp
class AppContext : public QObject {
    Q_OBJECT
public:
    explicit AppContext(QObject* parent = nullptr);
    CatalogService* catalogs() const;
};

class CatalogService : public QObject {
    Q_OBJECT
public:
    explicit CatalogService(AddonClient* client, QObject* parent = nullptr);
    void loadHomeRows();

signals:
    void rowsReady(QString routeId, QVector<CatalogRowModel> rows);
    void rowFailed(QString routeId, QString rowKey, QString error);
};
```

Do not touch yet:

- No navigation stack.
- No Detail page.
- No Addons page.
- No SQLite.
- No search.
- No stream endpoints.

Smoke test:

- `build.bat` succeeds.
- Home still shows Popular Movies and Popular Series.
- Posters lazy-load as before.
- Sidebar placeholders still work.
- No visual layout drift from the current Home.

Definition of done:

- The app looks unchanged, but Home data now flows through `AppContext -> CatalogService -> HomePage`.
- `PosterCard` has enough metadata to open Detail in Slice 2.

### Slice 1A: Required Home HeroCarousel

Purpose:

- Add the required Harbor-style Home hero carousel once Home rows are service-backed.
- Make the first viewport feel like Harbor without waiting for late polish.

Harbor references:

- `src/views/home.tsx`
- `src/components/hero-carousel.tsx`
- `src/views/home/home-rows.ts`

Tankoban 3 files to add:

```text
src/ui/widgets/HeroCarousel.h
src/ui/widgets/HeroCarousel.cpp
```

Tankoban 3 files to change:

```text
CMakeLists.txt
src/ui/HomePage.h
src/ui/HomePage.cpp
src/core/CatalogService.h
src/core/CatalogService.cpp
src/ui/Theme.h
```

Exact work:

- `CatalogService` exposes a small hero pool derived from the first Cinemeta movie/series rows.
- `HomePage` renders `HeroCarousel` above catalog rows.
- Hero uses backdrop first, poster fallback second.
- Hero slide opens Detail through the same `MetaPreview` activation path as `PosterCard`.
- Carousel controls are minimal: current slide, previous/next, click-to-open.
- Keep it native Qt, not a web-style overlay stack.

Do not touch yet:

- No row customization.
- No editorial hero copy beyond title/metadata already available.
- No award/critic/queue modules.
- No TMDB-specific hero dependencies.

Smoke test:

- Home first viewport shows a working hero carousel.
- Slides rotate or can be advanced manually.
- Clicking a hero slide opens the same placeholder/Detail path as a poster.
- Rows below still render and lazy-load.

Definition of done:

- Home has the required Harbor identity feature before the rest of the app grows.
- The hero is backed by the same data models and navigation as poster rows.

### Slice 2: AppNavigator and Placeholder Frame Stack

Purpose:

- Recreate Harbor's push-stack behavior natively before building heavy screens.
- Keep shell routes distinct from pushed frames.

Harbor references:

- `src/App.tsx`
- `src/lib/view.tsx`
- `src/chrome/sidebar.tsx`
- `src/chrome/floating-back.tsx`

Tankoban 3 files to add:

```text
src/ui/RouteTypes.h
src/ui/AppNavigator.h
src/ui/AppNavigator.cpp
src/ui/pages/PlaceholderFramePage.h
src/ui/pages/PlaceholderFramePage.cpp
```

Tankoban 3 files to change:

```text
CMakeLists.txt
src/ui/AppContext.h
src/ui/AppContext.cpp
src/ui/MainWindow.h
src/ui/MainWindow.cpp
src/ui/HomePage.h
src/ui/HomePage.cpp
src/ui/CatalogRow.h
src/ui/CatalogRow.cpp
src/ui/PosterCard.h
src/ui/PosterCard.cpp
src/ui/Sidebar.h
src/ui/Sidebar.cpp
```

Exact work:

- Add `ShellRoute`, `FrameKind`, `Frame`, and `EpisodeRef` to `RouteTypes.h`.
- Add `AppNavigator` to own the frame stack.
- `MainWindow` still owns the visible shell, but delegates route changes to `AppNavigator`.
- Sidebar clicks call `AppNavigator::setShellRoute`.
- `PosterCard::activated` flows through `CatalogRow` and `HomePage` to `AppNavigator::openDetail`.
- Add placeholder pages:
  - Detail placeholder with Back and Open Picker buttons.
  - Picker placeholder with Back and Open Player buttons.
  - Player placeholder with Back/Exit buttons.
- Hide sidebar for Picker and Player. Keep sidebar visible for Detail.
- Keep root shell pages alive only as needed; do not implement complex keep-alive yet.

Interfaces to lock:

```cpp
class AppNavigator : public QObject {
    Q_OBJECT
public:
    void setShellRoute(ShellRoute route);
    void openDetail(const MetaPreview& preview);
    void openPicker(const MetaPreview& preview);
    void openPlayerPlaceholder(const MetaPreview& preview);
    void goBack();
    void exitPlayback();

signals:
    void chromeHiddenChanged(bool hidden);
};
```

Do not touch yet:

- Do not fetch meta detail.
- Do not build final Detail UI.
- Do not implement stream fetching.
- Do not persist navigation state.

Smoke test:

- Home -> poster click -> Detail placeholder.
- Detail -> Back returns to Home without resetting app.
- Detail -> Picker hides sidebar.
- Picker -> Player keeps sidebar hidden.
- Back from Player returns to Picker or Detail according to the stack rule chosen.
- Sidebar route click clears pushed frames and returns to the selected root route.

Definition of done:

- The Harbor navigation spine exists mechanically:
  `Shell route -> Detail frame -> Picker frame -> Player frame`.

### Slice 3: Cinemeta DetailPage

Purpose:

- Ship the first real Harbor-faithful pushed screen.
- Convert current poster rows into a usable Detail experience.

Harbor references:

- `src/views/detail.tsx`
- `src/views/detail/title-plate.tsx`
- `src/views/detail/synopsis.tsx`
- `src/views/detail/cinemeta-episodes.tsx`
- `src/views/detail/series-episodes.tsx`
- `src/lib/cinemeta.ts`

Tankoban 3 files to add:

```text
src/core/MetaService.h
src/core/MetaService.cpp
src/ui/pages/DetailPage.h
src/ui/pages/DetailPage.cpp
src/ui/widgets/DetailHero.h
src/ui/widgets/DetailHero.cpp
src/ui/widgets/EpisodeList.h
src/ui/widgets/EpisodeList.cpp
```

Tankoban 3 files to change:

```text
CMakeLists.txt
src/ui/AppContext.h
src/ui/AppContext.cpp
src/ui/AppNavigator.h
src/ui/AppNavigator.cpp
src/ui/Theme.h
src/core/AddonClient.h
src/core/AddonClient.cpp
src/core/MetaModels.h
```

Exact work:

- Add `MetaDetail` and `Video` parsing if not fully complete from Slice 1.
- `MetaService::fetchDetail(MetaPreview)` calls:
  - Cinemeta: `/meta/{type}/{id}.json` for `tt...` movie/series.
  - Addon-origin base URL later, but define the branch now.
- `DetailPage` accepts a `MetaPreview` immediately and renders a skeleton/fallback while detail loads.
- Hero:
  - backdrop image or poster fallback.
  - dark gradient overlay in Qt style.
  - large title text if no logo.
  - metadata pill row.
- Action bar:
  - Play button.
  - Add to Watchlist button as disabled or in-memory placeholder until Library slice.
- Body:
  - synopsis.
  - episode list for series from Stremio `videos`.
- Play behavior:
  - movie Play calls `AppNavigator::openPicker(detail, std::nullopt)`.
  - episode row calls `AppNavigator::openPicker(detail, video)`.

UI fidelity notes:

- Backdrop hero should feel full-bleed inside content, not a card.
- Avoid extra marketing copy.
- Do not add non-Harbor tabs or custom sections.
- Keep controls gray/black/white/gold according to current Harbor theme.

Do not touch yet:

- No TMDB enrichment.
- No TVDB episode enrichment.
- No trailers.
- No downloads.
- No favorites unless watchlist has already landed.
- No Addons page dependency.

Smoke test:

- Movie opens Detail with hero/title/synopsis.
- Series opens Detail with episodes.
- Back returns to the previous row scroll position if current layout already preserves it; if not, no crash/regression.
- Play opens Picker placeholder with the correct title.
- Episode click opens Picker placeholder with season/episode.

Definition of done:

- `Home -> Detail` is a real app path, not a placeholder.
- All metadata shown comes from Stremio/Cinemeta protocol fields.

### Slice 4: AddonRegistry and Minimal Addons Page

Purpose:

- Let the user install real Stremio-compatible addons locally.
- Make addon order and manifest capability checks available to CatalogService and StreamService.

Harbor references:

- `src/views/addons.tsx`
- `src/views/addons/add-by-url-bar.tsx`
- `src/views/addons/installed-pane.tsx`
- `src/views/addons/addon-detail.tsx`
- `src/lib/addon-store.ts`
- `src/lib/addons.ts`

Tankoban 3 files to add:

```text
src/core/AddonRegistry.h
src/core/AddonRegistry.cpp
src/core/AppRepository.h
src/core/AppRepository.cpp
src/ui/pages/AddonsPage.h
src/ui/pages/AddonsPage.cpp
src/ui/widgets/AddonCard.h
src/ui/widgets/AddonCard.cpp
src/ui/widgets/AddManifestBar.h
src/ui/widgets/AddManifestBar.cpp
```

Tankoban 3 files to change:

```text
CMakeLists.txt
src/ui/AppContext.h
src/ui/AppContext.cpp
src/ui/MainWindow.cpp
src/core/CatalogService.h
src/core/CatalogService.cpp
src/core/AddonClient.h
src/core/AddonClient.cpp
src/core/AddonModels.h
src/ui/Theme.h
```

Exact work:

- Add a lightweight repository layer. QSettings is acceptable for this slice if SQLite is deferred, but keep `AppRepository` as the API boundary.
- Implement manifest URL parsing:
  - accept `https://.../manifest.json`.
  - accept `stremio://.../manifest.json` by converting to HTTPS.
  - append `/manifest.json` if the user pastes a base URL.
  - reject non-HTTP(S).
- Fetch and validate manifest.
- Persist:
  - manifest URL.
  - base URL.
  - id, name, version, logo, background, resources, catalogs, behavior hints.
  - enabled state.
  - display order.
- Add Addons page with Harbor tab shape:
  - Discover: small static useful list or empty install-focused state.
  - Browse: paste/install manifest.
  - Installed: installed list, remove, enable/disable.
- `CatalogService` listens to `AddonRegistry::addonsChanged`.
- `CatalogService` can merge installed addon catalog rows after built-in Cinemeta rows.

Do not touch yet:

- No Stremio account sync.
- No addon configuration webview.
- No adult browsing beyond manifest flag display/toggle.
- No stream picker changes in this slice.

Smoke test:

- Paste a known manifest URL.
- Installed page shows name/logo/resources.
- Remove works.
- Restart preserves installed addon.
- Installing a catalog addon can add rows to Home.
- Bad manifest URL shows an error and does not corrupt saved addons.

Definition of done:

- Addons are real persisted Stremio manifests.
- Other services can ask the registry, "which enabled addons support this resource/type/id?"

### Slice 5: PlayPicker, StreamService, and Advanced StreamScorer

Purpose:

- Turn Detail Play into real stream discovery.
- Support direct URL playback first, with unsupported source types displayed honestly.
- Make advanced stream scoring part of the initial picker, not a later polish pass.

Harbor references:

- `src/views/play-picker.tsx`
- `src/views/play-picker/picker-header.tsx`
- `src/views/play-picker/stremio-layout.tsx`
- `src/views/play-picker/tier-strip.tsx`
- `src/views/play-picker/source-drawer.tsx`
- `src/views/play-picker/picker-empty-ladder.tsx`
- `src/lib/streams/types.ts`
- `src/lib/streams/addons.ts`
- `src/lib/streams/stream-ids.ts`
- `src/lib/streams/resolve.ts`

Tankoban 3 files to add:

```text
src/core/StreamService.h
src/core/StreamService.cpp
src/core/StreamParser.h
src/core/StreamParser.cpp
src/core/StreamScorer.h
src/core/StreamScorer.cpp
src/ui/pages/PlayPickerPage.h
src/ui/pages/PlayPickerPage.cpp
src/ui/widgets/PickerHeader.h
src/ui/widgets/PickerHeader.cpp
src/ui/widgets/QualityTierStrip.h
src/ui/widgets/QualityTierStrip.cpp
src/ui/widgets/StreamList.h
src/ui/widgets/StreamList.cpp
```

Tankoban 3 files to change:

```text
CMakeLists.txt
src/ui/AppContext.h
src/ui/AppContext.cpp
src/ui/AppNavigator.h
src/ui/AppNavigator.cpp
src/ui/pages/DetailPage.cpp
src/core/AddonRegistry.h
src/core/AddonRegistry.cpp
src/core/StreamModels.h
src/core/AddonClient.h
src/core/AddonClient.cpp
src/ui/Theme.h
```

Exact work:

- Implement `StreamService::buildStreamIds` using Harbor's priority:
  - explicit video id.
  - default video id for movie/no episode.
  - `tt...` movie id.
  - `tt...:season:episode` for series.
  - addon-native id fallback.
- Query only enabled addons whose manifest accepts `stream` for type/id.
- Fetch `{base}/stream/{type}/{id}.json`.
- Attach addon id/name/logo/order to every stream.
- Emit partial results as addon requests finish.
- Parse stream labels for picker display and scoring:
  - resolution: 4K/2160, 1080, 720, SD.
  - HDR/DV.
  - source quality: REMUX, BluRay, WEB-DL, WEBRip, HDTV, DVD, CAM, TS, TC, SCR.
  - codecs: HEVC, AVC/x264/x265, AV1, VP9.
  - audio: Atmos, TrueHD, DTS-HD MA, DTS, DD+, AC3, AAC, Opus, FLAC.
  - size if present in title or behavior hints.
  - language/dub markers.
  - release group when detectable.
- Score streams so clean real sources outrank fake-looking high-resolution labels:
  - `1080p WEB-DL` should outrank `4K CAM`.
  - direct playable URL should outrank unsupported/hash-only source when quality is otherwise comparable.
  - addon order is a tiebreaker, not the only ranking signal.
  - suspicious markers lower score, not just tier.
- Tier order:
  - 4K DV
  - 4K HDR
  - 4K
  - 1080p HDR
  - 1080p
  - 720p
  - SD
  - Rough
- `StreamService::resolve` returns `ResolvedStream` only for direct `url`.
- Picker displays unsupported rows for:
  - `infoHash` only.
  - `externalUrl`.
  - `ytId`.
  - `nzbUrl`.
  - `url == "#"` addon-not-configured marker.
- Picker has:
  - full-page backdrop.
  - fixed back button.
  - title/episode header.
  - loading state.
  - no addons state.
  - no streams state.
  - list of sources.
  - tier strip when tiers exist.

Do not touch yet:

- No debrid.
- No torrent engine.
- No instant autoplay unless the stream list and direct resolve path are already stable.
- No downloads.

Smoke test:

- Detail Play opens real Picker.
- If no stream addon is installed, Picker shows no-sources state.
- With a direct stream addon installed, streams appear.
- A `1080p WEB-DL` stream ranks above a `4K CAM/TS` stream in a controlled fixture/manual sample.
- HDR/DV, audio, codec, source, size, and language markers are visible when parsed.
- Direct stream click resolves and opens Player placeholder or Agent 4 player seam.
- Hash-only stream is visible but not playable.
- Back returns to Detail.

Definition of done:

- `Detail -> PlayPicker` is powered by Stremio `stream` endpoints.
- Streams are parsed, scored, tiered, and ordered before display.
- Direct streams can proceed to the player seam.

### Slice 6: PlayerRequest Seam and Continue-Watching Seed

Purpose:

- Make the resolved stream handoff concrete for Agent 4's libmpv widget.
- Start the local progress loop required for Library and Home continue-watching.

Harbor references:

- `src/lib/view.tsx` (`PlayerSrc`)
- `src/views/player.tsx`
- `src/views/player/hooks/use-resume-autosave.ts`
- `src/lib/resume.ts`
- `src/lib/playback-history.ts`

Tankoban 3 files to add:

```text
src/ui/player/VideoPlayerPage.h
src/ui/player/VideoPlayerPage.cpp
src/core/LibraryService.h
src/core/LibraryService.cpp
```

Tankoban 3 files to change:

```text
CMakeLists.txt
src/ui/AppContext.h
src/ui/AppContext.cpp
src/ui/AppNavigator.h
src/ui/AppNavigator.cpp
src/ui/pages/PlayPickerPage.cpp
src/core/AppRepository.h
src/core/AppRepository.cpp
src/core/StreamModels.h
src/ui/Theme.h
```

Exact work:

- Add `PlayerRequest` as the stable boundary between app navigation and the player widget.
- `AppNavigator::openPlayer(ResolvedStream, MetaDetail, optional Video)` builds `PlayerRequest`.
- `VideoPlayerPage` can initially be a placeholder widget that displays:
  - title.
  - URL host or URL.
  - stream addon/source.
  - subtitles count.
  - Back button.
- When Agent 4's player lands, `VideoPlayerPage::load(PlayerRequest)` becomes the integration point.
- Add `LibraryService` with initial methods:

```cpp
void saveProgress(PlayerRequest request, qint64 positionMs, qint64 durationMs);
QVector<ContinueWatchingItem> continueWatching() const;
qint64 resumePositionMs(QString mediaKey) const;
```

- `VideoPlayerPage` emits progress signals even if placeholder uses a fake/manual event during smoke. If Agent 4's widget is ready, connect real playback progress.
- Persist progress through `AppRepository`.
- Add media key rules now:
  - movie: meta id.
  - episode: meta id + season + episode.
- Do not build full Library page yet, but expose data for the later Home continue-watching row.

Do not touch yet:

- No full player chrome recreation unless Agent 4's player contract is already ready.
- No subtitle menu.
- No audio track menu.
- No watched sync.
- No downloads.

Smoke test:

- Direct stream from Picker opens `VideoPlayerPage`.
- `PlayerRequest` contains URL, title, meta, optional episode, headers, subtitles, source stream.
- Back exits Player and restores Picker or Detail according to navigator rules.
- A progress record can be saved and read back after restart.

Definition of done:

- The full spine exists as working app architecture:
  `Home/Rows -> Detail -> PlayPicker -> PlayerRequest`.
- Continue-watching persistence has a minimal backend ready for the Library slice.

## First Milestone

The first milestone is not "all pages exist." It is:

```text
Home poster -> Detail -> Play Picker -> direct URL stream -> PlayerRequest
```

That milestone proves the native Qt adaptation has the same core Harbor spine, the Stremio addon protocol is wired through real services, and Agent 4's player can attach without reshaping the app.

## First Three Concrete Tickets

1. Extract data models and service ownership.
   - Add `AddonModels.h`, `MetaModels.h`, `StreamModels.h`, `AppContext`.
   - Keep current Home behavior green.
   - No UI redesign.

2. Add `AppNavigator`.
   - Sidebar routes become navigator actions.
   - `PosterCard` emits the full `MetaPreview`.
   - Add placeholder Detail/Picker/Player frames.

3. Build `DetailPage` on Cinemeta meta.
   - Fetch `/meta/{type}/{id}.json`.
   - Render backdrop hero, metadata, synopsis, play action, and episodes.
   - Play action opens placeholder Play Picker.
