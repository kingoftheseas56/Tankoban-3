# Batch 1 Roadmap: Addon Store

Planning date: 2026-06-16

Purpose: convert Harbor's Addons app-store experience into a native C++/Qt6 Tankoban 3 implementation. This batch covers addon discovery, public/community browsing, install/update/remove, installed addon management, ordering, backups, detail pages, adult gating, and manifest/deep-link install. It does not implement debrid playback behavior; debrid remains the last roadmap batch.

## Harbor Diagnosis

Primary Harbor sources:

- `src/views/addons.tsx`: top-level Addons route, tabs, search, paste URL, adult toggle, detail routing, install modal, organize overlay.
- `src/views/addons/discover-pane.tsx`: hero, curated rails, community rail, category grid.
- `src/views/addons/browse-pane.tsx`, `community-browse-list.tsx`: top/new/rising marketplace list, infinite paging, category/search filters.
- `src/views/addons/installed-pane.tsx`: installed grid, enable/disable switch, manage configurable addon, remove, reorder entry.
- `src/views/addons/addon-detail.tsx`: addon hero, install/remove/configure, copy URLs, masked manifest URL, manifest stats, docs, related/recommended rails.
- `src/views/addons/install-modal.tsx`: paste/resolve manifest, detect update/reconfigure, show stages, save result.
- `src/views/addons/organize/page.tsx`: reorder installed addons, backups, save/revert workflow.
- `src/lib/addon-store.ts`: local installed addon store, manifest parsing, validation, slim manifest persistence, enable/disable, install/update/uninstall.
- `src/lib/addons-store/store.ts`: merges curated, installed, Stremio account, stremio-addons.net, and community sources into `ResolvedAddon`.
- `src/lib/addons-store/curated.ts`: curated addons, rails, categories, tags, hero.
- `src/lib/addons-store/community.ts`: Stremio community/official directory fallback.
- `src/lib/providers/stremio-addons.ts`: stremio-addons.net API provider, categories, top/new/rising, detail.
- `src/lib/providers/stremio-addons-index.ts`: cached marketplace index by manifest id/slug.
- `src/lib/addons-store/adult-filter.ts`: adult text/manifest detection.
- `src/lib/addons-store/reorder.ts`: order sequence, validation, backup snapshots.
- `src/lib/addons-store/recommend.ts`: related/recommended addon ranking.

Harbor behavior to preserve:

- Addons page has three primary tabs: `Discover`, `Browse`, `Installed`.
- Header includes store search, paste/install URL, adult visibility toggle, and Browse filters.
- Discover has a featured addon hero, curated starter rails, category tiles, and public/community rails.
- Browse supports top-rated, rising, newest, search, category, paging, loading, empty, and installed markers.
- Installed supports search, open detail, enable/disable, remove, manage configurable addon, and reorder.
- Detail page shows addon identity, logo/background, category, tags, description, install/remove/configure actions, copy manifest URL, copy `stremio://` link, manifest stats, docs, and related rails.
- Manifest URLs are masked by default because configured addons can contain tokens in the URL path.
- Paste/install flow normalizes `https://`, `stremio://`, base URLs, and `/configure` URLs into a manifest URL, previews the manifest, detects replacement/update, and commits only after validation.
- Addon order matters. Catalog and stream services query installed addons in user order.

Tankoban 3 trims inside this batch:

- No Stremio account sign-in or cloud addon sync.
- No Trakt/AniList/Simkl addon sync affordances.
- No Live TV/IPTV category tile or Live TV-specific copy.
- No TMDB-specific curated rails or TMDB/API-key nudges.
- No debrid provider engine, cache checks, or debrid stream resolving in this batch. Debrid addons can be listed/installed as generic Stremio addons, but playback behavior is handled in the final debrid batch.
- No embedded configuration browser in the first Qt pass. Configurable addons open the system browser to their configure URL and the app accepts the resulting `stremio://` or manifest URL through deep link/paste.
- No related/recommended addon personalization. Keep same-category related rows only if they are cheap after the catalog index exists.

