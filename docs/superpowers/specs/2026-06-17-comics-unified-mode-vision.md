# Tankoban 3 — Unified Comics Mode Vision

Date: 2026-06-17
Author: Agent 0 (DeepSeek) + Hemanth
Status: Brainstorming / discussion draft — no implementation until video mode is complete

## 1. Core Philosophy

Tankoban 3's Comics mode is a single unified mode containing **Manga** and **Comics** as subcategories — exactly the way Video mode has Movies and Shows. The driving principles:

1. **APIs construct the library, not website scrapers.** Tankoban 2 scraped MangaFire, RCO, and WeebCentral for catalogue data. Tankoban 3 should call free public APIs for catalogue metadata. Scraping is for the content/source layer only (the addons).

2. **Comics and manga are structurally identical to video.** A series has seasons/volumes, each containing episodes/issues/chapters. The Detail → Season/Volume picker → Reader flow mirrors the Home → Detail → Play Picker → Player flow exactly.

3. **Sources are addons, not built-in scrapers.** Mihon-inspired: WeebCentral is the default manga source, RCO is the default comics source. Users install more. The addon store model already being built for video mode should extend naturally to comics.

4. **Reader is ported from Tankoban Electron, not Tankoban 2.** The Tankoban Electron reader is simpler, slicker, and already handles both manga and comics through a single component with a `source` switch. It delivers to QWebEngine the same way Tankoban 2's BookReader delivers Foliate.

## 2. Catalogue APIs — Options Under Discussion

### 2.1 Manga Side

**AniList GraphQL** is the leading candidate:
- **Endpoint:** `https://graphql.anilist.co`
- **Auth:** None for public data
- **Rate:** 90 req/min (currently degraded to 30)
- **Gives you:** Series title, synopsis, cover art, volumes/chapters count, status, genres, staff, characters
- **Proven:** Tankoban 2 and Tankoban Electron both already use it

Alternatives not yet ruled out: MyAnimeList API (requires registration), Kitsu (free, JSON, more anime-focused).

### 2.2 Western Comics Side

This is the open question. Several candidates were scouted:

