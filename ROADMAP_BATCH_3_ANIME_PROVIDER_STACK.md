# Batch 3 Roadmap: Anime Provider Stack

Planning date: 2026-06-16

Purpose: convert Harbor's Anime route and anime-specific metadata path into a native C++/Qt6 Tankoban 3 implementation. This batch makes Anime a first-class sidebar page with Jikan/MAL-backed rows, Kitsu detail/episode metadata, Anime Kitsu Stremio addon compatibility, anime search, adult filtering, and a clean handoff into the existing Detail -> Play Picker -> Player spine.

This batch intentionally avoids TMDB/API-key paths, AniList account integration, Simkl, anime awards, and recommendation/taste tuning.

## Harbor Diagnosis

Primary Harbor sources:

- `src/views/anime.tsx`: Anime route assembly, hero, static Jikan row specs, addon anime row merge, top-picks/tune hooks, AniList rows, award rows, loading cadence.
- `src/views/anime/anime-rows.tsx`: row model, Jikan row specs, addon anime row detection, lazy paging.
- `src/components/anime-hero.tsx`: large anime hero, slide rotation, Kitsu cover art, TMDB logo/backdrop fallback, awards badge, watchlist action, top-picks panel.
- `src/components/anime-genre-picker.tsx`: favorite-genre/tune-picks UI.
- `src/lib/providers/jikan.ts`: Jikan API rows, MAL -> Kitsu mapping through relations.yuna.moe, throttling, cache, adult filtering, franchise dedupe.
- `src/lib/providers/kitsu.ts`: Kitsu detail, cover art, episodes, characters, studios, streaming links, related titles, similar-by-genre.
- `src/lib/providers/anime-kitsu-addon.ts`: Anime Kitsu Stremio addon meta bridge from `kitsu:`, `mal:`, `anilist:`, `anidb:` ids to Stremio `meta` videos.
- `src/lib/providers/anime-mapping.ts`: Kitsu/MAL/AniDB/AniList/IMDb mapping through ARM, AniZip, and anime-list-master XML.
- `src/lib/providers/anime-detail.ts`: anime detail composition, episode merge, franchise building, Kitsu cast/studios/streamers, similar rows, TMDB/Fanart enrichments.
- `src/lib/providers/anime-episode-enrich.ts`: filler and TVDB thumbnail enrichment.
- `src/views/detail/anime-episodes.tsx`, `anime-episode-strip.tsx`: anime episode list/strip, season picker/franchise picker, filler/upcoming badges, episode click into picker.
- `src/lib/search.ts`, `src/components/search/anime-row.tsx`: Jikan search results opened as `mal:{id}` metadata.
- `src/lib/series-episodes.ts`: recognizes anime ids and gets episode playback ids from Anime Kitsu addon meta.

Harbor behavior to preserve:

- Anime route is not just a filtered Movies/Shows view. It has its own hero/row rhythm and anime-specific providers.
- Jikan/MAL supplies public rows without a user API key.
- Kitsu supplies richer anime detail, cover art, episode metadata, and ID stability.
- Anime Kitsu addon meta is the practical Stremio bridge for playable anime episodes and stream ids.
- Installed addon catalogs that look like anime rows are merged into the Anime page.
- Anime search returns Jikan results and opens them as `mal:{id}` metas.
- Adult anime is hidden when the adult-content filter is off.
- Episodes click into the same Play Picker used by normal series, carrying the anime stream id when available.

Tankoban 3 trims inside this batch:

- No TMDB lookup for anime logos, backdrops, cast fallback, detail enrichment, or rows.
- No Fanart/TVDB API-key enrichment.
- No AniList account rows, top-100 account surfaces, watched sync, or auto-sync.
- No Simkl continue-watching.
- No anime awards rows, award badges, or award detail pages.
- No recommendation/taste tuning: no thumbs, favorite-genre tuning, top-picks personalization, or "you might like" rows in v1.
- No trailer overlay. Kitsu/Jikan trailer ids may be stored for later, but no UI is built in this batch.
- No download buttons in anime episodes. Downloads are deferred to a separate owner plan.

## Product Scope

Keep:

- Anime sidebar page.
- Anime hero using non-TMDB assets only: Kitsu cover/background, Jikan poster, Stremio/addon background.
- Jikan rows:
  - Airing Now.
  - Top Airing on MAL.
  - Upcoming Season.
  - Top Series on MAL.
  - Top Movies on MAL.
  - Most Popular on MAL.
  - Top Rated on MAL.
  - Hidden Gems on MAL.
  - Era rows: 2020s, 2010s, 2000s, 90s/Foundation.
  - Genre rows: Action & Adventure, Romance, Slice of Life, Mecha, Fantasy, Sci-Fi, Psychological, Horror & Supernatural.
- Kitsu detail metadata for anime ids.
- Anime Kitsu Stremio addon bridge for meta/videos.
- Installed addon anime rows through `CatalogService`.
- Anime search through Jikan.
- Adult anime filtering.
- MAL score display when provided by Jikan/Kitsu/Stremio metadata.
- Local continue-watching compatibility after the Library/progress service exists.
- Episode list and optional strip layout, using the same picker/player spine as normal shows.

Defer:

- Anime hero top-picks side panel, because it is recommendation/taste-tuning adjacent.
- Favorite genre picker/tune picks.
- Kitsu character/cast row unless the general Detail cast decision later keeps cast rows.
- Kitsu studios/streaming-links UI unless the general Detail info-block decision later keeps those fields.
- Franchise picker beyond a lean season/source selector. Keep the data model ready, but do not block the first anime playback path on franchise graph traversal.
- Filler badge enrichment. It is useful, but not required for first playable anime.
- AniZip/XML/ARM deep mapping beyond what is needed for `mal:` -> `kitsu:` and stream id handoff.

Cut:

- AniList account integration and AniList rows.
- Simkl anime rows/history.
- Anime awards rows and badges.
- TMDB anime art/details.
- Fanart/TVDB key-based enrichments.
- Recommendation rows and tune UI for v1.
- Trailer overlay.
- Download actions.

## Qt File Map

Core:

```text
src/core/anime/AnimeModels.h
src/core/anime/AnimeService.h/.cpp
src/core/anime/JikanClient.h/.cpp
src/core/anime/KitsuClient.h/.cpp
src/core/anime/AnimeKitsuAddonClient.h/.cpp
src/core/anime/AnimeIdMapper.h/.cpp
src/core/anime/AnimeCatalogBuilder.h/.cpp
src/core/anime/AnimeEpisodeService.h/.cpp
src/core/anime/AnimeAdultFilter.h/.cpp
src/core/anime/AnimeCache.h/.cpp
```

UI:

```text
src/ui/pages/AnimePage.h/.cpp
src/ui/widgets/anime/AnimeHero.h/.cpp
src/ui/widgets/anime/AnimeRow.h/.cpp
src/ui/widgets/anime/AnimePosterCard.h/.cpp
src/ui/widgets/anime/AnimeRankCard.h/.cpp
src/ui/widgets/anime/AnimeEpisodeList.h/.cpp
src/ui/widgets/anime/AnimeEpisodeStrip.h/.cpp
```

Search:

```text
src/core/search/AnimeSearchService.h/.cpp
src/ui/widgets/search/AnimeResultRow.h/.cpp
```

Existing files expected to change:

```text
CMakeLists.txt
src/ui/AppContext.h/.cpp
src/ui/MainWindow.h/.cpp
src/ui/Sidebar.h/.cpp
src/ui/AppNavigator.h/.cpp
src/ui/pages/DetailPage.h/.cpp
src/ui/pages/SearchOverlay.h/.cpp
src/ui/pages/SettingsPage.h/.cpp
src/core/CatalogService.h/.cpp
src/core/MetaService.h/.cpp
src/core/StreamService.h/.cpp
src/core/AppRepository.h/.cpp
src/core/MetaModels.h
src/core/StreamModels.h
src/ui/Theme.h
```

## Data Models