## Product Scope

Keep:

- Full addon marketplace, not just paste-manifest install.
- Public stremio-addons.net provider.
- Stremio community/official fallback directories.
- Curated local rails, after trimming cut categories.
- Discover/Browse/Installed tabs.
- Addon detail page.
- Install by manifest URL and `stremio://`.
- Install, update/replace, uninstall.
- Enable/disable installed addons.
- Addon ordering.
- Local order backups/export/import.
- Adult addon gate for the addon store. This is store safety, not parental controls.
- Mask/reveal/copy manifest URL.

Defer:

- In-app configure webview via QtWebEngine.
- Related/recommended addon rows if they slow the first store implementation.
- Marketplace velocity/rising history beyond what stremio-addons.net directly returns.
- Cloud sync.

Cut:

- Account sync.
- Live TV/IPTV addon category surface.
- TMDB-specific store nudges/rails.
- Social/rating actions that send the user to star/rate addons.

## Qt File Map

Core:

```text
src/core/addons/AddonStoreModels.h
src/core/addons/AddonUrl.h/.cpp
src/core/addons/AddonManifestValidator.h/.cpp
src/core/addons/AddonRegistry.h/.cpp
src/core/addons/AddonStoreService.h/.cpp
src/core/addons/AddonMarketplaceProvider.h
src/core/addons/StremioAddonsProvider.h/.cpp
src/core/addons/StremioDirectoryProvider.h/.cpp
src/core/addons/CuratedAddonsProvider.h/.cpp
src/core/addons/AddonCategorizer.h/.cpp
src/core/addons/AdultAddonFilter.h/.cpp
src/core/addons/AddonOrderService.h/.cpp
src/core/addons/AddonLogoCache.h/.cpp
```

UI:

```text
src/ui/pages/AddonsPage.h/.cpp
src/ui/pages/AddonDetailPage.h/.cpp
src/ui/dialogs/AddonInstallDialog.h/.cpp
src/ui/dialogs/AddonOrganizeDialog.h/.cpp
src/ui/dialogs/AgeGateDialog.h/.cpp
src/ui/widgets/addons/AddonSearchBar.h/.cpp
src/ui/widgets/addons/AddManifestBar.h/.cpp
src/ui/widgets/addons/AddonHeroCard.h/.cpp
src/ui/widgets/addons/AddonRail.h/.cpp
src/ui/widgets/addons/AddonCard.h/.cpp
src/ui/widgets/addons/AddonListRow.h/.cpp
src/ui/widgets/addons/AddonCategoryGrid.h/.cpp
src/ui/widgets/addons/InstalledAddonRow.h/.cpp
src/ui/widgets/addons/AddonTagRow.h/.cpp
src/ui/widgets/addons/AddonToast.h/.cpp
```

Persistence:

```text
src/core/AppRepository.h/.cpp
```

Extend existing repository work rather than letting addon UI write `QSettings`/SQLite directly.

## Data Models

```cpp
enum class AddonStoreSource {
    Curated,
    Community,
    StremioAddonsNet,
    LocalInstalled
};

enum class AddonCategory {
    Streams,
    Metadata,
    Subtitles,
    Anime,
    Tools,
    Adult,
    Unknown
};

enum class AddonTag {
    Official,
    Free,
    Premium,
    DebridRequired,
    P2P,
    Torrent,
    Usenet,
    Configurable,
    SelfHost
};

struct AddonStoreEntry {
    QString id;
    QUrl manifestUrl;
    QString slug;
    Manifest manifest;
    AddonStoreSource source = AddonStoreSource::Community;
    AddonCategory category = AddonCategory::Unknown;
    QVector<AddonTag> tags;
    QString curatorNote;
    QStringList warnings;
    QStringList railIds;
    int recommendedScore = 0;
    int stars = 0;
    QDateTime createdAt;
    QDateTime updatedAt;
    bool installed = false;
    bool enabled = true;
    bool adult = false;
};

struct AddonListPage {
    QVector<AddonStoreEntry> entries;
    int page = 1;
    int totalPages = 1;
    bool hasNextPage = false;
};

struct AddonInstallPreview {
    QUrl manifestUrl;
    Manifest manifest;
    enum MatchKind { Fresh, IdMatch, HostnameMatch } matchKind = Fresh;
    QString replaceId;
    QString replaceName;
};

struct AddonOrderBackup {
    QDateTime createdAt;
    QVector<QUrl> manifestUrls;
    QStringList names;
};
```