**Metron** ([metron.cloud](https://metron.cloud)) — best API-first free option:
- REST API with proper endpoints: `/api/series/`, `/api/issue/`, `/api/series/{id}/issue_list/`
- Free account (username+password), 20 req/min burst, 5,000/day sustained
- ~2M+ issues, cover art in detail responses, community-maintained
- Downside: requires credentials → needs a proxy server so users don't have to create Metron accounts

**ComicVine** (comicvine.gamespot.com/api):
- Most mature, richest metadata, 200 req/hr free tier
- Downside: requires per-user API key (same friction as Metron, worse UX), rate limit per key

**Grand Comics Database** (comics.org):
- Largest dataset: 9K+ publishers, 99K+ series, 1.3M+ issues
- Fully open, no auth
- Downside: no proper REST API — only data dumps (~4GB SQLite). Would need self-hosting + query layer. No cover image URLs in the dump.

**OpenLibrary** (openlibrary.org):
- Free, no auth, REST API with subject browsing (comics, graphic_novels)
- Downside: not comic-native — library catalog semantics, poor issue-level detail. Probably not viable as primary.

**iTunes Search API** — already proven in Tankoban Electron:
- Not a catalogue API. It's a cover-art upscaler.
- Takes an RCO comic title → searches Apple Books → returns hi-res cover URL (up to 2000x2000px)
- Free, keyless, ~20 req/min
- Complements whichever catalogue API is chosen

### 2.3 The Credential Problem

Every serious comics API (Metron, ComicVine, even GCD for data dumps) requires some form of account. The options are:

- **Option A: Require users to create accounts.** Friction. "Just works" is a core value.
- **Option B: Ship a shared app-level account.** Simple but doesn't scale. Gray area with ToS.
- **Option C: Small proxy server.** Hide credentials behind a thin caching server. $5/month VPS. Tankoban 3 talks to proxy, proxy talks to Metron.
- **Option D: Self-host GCD dumps.** Download the 4GB SQLite dump, bundle it, query locally. No API dependency, but stale data and distribution costs.

None decided yet. Hemanth's call.

## 3. Data Models — Unified Shape For Discussion

The catalogue normalizes manga and comics into a shared shape that mirrors Video's `MetaPreview` / `MetaDetail` / `Video` pattern. This is a sketch, not a contract:

```cpp
// Shared across manga and comics
enum class ComicMedium { Manga, Comic };

struct ComicPreview {
    QString id;              // anilist:12345 or metron:67890
    ComicMedium medium;
    QString title;
    QUrl poster;             // cover art
    QUrl background;         // banner/backdrop
    QString author;          // mangaka or writer
    QString status;          // Publishing / Ongoing / Completed
    QStringList genres;
    QString sourceLabel;     // "AniList" or "Metron"
};

struct ComicDetail : ComicPreview {
    QString description;     // synopsis
    QString publisher;
    QString artist;          // separate from author for western comics
    int volumeCount = 0;
    int chapterCount = 0;
    QVector<ComicVolume> volumes;
};

struct ComicVolume {
    int number;
    QString title;
    int chapterCount;
    QVector<ComicChapter> chapters;
};

struct ComicChapter {
    QString id;              // weebcentral:<id> or rco:<slug>
    double number;           // e.g. 1185 or 10.5
    QString title;
    QString date;
    bool downloaded = false;
    qint64 downloadProgress = 0;
};
```

Key question: AniList gives volume/chapter counts but not per-chapter data. Metron gives individual issues. How does the normalizer resolve this? For manga, the chapter list comes from the content addon (WeebCentral), not the catalogue API. The catalogue gives metadata; the addon gives the actual chapter listing.

## 4. Addon Store Model

Video mode has Stremio addons (Cinemeta for catalogue, community addons for streams). Comics mode extends the same pattern with content addons:

### Default Addons (pre-installed)

| Addon | Medium | Role |
|-------|--------|------|
| **WeebCentral** | Manga | Chapter list + page images |
| **RCO (ReadComicOnline)** | Comics | Issue list + page images |

### Addon vs Catalogue — The Separation

The catalogue API (AniList / Metron / whatever we choose) provides **metadata**: title, synopsis, cover art, creator info, volume count. The addon provides **content**: which chapters actually exist, where to get their pages.

This is the same split Video mode has: Cinemeta gives you the movie poster and description, but the stream addon gives you the actual watchable stream. A user could install a different manga source addon and get different chapter availability from the same AniList metadata.

### Addon Manifest Sketch

```json
{
  "id": "weebcentral",
  "name": "WeebCentral",
  "version": "1.0.0",
  "medium": "manga",
  "resources": ["chapters", "pages", "search"],
  "rateLimit": { "requestsPerMinute": 15 }
}
```

The existing Video addon store (being built in the current roadmap) has Discover, Browse, Installed tabs, paste-manifest install, enable/disable. Comics addons live in the same store, filtered by medium.

## 5. Screen Architecture

### 5.1 Mode Shell

The sidebar gains a **Comics** route. Clicking it switches to the Comics mode shell with sub-navigation:

```
Comics (sidebar route)
  ├── Home (hero + rows: Popular Manga, Latest Comics, Continue Reading)
  ├── Browse (grid, filterable: All / Manga / Comics)
  ├── Search (unified search across catalogue APIs + addon content)
  ├── Library (downloaded + continue reading)
  └── Addons (shared addon store, filtered to comics/manga addons)
```

### 5.2 Navigation Stack

Same push-stack pattern as Video:

```
Home/Browse/Search
  → Series Detail (hero + synopsis + volume picker + chapter list)
    → Reader (fullscreen takeover, hides sidebar)
```

The Series Detail reuses the same widgets Video's DetailPage uses: `DetailHero`, `MetaPills`, `ActionBar`, `Synopsis`, `EpisodeList`. The only difference is data — manga chapters instead of TV episodes, comic issues instead of movie streams.

### 5.3 Series Detail

Matches the Tankoban Electron `MangaSeries.jsx` / `ComicsSeries.jsx` pattern:

- **Hero:** Backdrop image (AniList banner or hi-res iTunes cover), title plate, metadata pills (author, status, genres), action bar (Read/Continue primary, Download Volume secondary)
- **Volume picker:** Dropdown for manga with volume structure. Comics can be flat issue list or grouped by TPB collection.
- **Chapter/Issue list:** `EpisodeList` with per-chapter rows. Number, title, date, download status, progress bar. Click downloaded → reader. Click undownloaded → download.
- **Info grid:** Type, Author/Writer, Artist, Status, Publisher, Chapter count, Genres, Source

### 5.4 Reader — QWebEngine + Tankoban Electron JS Engine

The reader is a `QWebEngineView` hosting the Tankoban Electron reader engine as static HTML/JS/CSS — exactly like Tankoban 2's BookReader hosts Foliate:

```
ComicReader (QWidget)
  └── QWebEngineView (full viewport)
        └── reader.html + readerEngine.js + comic-reader.css
              └── QWebChannel → ComicBridge (C++ ↔ JS)
```

**Why not native Qt:** The Tankoban Electron reader engine ([readerEngine.js](C:\Users\Suprabha\Desktop\Tankoban Electron\src\renderer\src\lib\readerEngine.js)) is 160 lines of pure math handling spread detection, page pairing, RTL/LTR mirroring, step navigation, and pixel-perfect layout. The reader UI ([MangaReader.jsx](C:\Users\Suprabha\Desktop\Tankoban Electron\src\renderer\src\pages\manga\MangaReader.jsx)) is 1340 lines of battle-tested reader chrome. Rewriting this in native Qt C++ would be months. The QWebEngine approach reuses the proven JS engine verbatim while keeping the shell (window chrome, modals, file I/O, persistence) in native Qt. Tankoban 2's [BookReader](src/ui/readers/BookReader.h) + [BookBridge](src/ui/readers/BookBridge.h) already proves this pattern works.

**Bridge surface (ComicBridge via QWebChannel):**

| Method | Direction | Purpose |
|--------|-----------|---------|
| `filesRead(path)` | JS → C++ | Read downloaded page image |
| `progressGet(seriesId, chapterId)` | JS → C++ | Load saved reading position |
| `progressSave(seriesId, chapterId, data)` | JS → C++ | Save position (page, max) |
| `prefsGet()` / `prefsSave(data)` | JS → C++ | Reader preferences |
| `windowToggleFullscreen()` | JS → C++ | OS-level fullscreen |
| `requestClose()` | JS → C++ | Back to series detail |
| `markReaderReady()` | JS → C++ | Engine initialized → hide loading overlay |

**Reading modes** (all four come from the Tankoban Electron engine):

| Mode | Behavior |
|------|----------|
| Long Strip | Vertical scroll, IntersectionObserver tracks visible page |
| Single Page | One page at a time, click edges to turn, direction-aware |
| Double Page | Facing pages, auto-detects wide spreads, RTL/LTR mirror |
| MangaPlus | Double page but forced LTR (official flipped layout) |

**Reader features** (all from Tankoban Electron):
- HUD toolbar: Back, Series title, Chapter popup, Prev/Next, Page counter, Reading mode dropdown, Direction toggle, Pairing offset, Width %, Fullscreen, Settings
- Side navigation bars: Auto-hide edge buttons for prev/next (paged) or scroll up/down (strip)
- Click zones: Left third → previous, right third → next, center double-click → fullscreen
- Preferences modal: Reading style, direction, image fit, gap, dark background, back-to-top, sticky nav, portrait width
- Continue reading: Progress saved on page turn + unmount. Deep-link resume via `?page=N`
- Neighbour prefetch in paged modes
- Keyboard: Arrow keys (direction-aware), Escape, F (fullscreen), G (gap toggle)
- Download-fed: Reader opens only downloaded chapters. Undownloaded → "go download it" message

**Both manga and comics share the same reader.** The Tankoban Electron reader already works this way — a `source` prop switches between manga and comics data sources. The reader component, engine, HUD, preferences, and bridge are identical. Only the data source changes.

## 6. Home Integration

The unified Comics Home page shows:
- **Hero carousel** — featured/trending from catalogue APIs
- **Continue Reading row** — merged manga + comics progress
- **Popular Manga row** — from AniList
- **Latest Comics row** — from whatever comics API we choose
- **Recently Downloaded row** — from local library

Each row reuses `CatalogRow` + `PosterCard`. A `ComicPreview` flows through the same `PosterCard::activated` → `AppNavigator::openDetail` path that video posters use.

## 7. Reference Material

| Source | Path | Relevance |
|--------|------|-----------|
| Tankoban Electron reader engine | `src/renderer/src/lib/readerEngine.js` | Pure-math spread detection, page pairing, step navigation, layout (~160 lines) |
| Tankoban Electron reader UI | `src/renderer/src/pages/manga/MangaReader.jsx` | Full reader component: HUD, modals, click zones, side bars, fullscreen, keyboard (~1340 lines) |
| Tankoban Electron manga series | `src/renderer/src/pages/manga/MangaSeries.jsx` | Volume picker, chapter list, download flow |
| Tankoban Electron comics series | `src/renderer/src/pages/comics/ComicsSeries.jsx` | Issue list, download flow |
| Tankoban Electron AniList client | `src/main/anilist.js` | GraphQL queries for manga |
| Tankoban Electron iTunes covers | `src/main/itunesCovers.js` + `src/renderer/src/lib/itunesCovers.js` | Cover search, title matching, hi-res URL rewrite, dual cache |
| Tankoban 2 BookReader | `src/ui/readers/BookReader.h/.cpp` | QWebEngineView + QWebChannel + Bridge pattern to replicate |
| Tankoban 2 BookBridge | `src/ui/readers/BookBridge.h/.cpp` | QWebChannel bridge: file I/O, progress, prefs, window chrome, ready gate |
| Tankoban 3 ROADMAP | `ROADMAP.md` | Video mode service architecture that comics mode mirrors |
| Tankoban 3 Addon Registry | Slice 4 in `ROADMAP.md` | AddonRegistry pattern to extend for comics content addons |

## 8. Open Questions

1. **Which comics catalogue API?** Metron, GCD dumps, ComicVine, or something else? None have been decided.
2. **How to handle the credential problem?** Proxy server, shared account, user accounts, or self-hosted dumps?
3. **Does iTunes cover upscaling apply to both manga and comics, or only comics?** AniList covers are already decent quality. Metron/GCD covers may benefit from the iTunes upscaler.
4. **Volume structure for western comics?** Manga has MangaFire volume ranges. Western comics have TPBs/collected editions. How does the unified volume model represent both?
5. **Does the reader stay download-fed?** Tankoban Electron requires download-before-read. Could it also support streaming (progressive image loading from addon URLs)? Trade-off between instant access and offline reliability.
6. **Shared sidebar with Video mode vs separate Comics sidebar?** The current Harbor recreation has a video-focused sidebar. Does Comics mode get its own nav rail or share the same one with a mode switcher?

## 9. Non-Goals For Now

- No building until video mode (Harbor 1:1 recreation) is complete
- No decisions locked — this is a discussion document
- No Tankoban 2 comic reader port — the reader comes from Tankoban Electron
- No separate Manga mode and Comics mode — they're one unified Comics mode
- No AniList user accounts or list sync — AniList is catalogue-only