```cpp
enum class AnimeIdKind {
    Kitsu,
    Mal,
    AniDb,
    AniList,
    Imdb,
    Unknown
};

struct AnimeId {
    AnimeIdKind kind = AnimeIdKind::Unknown;
    QString raw;          // kitsu:123, mal:456, anidb:789
    qint64 numericId = 0;
};

struct AnimeExternalIds {
    std::optional<qint64> kitsuId;
    std::optional<qint64> malId;
    std::optional<qint64> anidbId;
    QString imdbId;
};

struct AnimeMeta {
    AnimeId primaryId;
    AnimeExternalIds externalIds;
    QString name;
    QString originalName;
    QString type;         // series, movie, anime
    QString subtype;      // tv, movie, ova, ona, special
    QString synopsis;
    QUrl poster;
    QUrl background;
    QString releaseYear;
    QString startDate;
    QString endDate;
    QString status;
    QString ageRating;
    QStringList genres;
    QStringList studios;
    QString rating;       // MAL/Kitsu score normalized to 0-10 string
    int episodeCount = 0;
    int episodeLengthMinutes = 0;
    bool adult = false;
};

struct AnimeEpisode {
    QString id;
    int season = 1;
    int episode = 0;
    int absoluteNumber = 0;
    QString title;
    QString overview;
    QUrl thumbnail;
    QDate airDate;
    int runtimeMinutes = 0;
    QString streamId;     // Anime Kitsu/Stremio video id when available
    QString imdbId;
    int imdbSeason = 0;
    int imdbEpisode = 0;
    bool filler = false;
    bool upcoming = false;
};

struct AnimeRowSpec {
    QString key;
    QString title;
    QString provider;     // jikan, addon
    QString endpoint;
    QHash<QString, QString> query;
    bool ranked = false;
    bool paged = true;
};

struct AnimeRowModel {
    QString key;
    QString title;
    QVector<MetaPreview> items;
    bool ranked = false;
    bool hasMore = false;
    int nextPage = 2;
};

struct AnimeDetail {
    AnimeMeta meta;
    QVector<AnimeEpisode> episodes;
    QVector<MetaPreview> relatedFranchise; // optional, defer UI
};
```

Mapping rules:

- `kitsu:{id}` is the preferred canonical anime id for detail and playback.
- `mal:{id}` is accepted everywhere because Jikan search/rows are MAL-native.
- `anidb:{id}` and `anilist:{id}` are accepted as addon ids for compatibility, but Tankoban 3 does not add AniList account features.
- A Jikan row item should prefer `kitsu:{id}` when ARM mapping returns Kitsu; otherwise keep `mal:{id}`.
- Stream lookup must preserve Anime Kitsu video ids from episode metadata when present.

## Service Interfaces

### AnimeService

Owns anime route orchestration. UI pages talk to this, not directly to Jikan/Kitsu.

```cpp
class AnimeService : public QObject {
    Q_OBJECT
public:
    explicit AnimeService(
        JikanClient* jikan,
        KitsuClient* kitsu,
        AnimeKitsuAddonClient* animeKitsu,
        AnimeIdMapper* mapper,
        CatalogService* catalogs,
        AppRepository* repo,
        QObject* parent = nullptr);

    void loadPage();
    void fetchRowPage(QString rowKey, int page);
    void search(QString query, int limit = 8);
    void fetchDetail(MetaPreview preview);
    void fetchEpisodes(MetaPreview preview);

signals:
    void pageReady(QVector<AnimeRowModel> rows, QVector<MetaPreview> heroItems);
    void rowPageReady(QString rowKey, QVector<MetaPreview> items, bool hasMore, int nextPage);
    void searchReady(QString query, QVector<MetaPreview> results);
    void detailReady(QString metaId, AnimeDetail detail);
    void failed(QString context, QString error);
};
```

Behavior:

- Load fixed Jikan rows first, in small batches like Harbor, so the page becomes useful quickly.
- Merge local installed addon anime rows after Jikan rows.
- Use cached rows immediately when fresh, then refresh in the background.
- Keep route state separate from Home rows; Anime is a first-class page.
- Return `MetaPreview` so existing `AppNavigator::openDetail()` can open anime without a separate navigation path.

### JikanClient

Public, no-key provider for rows/search.

```cpp
class JikanClient : public QObject {
    Q_OBJECT
public:
    void fetchRow(AnimeRowSpec spec, int page);
    void search(QString query, int limit);
    void resolveMal(qint64 malId);

signals:
    void rowReady(QString rowKey, int page, QVector<AnimeMeta> items, bool hasMore);
    void searchReady(QString query, QVector<AnimeMeta> items);
    void malResolved(qint64 malId, AnimeExternalIds ids);
    void requestFailed(QString key, QString error);
};
```

Implementation notes:

- Base URL: `https://api.jikan.moe/v4`.
- Throttle requests. Harbor serializes with a minimum gap; Qt should do the same with a queue/timer to avoid 429s.
- Retry 429 with exponential backoff.
- Cache catalog/search responses for roughly 6 hours.
- Always pass `sfw=true` when adult content is hidden.
- Use WebP/JPG large image first.
- Normalize scores to one decimal.
- Franchise-dedupe simple sequel suffixes in row results, but do not overbuild franchise UI yet.

### KitsuClient

Detail/episode/art provider.

```cpp
class KitsuClient : public QObject {
    Q_OBJECT
public:
    void fetchAnime(qint64 kitsuId);
    void fetchEpisodes(qint64 kitsuId, int limit = 100);
    void fetchCover(qint64 kitsuId);

signals:
    void animeReady(qint64 kitsuId, AnimeMeta meta);
    void episodesReady(qint64 kitsuId, QVector<AnimeEpisode> episodes);
    void coverReady(qint64 kitsuId, QUrl cover);
    void requestFailed(QString key, QString error);
};
```

Implementation notes:

- Base URL: `https://kitsu.io/api/edge`.
- Accept JSON:API envelopes and keep parsing isolated.
- Detail fields to keep for v1: title, synopsis, poster, cover/background, rating, episode count/length, status, subtype, start/end date, age rating, genres.
- Episode fields to keep for v1: number, season number, title, synopsis, thumbnail, airdate, runtime.
- Character, studio, streamer, related, and similar APIs are optional deferred branches unless a later detail-scope decision keeps those UI rows.

### AnimeKitsuAddonClient

Stremio bridge for anime metadata/videos.

```cpp
class AnimeKitsuAddonClient : public QObject {
    Q_OBJECT
public:
    void fetchMeta(QString animeId);

signals:
    void metaReady(QString animeId, MetaDetail detail);
    void metaMissing(QString animeId);
    void requestFailed(QString animeId, QString error);
};
```

Implementation notes:

- Default host: `https://anime-kitsu.strem.fun`.
- Query both `/meta/series/{id}.json` and `/meta/movie/{id}.json`, matching Harbor.
- Accept ids beginning with `kitsu:`, `mal:`, `anilist:`, `anidb:`.
- Returned `videos` are critical because they include Stremio stream ids and IMDb mapping hints.
- This client should also work if the user installs Anime Kitsu through the Addon Store; the hardcoded default is a practical bootstrap, not a substitute for `AddonRegistry`.

### AnimeIdMapper

Converts between MAL/Kitsu/AniDB/AniList/IMDb only as needed for playback compatibility.

```cpp
class AnimeIdMapper : public QObject {
    Q_OBJECT
public:
    void malToKitsu(qint64 malId);
    void externalToKitsu(QString source, qint64 id);
    void kitsuToMal(qint64 kitsuId);

signals:
    void mappingReady(QString requestKey, AnimeExternalIds ids);
    void mappingMissing(QString requestKey);
};
```

Implementation notes:

- Use `https://relations.yuna.moe/api/ids` first because Harbor relies on it for MAL/Kitsu bridging.
- Cache positive mappings for 30 days.
- Cache negative mappings for 24 hours.
- Do not add AniList account calls. `anilist:` remains an identifier only.
- AniZip/XML mappings can be deferred unless needed to make stream handoff work for a specific installed addon.

### AnimeEpisodeService

Merges Kitsu episodes, Anime Kitsu addon videos, and existing `MetaDetail.videos`.

```cpp
class AnimeEpisodeService : public QObject {
    Q_OBJECT
public:
    QVector<AnimeEpisode> mergeEpisodes(
        QVector<AnimeEpisode> kitsuEpisodes,
        MetaDetail animeKitsuMeta,
        MetaDetail addonOrCinemetaDetail) const;

    Video toVideo(AnimeEpisode episode) const;
};
```

Merge priority:

1. Anime Kitsu/Stremio video id and IMDb hints.
2. Kitsu title/thumbnail/synopsis/runtime.
3. Addon meta videos if the installed addon supplied better stream ids.
4. Fallback generated `Episode N`.

The final `Video` handed to Play Picker must include:

- season.
- episode.
- name/title.
- overview.
- thumbnail.
- `id` or `defaultVideoId` that StreamService can query.
- anime-specific stream id when present.

### AnimeAdultFilter

```cpp
class AnimeAdultFilter {
public:
    bool isAdult(const AnimeMeta& meta) const;
    bool isAdultText(QStringView text) const;
};
```