Use the existing planned `Manifest`, `CatalogDef`, and `InstalledAddon` models from `ROADMAP.md`. Do not duplicate protocol models.

## Services

### AddonRegistry

Owns installed addons and is the single source of truth for all other services.

Responsibilities:

- Normalize manifest URLs.
- Fetch and validate manifests.
- Install, update/replace, uninstall.
- Enable/disable.
- Preserve order.
- Emit `addonsChanged`.
- Expose `enabledAddons()` for `CatalogService`, `MetaService`, `StreamService`, and `SubtitleService`.

Interface:

```cpp
class AddonRegistry : public QObject {
    Q_OBJECT
public:
    explicit AddonRegistry(AppRepository* repo, AddonClient* client, QObject* parent = nullptr);

    QVector<InstalledAddon> installedAddons() const;
    QVector<InstalledAddon> enabledAddons() const;
    bool isInstalled(QString addonId) const;
    bool isEnabled(QUrl manifestUrl) const;

    void installFromUrl(QString rawUrl, QString replaceId = {});
    void uninstall(QString addonId, QUrl manifestUrl = {});
    void setEnabled(QUrl manifestUrl, bool enabled);
    void setOrder(QVector<QUrl> orderedManifestUrls);
    void refreshInstalledManifests();

signals:
    void installPreviewReady(AddonInstallPreview preview);
    void installSucceeded(InstalledAddon addon, bool replaced);
    void installFailed(QString rawUrl, QString message);
    void uninstallSucceeded(QString addonId);
    void addonsChanged();
};
```

### AddonStoreService

Qt equivalent of Harbor's `useAddonsCatalog`.

Responsibilities:

- Merge curated, public marketplace, community fallback, and installed addons.
- Dedupe by manifest id, manifest URL, and normalized name.
- Apply adult filtering.
- Attach installed/enabled state.
- Build Discover rails.
- Serve Browse pages.
- Serve Addon Detail lookup by manifest id or slug.

Interface:

```cpp
class AddonStoreService : public QObject {
    Q_OBJECT
public:
    explicit AddonStoreService(
        AddonRegistry* registry,
        CuratedAddonsProvider* curated,
        StremioAddonsProvider* marketplace,
        StremioDirectoryProvider* directories,
        QObject* parent = nullptr);

    void loadDiscover(bool includeAdult);
    void browse(QString mode, QString category, QString search, int page, bool includeAdult);
    void loadDetail(QString addonIdOrSlug, bool includeAdult);
    QVector<AddonCategory> categories() const;

signals:
    void discoverReady(AddonStoreEntry hero, QVector<AddonRailModel> rails);
    void browseReady(AddonListPage page);
    void detailReady(AddonStoreEntry entry);
    void loadingChanged(bool loading);
    void error(QString message);
};
```

### Providers

`StremioAddonsProvider`:

- API base: `https://stremio-addons.net/api/v0`.
- `GET /addons?page=&limit=&search=&category=&sort_by=&order=&nsfw=`.
- `GET /addons/{uuidOrSlug}`.
- `GET /categories`.
- `GET /rising`.
- Cache list/detail/category responses for 1 hour.
- Fall back to `StremioDirectoryProvider` when API fails.

`StremioDirectoryProvider`:

- Fetch community/official directories:
  - `https://v3-cinemeta.strem.io/addon_catalog/all/community.json`
  - `https://v3-cinemeta.strem.io/addon_catalog/all/official.json`
  - `https://api.strem.io/addonsofficialcollection.json`
- Return normalized `AddonStoreEntry` records.

`CuratedAddonsProvider`:

- Native static table equivalent of Harbor's `CURATED_ADDONS`.
- Remove entries that are purely Live TV/IPTV, TMDB-specific, Trakt/account-sync, ratings-only, trailers-only, or watch-party/social.
- Keep stream, subtitle, anime, catalog/metadata, and useful tool addons that do not violate current cuts.

### AddonOrderService

Responsibilities:

- Store display order.
- Reorder installed addons.
- Save and restore last five local backups.
- Validate reorder by URL multiset before saving.
- Export/import backup JSON for local recovery.

No cloud validation in Tankoban 3 v1 because account sync is cut.

## Persistence

SQLite:

```sql
installed_addons(
  manifest_url TEXT PRIMARY KEY,
  addon_id TEXT NOT NULL,
  name TEXT NOT NULL,
  version TEXT,
  logo TEXT,
  background TEXT,
  manifest_json TEXT NOT NULL,
  installed_at INTEGER NOT NULL,
  display_order INTEGER NOT NULL,
  enabled INTEGER NOT NULL DEFAULT 1,
  source TEXT NOT NULL DEFAULT 'local'
);

addon_order_backups(
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  created_at INTEGER NOT NULL,
  urls_json TEXT NOT NULL,
  names_json TEXT NOT NULL
);

addon_store_cache(
  cache_key TEXT PRIMARY KEY,
  fetched_at INTEGER NOT NULL,
  json TEXT NOT NULL
);
```

QSettings:

- `settings/sources/showAdultAddons`
- `settings/addons/lastTab`
- `settings/addons/lastBrowseMode`
- `settings/addons/lastCategory`

Manifest persistence must be slimmed:

- Keep `id`, `name`, `version`, `description`, `logo`, `background`, `types`, `idPrefixes`, `resources`, `catalogs`, `behaviorHints`.
- Drop data-URL logos/backgrounds.
- Truncate very long descriptions.
- Persist the full raw manifest only if it remains small enough for SQLite without cache bloat.

## UI Composition

### AddonsPage

Native layout:

- `QWidget` page inside shell route.
- Fixed page header with:
  - segmented tab control: Discover / Browse / Installed.
  - `AddonSearchBar`.
  - compact `AddManifestBar`.
  - adult toggle button.
  - Browse-only category/mode filter row.
- `QScrollArea` body.
- Toast overlay for install/remove/error.
- Modal/dialog stack for install, age gate, organize.

### Discover Tab

Composition:

- Optional mosaic/backdrop band.
- `AddonHeroCard` from curated hero if available.
- Community/top addons rail.
- Starter rail.
- `AddonCategoryGrid`.
- Curated rails:
  - Essential addons
  - Streams
  - Subtitles
  - Anime
  - Catalogs/metadata
  - Tools

Trimmed category tiles:

- Keep Streaming, Catalogs, Subtitles, Anime, Torrents/Streams.
- Cut Live TV tile.
- Adult tile appears only after adult gate is enabled.

### Browse Tab

Composition:

- Mode segmented control:
  - Top rated
  - Top rising
  - Just added
- Category chips from provider, filtered through Tankoban scope.
- Search uses marketplace search when text is non-empty.
- `AddonListRow` virtualized-ish list via `QListView`/model or manual paged rows.
- Load more on scroll near bottom.

Model recommendation:

- Use `QAbstractListModel` for Browse so paging and filtering do not rebuild the whole widget tree.

### Installed Tab

Composition:

- Empty state if no installed addons.
- Search filters installed list locally.
- Reorder button opens `AddonOrganizeDialog`.
- Two-column grid/list of `InstalledAddonRow`.

Installed row actions:

- Open detail.
- Enable/disable switch.
- Manage if configurable.
- Remove.

### AddonDetailPage

Navigation:

- Pushed over the shell like other detail pages.
- Sidebar remains visible.
- Back returns to Addons tab state.

Composition:

- Hero header with logo/background, category, id, name, description.
- Action bar:
  - Install / Installed-remove / Configure & install / Reconfigure.
  - Copy manifest URL.
  - Copy `stremio://` link.
- Tags row.
- Warnings block.
- Project information:
  - version
  - resources
  - types
  - id prefixes
  - catalog count
  - P2P behavior hint
- Masked manifest URL with Reveal/Hide.
- Optional documentation text if marketplace provider supplies it.
- Optional same-category rail after the core detail page is stable.

### AddonInstallDialog

States:

- Paste URL.
- Resolving manifest.
- Manifest preview.
- Installing/updating.
- Success.
- Error.

Behavior:

- `stremio://...` converts to `https://...`.
- `/configure` and `/#/configure` normalize to base manifest.
- Base URL appends `/manifest.json`.
- Validate JSON object with `id` and `name`.
- Detect existing addon by manifest id.
- Detect likely reconfiguration by hostname match.
- Replace old entry if user confirms update/reconfigure.

### AddonOrganizeDialog

Composition:

- Full-screen or large modal dialog.
- Ordered installed addon list.
- Move up/down, move top, drag reorder if straightforward.
- Backup panel:
  - backup current order
  - restore backup into editor
  - keep last five
  - export/import JSON
- Save/cancel.

Keep this local-only. Do not mention Stremio account sync.

## Deep Links

Support:

- `stremio://.../manifest.json`
- `stremio://.../configure...` when the configured install link resolves to a manifest URL.

Qt implementation:

- Add a `DeepLinkService` abstraction.
- Windows URL protocol registration can be deferred until installer packaging exists.
- Before OS registration exists, support deep links through paste, command-line argument, and copied links.
- `MainWindow` should route addon links to `AddonInstallDialog`.

File candidates:

```text
src/core/DeepLinkService.h/.cpp
src/app/CommandLine.h/.cpp
```

## Phase Plan

### Phase 1.1: Local Registry Hardening

Goal: replace the seed `AddonClient` install assumptions with a robust local registry.

Deliverables:

- `AddonUrl` parser.
- `AddonManifestValidator`.
- `AddonRegistry` install/update/remove/enable/order.
- SQLite persistence for `installed_addons`.
- Manifest URL masking helper.

Smoke:

- Paste Cinemeta manifest/base URL.
- Invalid URL reports a useful error.
- Restart preserves installed addon.
- Enable/disable changes `enabledAddons()`.

### Phase 1.2: Minimal Addons Page Shell

Goal: Harbor-shaped Addons route with real installed management.

Deliverables:

- `AddonsPage` with Discover/Browse/Installed tabs.
- Header search field, paste bar, adult toggle placeholder.
- Installed tab reads `AddonRegistry`.
- `AddonInstallDialog`.
- `AddonToast`.

Smoke:

- Addons sidebar opens page.
- Paste manifest opens preview.
- Install succeeds and appears in Installed.
- Remove deletes it.
- Search filters Installed.

### Phase 1.3: Marketplace Providers

Goal: populate the store from public sources.

Deliverables:

- `StremioAddonsProvider`.
- `StremioDirectoryProvider`.
- Cache layer in `addon_store_cache`.
- Category normalization.
- Adult filtering.
- Failure fallback to Stremio directories.

Smoke:

- Browse Top rated loads public addons.
- Browse New and Rising load or show clean fallback.
- Category filter changes results.
- Search returns matching addons.
- Turning adult off hides adult entries.