Rules:

- Hide Jikan rating `Rx`.
- Hide genres `Hentai` and `Erotica`.
- Hide obvious adult title keywords.
- Reuse the Addon Store adult gate setting where possible so the app has one adult-content policy.

## UI Composition

### AnimePage

Native equivalent of Harbor `src/views/anime.tsx`.

Composition:

- `AnimeHero` at top.
- Row stack of `AnimeRow` widgets.
- Addon anime rows after the built-in public rows.
- Loading placeholders per row.
- Back-to-top affordance if the route scrolls deeply.

Behavior:

- On first show, call `AnimeService::loadPage()`.
- Render rows incrementally as batches return.
- Row "View all" opens the existing grid/row page once that generic route exists.
- Card click emits `MetaPreview` into `AppNavigator::openDetail()`.
- No visible tune/recommendation controls.

### AnimeHero

Faithful shape, trimmed data.

Keep:

- Large visual anime hero.
- Manual previous/next.
- Timed slide rotation.
- Title, year, score.
- Watchlist action if Library has landed.
- Click opens Detail.

Remove from this batch:

- TMDB logo/backdrop resolver.
- Awards badge.
- Top-picks side panel.
- Thumbs up/down tune controls.

Asset priority:

1. Kitsu cover/background.
2. Jikan poster as blurred/background fallback.
3. Stremio/addon background.
4. Poster fallback.

### Anime Rows

Row shape should reuse the existing `CatalogRow`/`PosterCard` visual language where possible, with anime-specific rank cards only for ranked MAL rows.

Rows:

```text
Airing Now
Top Airing on MAL
Upcoming Season
Top Series on MAL
Top Movies on MAL
Most Popular on MAL
Top Rated on MAL
Hidden Gems on MAL
2020s Hits
2010s Classics
2000s Era
Foundation Years
Action & Adventure
Romance
Slice of Life
Mecha
Fantasy
Sci-Fi
Psychological
Horror & Supernatural
Installed addon anime rows
```

### Detail Integration

`MetaService` should recognize anime ids:

```cpp
bool isAnimeId(QString id) {
    return id.startsWith("kitsu:")
        || id.startsWith("mal:")
        || id.startsWith("anidb:")
        || id.startsWith("anilist:");
}
```

When detail opens for anime:

1. Render the preview immediately.
2. Resolve Kitsu id if needed.
3. Fetch Kitsu detail and Anime Kitsu addon meta.
4. Merge episodes.
5. Return a normal `MetaDetail` plus an anime episode list.

Detail UI should use the existing `DetailPage` shell:

- Same hero/action bar/synopsis pattern.
- Episode list appears in the normal episode section.
- Play button for anime movies opens picker directly.
- Episode click opens picker with the merged `Video`.

No separate anime-only detail route unless the generic Detail page cannot handle the episode metadata.

### Search Integration

Harbor search opens Jikan hits as `mal:{id}` metas. Qt should do the same.

Search result row:

- section label: `Anime`.
- poster thumbnail.
- title.
- year.
- MAL score.
- short synopsis.

Click path:

```text
SearchOverlay -> AnimeResultRow -> AppNavigator::openDetail(MetaPreview{ id="mal:..." })
```

## Stream/Picker Handoff

Anime playback must not fork the player path.

Expected handoff:

```text
AnimePage/Search -> DetailPage -> AnimeEpisodeService -> PlayPickerPage -> StreamService -> VideoPlayer
```

Stream id rules:

- If `AnimeEpisode.streamId` exists, query streams with that id first.
- Also try the canonical anime meta id when the addon manifest supports anime ids.
- For movie-like anime, try `kitsu:{id}` or `mal:{id}` as the base id.
- Preserve `imdbId`, `imdbSeason`, and `imdbEpisode` hints for addons that expect IMDb-style series ids.
- Do not use TMDB ids.

`StreamService::buildStreamIds()` should accept anime ids without downgrading them to movie/series IMDb ids.

## Persistence

QSettings:

```text
settings/anime/hideAdult = true
settings/anime/enableJikanRows = true
settings/anime/heroRotationSeconds = 14
settings/anime/rowBatchSize = 6
```

SQLite/cache tables:

```sql
CREATE TABLE anime_cache (
    key TEXT PRIMARY KEY,
    provider TEXT NOT NULL,
    fetched_at INTEGER NOT NULL,
    expires_at INTEGER NOT NULL,
    json TEXT NOT NULL
);

CREATE TABLE anime_id_map (
    source TEXT NOT NULL,
    source_id TEXT NOT NULL,
    kitsu_id INTEGER,
    mal_id INTEGER,
    anidb_id INTEGER,
    anilist_id INTEGER,
    imdb_id TEXT,
    negative INTEGER NOT NULL DEFAULT 0,
    fetched_at INTEGER NOT NULL,
    expires_at INTEGER NOT NULL,
    PRIMARY KEY (source, source_id)
);
```

Recommended TTLs:

- Jikan rows/search: 6 hours.
- Kitsu detail/episodes: 30 minutes to 6 hours.
- Anime Kitsu addon meta: 6 hours.
- ID mappings: 30 days positive, 24 hours negative.

Do not persist recommendation/tune state in this batch.

## Phased Build Plan

### Phase 3.1: Models and Provider Boundary

Deliverable:

- Add `AnimeModels.h`.
- Add service class shells for `AnimeService`, `JikanClient`, `KitsuClient`, `AnimeKitsuAddonClient`, `AnimeIdMapper`, `AnimeEpisodeService`, and `AnimeAdultFilter`.
- Register services in `AppContext`.
- No visible UI change.

Dependencies:

- `AppContext` exists.
- `MetaPreview`, `MetaDetail`, and `Video` exist from the base roadmap.

Smoke:

- Build succeeds.
- Existing Home/Detail/Picker behavior is unchanged.

### Phase 3.2: Jikan Rows and Cache

Deliverable:

- Implement `JikanClient`.
- Implement fixed `AnimeRowSpec` list.
- Parse Jikan items into `AnimeMeta` and then `MetaPreview`.
- Add request throttling, 429 backoff, and cache.

Dependencies:

- Network/JSON utility in `AddonClient` or shared HTTP helper.
- `AppRepository` cache boundary if SQLite has landed; otherwise in-memory cache plus QSettings marker is acceptable temporarily.

Smoke:

- A diagnostic/manual call can load Airing Now and Top Anime.
- Adult filter removes `Rx` and Hentai/Erotica items when enabled.
- 429s do not hard-fail the page.

### Phase 3.3: AnimePage Rows

Deliverable:

- Add `AnimePage`.
- Wire Sidebar Anime route to the page.
- Render Jikan rows incrementally.
- Support paging/"View all" handoff once generic row grid exists.

Dependencies:

- MainWindow route wiring.
- Existing row/card widgets.

Smoke:

- Sidebar Anime opens a real page.
- Rows appear with posters and titles.
- Card click opens Detail preview path.
- Page remains usable if one row fails.

### Phase 3.4: Anime Hero

Deliverable:

- Add `AnimeHero`.
- Build hero pool from the first successful Jikan/Kitsu/addon row items.
- Use non-TMDB image priority.
- Add slide rotation and manual controls.

Dependencies:

- `AnimePage` rows.
- Shared image loader/cache.

Smoke:

- Anime first viewport has a large hero.
- Hero rotates or advances manually.
- Hero click opens Detail.
- No TMDB/API-key dependency exists.

### Phase 3.5: Installed Addon Anime Rows

Deliverable:

- Add anime row detection to `CatalogService` or `AnimeCatalogBuilder`.
- Merge installed addon anime catalogs into AnimePage.
- Respect `AddonRegistry` order.

Dependencies:

- Batch 1 Addon Store/AddonRegistry.
- Catalog aggregation service.

Detection:

- Catalog type is `anime`.
- Catalog/addon name contains anime, MAL, AniList, Kitsu, AniDB, Crunchyroll, etc.
- Sample ids include `kitsu:`, `mal:`, `anilist:`, or `anidb:`.

Smoke:

- Installing Anime Kitsu or another anime catalog addon adds rows.
- Disabling/removing the addon removes rows.
- Addon rows do not pollute Movies/Shows unless that generic catalog explicitly belongs there.

### Phase 3.6: Kitsu Detail and Anime Kitsu Episodes

Deliverable:

- `MetaService` routes anime ids into `AnimeService::fetchDetail()`.
- Resolve `mal:` to `kitsu:` where possible.
- Fetch Kitsu detail.
- Fetch Anime Kitsu addon meta/videos.
- Merge episodes.
- Render episodes in Detail.