### Phase 1.4: Discover Experience

Goal: native version of Harbor's curated storefront.

Deliverables:

- `CuratedAddonsProvider` with trimmed curated table.
- `AddonHeroCard`.
- `AddonRail`.
- `AddonCategoryGrid`.
- Discover rails from curated + public/community entries.

Smoke:

- Discover shows hero, starters, category grid, and rails.
- Install from rail updates Installed.
- Cut categories do not appear.

### Phase 1.5: Addon Detail Page

Goal: full addon detail flow.

Deliverables:

- `AddonDetailPage`.
- Manifest stats block.
- Mask/reveal/copy manifest URL.
- Copy `stremio://`.
- Configure/open browser behavior for configurable addons.
- Warnings/tags.
- Optional docs block.

Smoke:

- Opening addon card pushes detail page.
- Install/remove from detail works.
- Configurable addon opens external configure URL.
- Manifest URL is hidden by default and revealable.

### Phase 1.6: Installed Ordering and Backups

Goal: addon order becomes user-controllable and reliable.

Deliverables:

- `AddonOrderService`.
- `AddonOrganizeDialog`.
- Move up/down/top and/or drag reorder.
- Local order backups.
- Export/import order JSON.
- `CatalogService` and later `StreamService` consume registry order.

Smoke:

- Reorder installed addons.
- Save and restart preserves order.
- Restore backup changes order only after save.
- Catalog rows honor order where visible.

### Phase 1.7: Deep-Link Intake

Goal: support install links from configure pages and external sources.

Deliverables:

- `DeepLinkService`.
- Command-line argument intake for manifest/stremio URLs.
- MainWindow route to Addons install dialog.
- Optional Windows protocol registration note for installer phase.

Smoke:

- Launch app with a manifest URL argument and see install dialog.
- Paste `stremio://.../manifest.json` and install.
- Reconfigure flow can replace the old addon by id/host match.

### Phase 1.8: Store Polish and Error Pass

Goal: make the store resilient and Harbor-faithful.

Deliverables:

- Empty/loading/error states for every tab.
- Partial provider failure messaging.
- Image/logo cache fallbacks.
- Duplicate addon cleanup by id/url/normalized name.
- Keyboard focus and escape behavior for dialogs.
- Clear toasts.

Smoke:

- Network failure does not crash page.
- Bad manifest leaves existing installed addons untouched.
- Duplicate marketplace entries collapse.
- Dialog escape/cancel behaves correctly.

## Dependencies

- Phase 1.1 depends on existing `AddonClient`.
- Phase 1.2 depends on Phase 1.1.
- Phase 1.3 can start after Phase 1.1 model definitions.
- Phase 1.4 depends on Phase 1.3.
- Phase 1.5 depends on Phase 1.2 and Phase 1.3.
- Phase 1.6 depends on Phase 1.1.
- Phase 1.7 depends on Phase 1.2 install dialog.
- Phase 1.8 depends on all previous phases.

This batch feeds later roadmap batches:

- Catalog rows use installed enabled addons in registry order.
- Stream scoring uses installed enabled addons in registry order.
- Subtitle providers use installed subtitle addons.
- Debrid batch can add provider-specific configuration without reshaping the addon store.

## Acceptance Criteria

Batch 1 is complete when:

- A fresh Tankoban 3 install can open Addons and browse real marketplace/community addons.
- User can install from public Browse, Discover rail, manifest URL, and `stremio://`.
- User can update/reconfigure by replacing an existing addon.
- User can uninstall and enable/disable addons.
- User can reorder installed addons and restore/export/import local order backups.
- Restart preserves installed addons, enabled state, and order.
- `CatalogService` sees installed catalog addons after install.
- `StreamService` can later query enabled stream addons in user order without any UI changes.
- Cut surfaces are absent: account sync, Live TV/IPTV store emphasis, TMDB/API-key nudges, social/rating calls, debrid engine behavior.