Dependencies:

- DetailPage from the base roadmap.
- Anime Kitsu addon client.

Smoke:

- Opening a Jikan `mal:` result yields Kitsu-backed detail when mapping exists.
- Series displays episodes.
- Episode row opens Play Picker with anime stream id when present.
- Missing mapping still shows preview and a clear no-episodes state.

### Phase 3.7: Anime Search

Deliverable:

- Add `AnimeSearchService` or connect global SearchOverlay to `AnimeService::search()`.
- Add `AnimeResultRow`.
- Open result as `mal:{id}`.

Dependencies:

- Global SearchOverlay batch.
- JikanClient.

Smoke:

- Search for an anime title returns anime results.
- Clicking a result opens Detail.
- Search works without TMDB.

### Phase 3.8: Adult Filter and Settings

Deliverable:

- Add `settings/anime/hideAdult`.
- Reuse Addon Store adult gate language where possible.
- Apply to Jikan, Kitsu, and addon anime rows.

Dependencies:

- Settings page section.
- Addon Store adult gate if already landed.

Smoke:

- Adult-hidden default hides obvious adult anime.
- Enabling adult content refreshes rows.
- The filter does not require parental controls.

### Phase 3.9: QA, Failure States, and Performance

Deliverable:

- Empty states for provider down, no rows, no episodes, and no mapping.
- Cache eviction and refresh behavior.
- Scroll preservation.
- Manual smoke script/checklist.

Smoke:

- Disconnect network or block Jikan: Anime page shows a useful empty/error state.
- Kitsu failure does not crash Detail.
- Anime Kitsu failure still shows Kitsu detail with no playable episodes.
- Reopening Anime page uses cache quickly.

## Dependencies

Hard dependencies:

- Base `MetaPreview`, `MetaDetail`, `Video`.
- `AppContext`.
- `AppNavigator`.
- `DetailPage`.
- `CatalogService`.
- `MetaService`.
- `StreamService`.
- `AppRepository` or an equivalent cache/persistence boundary.

Soft dependencies:

- Batch 1 Addon Store for installed addon anime rows.
- Search batch for global anime search UI.
- Library batch for watchlist/continue-watching display.
- Player batch for real playback after Play Picker.

No dependency:

- TMDB.
- AniList account.
- Simkl.
- Awards.
- Recommendations.
- Downloads.

## Acceptance Criteria

- Anime sidebar opens a real native Qt page.
- Anime page has a Harbor-like hero and row stack.
- Jikan rows load without any user API key.
- Kitsu supplies anime detail/episode metadata where mapping exists.
- Anime Kitsu addon meta/videos can feed episode stream ids.
- Installed anime addon catalog rows appear after Batch 1 is available.
- Global Search can return and open anime results after the Search batch is available.
- Adult anime is hidden by default.
- Anime Detail uses the same Detail page shell as movies/shows.
- Anime episodes use the same Play Picker and Player handoff as normal series.
- No TMDB/API-key path is required or shown.
- No AniList/Simkl/awards/recommendation UI appears.

## Manual Smoke Checklist

1. Open Anime from the sidebar.
2. Confirm hero appears with non-TMDB art.
3. Confirm at least Airing Now, Top Airing, Upcoming, and Popular rows load.
4. Open a row item.
5. Confirm Detail shows anime title, poster/background, synopsis, rating/year, and episodes when available.
6. Click an episode.
7. Confirm Play Picker opens with an anime id/episode id request.
8. Install/enable an anime addon after Batch 1.
9. Confirm addon anime rows appear and disappear with install/disable/remove.
10. Search for an anime title after Search batch.
11. Confirm search result opens Detail.
12. Toggle adult filter and confirm rows refresh.

## Implementation Notes

- Keep the first pass practical. The essential anime stack is rows -> detail -> episodes -> picker, not a full anime social/account ecosystem.
- Prefer Kitsu ids for canonical detail/playback because Anime Kitsu and many Stremio anime addons understand them.
- Keep MAL ids because Jikan is MAL-native and search results come from Jikan.
- Cache aggressively enough to avoid annoying Jikan rate limits.
- Do not let anime provider classes leak into generic UI. UI should receive `MetaPreview`, `MetaDetail`, and `Video` wherever possible.
- Do not build a separate player/picker for anime. Anime should prove the app spine is generic enough.
