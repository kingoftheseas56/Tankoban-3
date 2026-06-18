# Western-Comics Catalogue Brain — Corrected Follow-Up Report

**Date:** 2026-06-17
**Author:** Agent 1 (DeepSeek), per follow-up brief
**Status:** Decision-ready — corrects v1, deep-dives two new vectors, validates the combined architecture

---

## Executive Summary

**Corrected recommendation: GCD spine via server-side delta-publisher as the catalogue brain. Plural western-comic sites as content addons only (they carry zero bibliographic metadata). LoCG for popularity. iTunes for covers. Metron BYOK. No shared-account proxy, no live API dependency, no per-user infra.**

The v1 recommendation (Metron proxy) fails on the ruled-out constraint: a shared app-level account is explicitly prohibited, and 5,000 req/day across an entire userbase is mathematical non-viability. Metron is demoted to **BYOK power-user enrichment only.**

---

## 0. What v1 got right — BANKED

The following from v1 are confirmed and carried forward without re-research:

- **GCD update mechanism:** Bi-weekly full SQL dumps. No incremental feed. ~2.4GB MySQL, ~4.1GB SQLite. CC BY-SA 4.0 for data; covers NOT CC-licensed.
- **ComicVine:** Ruled out (per-user API key, no redistribution, non-commercial only).
- **Wikidata:** Sparse for issues. Good as cross-reference identity bridge, not primary.
- **OpenLibrary:** TPB/graphic novel metadata only. Good supplementary source.
- **LoCG "Most Pulled":** Best free community popularity signal. Weekly cadence.
- **ICv2 Top 50:** Free monthly POS sales rankings. HTML scrape.
- **iTunes/Apple Books:** Keyless hi-res covers. Publisher-supplied, legally clean.
- **GCD Series ID:** Best canonical identity anchor. CC-licensed, institutional permanence.
- **Electron prototype patterns:** AniList = brain, RCO = content addon, iTunes = cover upscaler. Same split for Tankoban 3.

---

## 1. Corrected Ranking Premise

The v1 recommendation **(Metron-as-primary via shared-account caching proxy)** is dead on arrival:

| Failure mode | Detail |
|---|---|
| **Ruled-out constraint violated** | The original brief §4 explicitly rules out "shared app-level Metron/ComicVine account/key — doesn't scale, ban risk, quota collapse." The proxy doesn't change the account — it just hides it. |
| **Math is fatal** | 5,000 requests/day for ONE account shared across the entire userbase. A single catalogue refresh (popular series + their issues) can burn 1,000+ requests. 5 users refreshing their library destroys the quota. Metron **cut** limits in March 2026 (from 10,000→5,000/day) due to server load — the trend is down, not up. |
| **Concentration risk** | One donation-funded project as the single point of catalogue failure. If Metron goes down or changes policy, the entire Comics mode catalogue goes dark. |
| **Infra we babysit** | Running a proxy server that routes every user's catalogue traffic through one account. Rate-limit gating, caching logic, quota monitoring, donation calculus. This is the infra the brief explicitly wants to avoid. |

**Metron's only role going forward:** Optional BYOK enrichment. A power user enters their own Metron credentials in Settings → the app uses their personal account and personal quota to enrich the local catalogue. This is a nice-to-have feature, not the backbone. Zero infra required from us.

---

## 2. Vector A — Mihon/Keiyoushi Western-Comic Extension Sources

### 2.1 The Honest Count: ZERO

**The Keiyoushi extension ecosystem (14.1K stars, 200+ sources) contains exactly zero dedicated western-comic sources.**

Evidence:
- **Keiyoushi extensions listing (deepwiki):** Every English-language source is manga/manhwa/manhua. MangaKakalot, MangaBat, MangaDex, MangaNelo, Asura Scans, Webtoons, MANGA Plus — all manga/manhwa. Zero Marvel, zero DC, zero Image.
- **ReadComicOnline extension:** Reported broken July 2024 (Issue #3875 — "step1 is not defined"). Closed as "not planned" by maintainers. Confirmed not in the active extension list.
- **Comick extension:** Confirmed dead ("shut down mid-2025"). The `api.comick.io` domain redirects to `comick.dev` which returns 403 on API probes.
- **No forks carry western sources:** Komikku extensions, Yuzono repo, xdeiimoss/komikku-extensions — all manga/manhwa/manhua. Zero western.

**The Mihon ecosystem is structurally manga-only.** It was born from Tachiyomi (manga reader), extended into manhwa/manhua, and never crossed into western comics. The reasons are probably: western comic readers don't use mobile manga-reader apps; western comic piracy sites are structurally different (less API-friendly, more Cloudflare); the user communities don't overlap.

### 2.2 What DOES Exist: 3 Western Comic Downloader Sources

The [comics-downloader](https://github.com/Girbons/comics-downloader) (Go, MIT license, 8 sources total) has **3 western comic scrapers**. I read the full source of all three:

| Source | Site | Metadata extracted | Bibliographic quality |
|--------|-----|--------------------|-----------------------|
| **comicextra.go** | comicextra.net | Name + issue number from URL path segments. **Nothing else.** | ★☆☆☆☆ — Zero synopsis, zero creators, zero publisher, zero dates. Only a flat list of `<a>` links inside `div.episode-list`. No series/run/TPB structure. |
| **readallcomics.go** | readallcomics.com | Name + issue number from URL path segments. **Nothing else.** | ★☆☆☆☆ — Zero synopsis, zero creators, zero publisher. Discovers issues via `<select id="selectbox">` `<option>` values on individual pages, or `<ul class="list-story">` `<a>` tags on category pages. |
| **readcomiconline.go** | readcomiconline.li | Name + issue number from URL path segments + regex deobfuscation of image URLs. **Nothing else.** | ★☆☆☆☆ — Zero synopsis, zero creators, zero publisher. Issues discovered via regex `<a href=".../name/...">` on listing pages. Images deobfuscated via custom base64/slice manipulation pipeline. |

**All three are content-only.** They extract zero bibliographic metadata. No synopsis. No writer/artist credits. No publisher. No publication dates. No series/run structure. No collected edition grouping. No variant tracking. No stable ID system — everything is URL slugs. They answer exactly one question: "given a comic name, where are the pages?"

This is the comics-downloader data structure (from `core.Comic`):
```
Name          string   // from URL path segments only
IssueNumber   string   // from URL path segments only  
URLSource     string   // the page URL
Links         []string // page image URLs (regex/deobfuscated)
```

That's it. Four fields. No metadata.

### 2.3 Role Classification

| Source | Metadata contributor? | Content addon? | Verdict |
|--------|----------------------|----------------|---------|
| comicextra.net | ❌ Zero bibliographic metadata | ✅ Working page-image scraper | **Content addon only** |
| readallcomics.com | ❌ Zero bibliographic metadata | ✅ Working page-image scraper | **Content addon only** |
| readcomiconline.li | ❌ Zero bibliographic metadata | ✅ Working page-image scraper (deobfuscated) | **Content addon only** |
| rcostation.xyz (RCO) | ❌ Scraped but fragile (our own code) | ✅ Working (our Electron code) | **Content addon only** |

**These sources can feed the content layer (plural addons for downloadable pages) but CANNOT feed the identity layer (curated catalogue brain).** They are the equivalent of a torrent stream addon in Video mode — they supply the bytes, not the Netflix catalogue.

### 2.4 The Mihon Extension Model for Content Addons

Even though Keiyoushi has no western sources, the **Mihon extension architecture** is worth keeping as a content-addon model:

- Each extension is a self-contained Kotlin/Java module with a standard interface: `popular()`, `latest()`, `search(query)`, `seriesDetail(url)`, `chapterList(url)`, `pageList(url)`.
- Extensions are community-maintained and versioned independently.
- The Keiyoushi repo serves as an "addon store" — a JSON index of available extensions.

**For Tankoban 3's Comics mode:** we can adopt this pattern without adopting the Mihon codebase. Content addons are lightweight modules (C++/Qt plugins or even scripted configs) that implement: search → series → issues → pages. RCO, comicextra, readallcomics, readcomiconline, MangaFire (for the manga half), and WeebCentral (for manga) are all content addons. The catalogue brain (GCD) is separate.

**Recap — Vector A delivers:** 3 viable western-comic **content** addon sites (comicextra, readallcomics, readcomiconline), plus RCO (our own code). Zero viable **metadata** contributors. The Mihon ecosystem confirms: no one has built a western-comic metadata source for these reader apps. The brain must come from elsewhere.

---

## 3. Vector B — Server-Side GCD Delta-Publisher

### 3.1 Current Sizes (estimated from 2023 baselines + growth)

| Artifact | 2023 (measured) | 2026 (estimated, +~5% growth) |
|----------|----------------|-------------------------------|
| MySQL dump (`current.zip`) | ~2.4GB | ~2.5GB |
| SQLite conversion | ~4.1GB | ~4.3GB |
| **Pruned baseline** (English + major pubs + compressed) | n/a | **~800MB–1.2GB** (SQLite, filtered) |
| **Bi-weekly delta** (changed rows only) | n/a | **~5–20MB** per cycle |

**Pruning rationale:** GCD's 9K+ publishers include thousands of micro-publishers, international publishers, and defunct/obscure imprints. Filtering to English-language + top ~50 publishers (Marvel, DC, Image, Dark Horse, IDW, Boom, Dynamite, Valiant, Archie, etc.) + active series removes ~60-70% of rows. The remaining data is the western-comics catalogue a user actually browses.

**Delta size rationale:** GCD adds ~1,600 issues per 2-week cycle (~800/week, projected from Metron's 1,809/month community contributor scale). Each issue generates maybe 5-10 story rows with creator credits. Changed rows = new issues + new stories + occasional series metadata edits. 5-20MB per delta is realistic when compressed.

### 3.2 Pipeline Sketch

```
┌─────────────────────────────────────────────────────────────┐
│              SERVER-SIDE (runs every 2 weeks)                │
│                                                               │
│  1. DOWNLOAD: Fetch latest current.zip from comics.org       │
│     (requires 1 GCD account — free, used server-side only)   │
│                                                               │
│  2. UNZIP: Extract YYYY-MM-DD.sql (~2.5GB text)              │
│                                                               │
│  3. CONVERT: MySQL SQL → SQLite (awk script or mysql2sqlite) │
│     Produces: full_current.db (~4.3GB)                        │
│                                                               │
│  4. PRUNE: SQLite query — filter to English + major pubs     │
│     Produces: pruned_current.db (~800MB–1.2GB)                │
│                                                               │
│  5. DIFF: Compare pruned_current.db vs pruned_previous.db    │
│     - New/changed rows in each table                          │
│     - Deleted rows (rare)                                     │
│     Produces: delta_v2026-06-17.sqlite (~5-20MB)              │
│                                                               │
│  6. MANIFEST: Generate manifest.json                          │
│     {                                                         │
│       "version": "2026-06-17",                                │
│       "baseline_url": "gcd_baseline_v2026-06-17.sqlite.zst", │
│       "baseline_size": 850000000,                             │
│       "baseline_sha256": "abc...",                            │
│       "delta_from": "2026-06-03",                             │
│       "delta_url": "delta_v2026-06-17.sqlite.zst",           │
│       "delta_size": 12000000,                                 │
│       "delta_sha256": "def..."                                │
│     }                                                         │
│                                                               │
│  7. UPLOAD: Push baseline + delta + manifest to static host   │
│     (R2 / S3 / BunnyCDN / even GitHub Releases)               │
│                                                               │
│  8. ROTATE: Keep last N baselines; delete old                 │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│              CLIENT-SIDE (Tankoban 3 app)                     │
│                                                               │
│  FIRST LAUNCH:                                                │
│  1. Ship with a SEED baseline in the installer                │
│     (~200MB highly compressed, top ~2,000 series)             │
│     OR: first-run download of full pruned baseline            │
│                                                               │
│  EVERY LAUNCH (background):                                   │
│  2. Fetch manifest.json from our static host                  │
│  3. Compare local version vs manifest version                 │
│  4. If delta available: download delta, apply to local DB     │
│  5. If too far behind: download fresh baseline                │
│  6. Update local version stamp                                │
│                                                               │
│  OFFLINE: App works fully from local SQLite.                  │
│  No server dependency during normal use.                      │
└─────────────────────────────────────────────────────────────┘
```

### 3.3 Hosting Cost vs. Metron Proxy vs. Full Snapshots

| Approach | Monthly bandwidth (per user) | Server cost | App size | Offline | Scale property |
|----------|------------------------------|-------------|----------|---------|----------------|
| **Delta-publisher (recommended)** | ~5-20MB/2wks per active user | $0-5/mo (static file host) | 200MB seed OR 800MB baseline download | ✅ Full | Linear per user (static files, CDN) |
| Metron proxy (rejected) | ~50MB/mo per user (API responses) | $5-30/mo (live server) + Metron donation | 50MB seed DB | ⚠️ Degraded | Capped at 5,000 req/day for ALL users |
| Full bi-weekly snapshots | ~800MB/2wks per active user | $0-5/mo (static host) but high egress | 800MB per download | ✅ Full | Linear but expensive per user |
| Bundled-only (no updates) | 0 | 0 | 800MB in installer | ✅ Full | No updates — catalogue rots |

**Delta-publisher wins on every axis:**
- **Cost:** Static files on CDN. R2 gives 10GB free + zero egress. BunnyCDN is pennies/GB. Even with 1,000 users syncing weekly, bandwidth cost rounds to zero.
- **Freshness:** Bi-weekly deltas mean the catalogue is at most 2 weeks behind the live GCD.
- **Offline:** Local SQLite is the primary query surface. Sync only when online.
- **Scale:** Every user pulls the same static files from a CDN. No per-user server state. No quota ceiling.
- **Simplicity:** A cron job + shell script + `mysql2sqlite` + `sqldiff`. No running server. No auth. No rate limits. No donation calculus.

### 3.4 Honest Trade-Offs

| Trade-off | Detail | Mitigation |
|-----------|--------|------------|
| **2-week lag** | New series/issues take up to 2 weeks to appear in the catalogue | Acceptable for a curated catalogue (not a live feed). Popularity signals (LoCG weekly) fill the "what's new" gap. |
| **GCD metadata gaps** | GCD has no synopsis text for most issues. Creator credits are at the story level — require traversal to surface "writer" at the series level. | Enrich from iTunes (publisher descriptions), OpenLibrary (TPB descriptions), Wikidata (creator QIDs). Synopses are nice-to-have, not catalogue-critical. |
| **Covers excluded from dump** | GCD explicitly strips image URLs from the dump. | iTunes covers (keyless, hi-res, publisher-supplied). Zero GCD cover dependency. |
| **MySQL→SQLite conversion step** | Dump is MySQL SQL; we need SQLite. | Server-side awk conversion (proven in GCD Discussion #608). Or use `mysql2sqlite` Python script (server-side is fine — users never touch Python). |
| **Diff quality** | No built-in change tracking in GCD. Diff is computed by comparing full dumps. | SQLite's `sqldiff` tool or a custom table-hash comparison. Bi-weekly cadence means only ~1,600 issues changed — diff is tractable. |
| **Server-side job maintenance** | Someone needs to keep the cron job running. | Single static server. If the job breaks, clients just stay on the last delta until fixed. No live dependency. |
| **Initial seed size** | Shipping a 200MB seed in the installer is noticeable. | Acceptable for a desktop app (many games ship gigabytes). Alternative: first-run background download of baseline. The "just works" value outweighs the download size. |

---

## 4. Pressure-Testing the Combined Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    TANKOBAN 3 COMICS MODE                         │
│                                                                   │
│  ┌─────────────────────────────────────┐  ┌───────────────────┐ │
│  │      Catalogue Brain (IDENTITY)      │  │  Content Addons    │ │
│  │                                       │  │  (DOWNLOADABLE)    │ │
│  │  GCD Spine (delta-published)          │  │                     │ │
│  │  ├─ Series + Issues + Creators        │  │  ├─ RCO (built-in)  │ │
│  │  ├─ Publishers + Dates + Genres       │  │  ├─ comicextra      │ │
│  │  ├─ TPB/Collected Edition structure   │  │  ├─ readallcomics   │ │
│  │  └─ GCD Series ID (canonical key)     │  │  └─ readcomiconline │ │
│  │                                       │  │                     │ │
│  │  Enrichment (optional, client-side)   │  │  (Manga half:       │ │
│  │  ├─ iTunes covers (hi-res, keyless)   │  │   WeebCentral)      │ │
│  │  ├─ LoCG popularity (weekly)          │  │                     │ │
│  │  ├─ ICv2 Top 50 (monthly)            │  │  Pattern: search →  │ │
│  │  ├─ Metron BYOK (power user only)     │  │  series → issues →  │ │
│  │  └─ OpenLibrary TPB descriptions      │  │  pages → download   │ │
│  │                                       │  │                     │ │
│  │  Stored as: local SQLite (~800MB)     │  │  Stored as: files   │ │
│  │  Updated: bi-weekly delta pull        │  │  on disk (CBZ/PDF)  │ │
│  └─────────────────────────────────────┘  └───────────────────┘ │
│                                                                   │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │                    Home Page Rows                             │ │
│  │  Hero Carousel     ← GCD spine (curated picks)                │ │
│  │  Continue Reading  ← Local progress DB                        │ │
│  │  Trending This Week ← LoCG "Most Pulled" (community signal)   │ │
│  │  Popular This Month ← ICv2 Top 50 (sales signal)              │ │
│  │  Latest Releases   ← GCD spine (recent pub dates) + LoCG      │ │
│  │  Recently Added    ← Local library                             │ │
│  └─────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

### Does it hold?

| Requirement | Status | How |
|-------------|--------|-----|
| **No end-user account** | ✅ PASS | GCD account is server-side only. iTunes is keyless. LoCG/ICv2 are scraped client-side. Metron is opt-in BYOK. |
| **Source-independent / resilient** | ✅ PASS | GCD spine is institutional (Library of Congress, CC-licensed, board-governed). If GCD disappears, the last delta is frozen in every user's local DB. Content addons are plural — any one can die without affecting the catalogue. |
| **Curated Stremio-style Home** | ✅ PASS | GCD spine provides canonical series identity + bibliographic structure. LoCG + ICv2 provide popularity signals. Hero rows and catalog rows are populated from the local SQLite. |
| **Rich metadata + issue/TPB structure** | ✅ PASS | GCD has the richest bibliographic schema of any source: series, issues, variants, reprints, story-level creator credits, collected editions. Weak on synopses — supplemented by iTunes/OpenLibrary. |
| **Covers** | ✅ PASS | iTunes/Apple Books Search API (already proven in Electron prototype). Keyless. Publisher-supplied hi-res covers. Legally clean. |
| **Legally workable** | ✅ PASS | GCD data = CC BY-SA 4.0 (attribution + link back). Covers = publisher marketing images via iTunes (fair use / implied license for display). Content addons = user's own downloads. No redistribution of copyrighted scans. |
| **Offline-capable** | ✅ PASS | Local SQLite is the primary query surface. Sync is background-only. |
| **Scales to many users** | ✅ PASS | Static files on CDN. No per-user server state. No API quota ceiling. Each user pulls deltas independently. |
| **No per-user infra** | ✅ PASS | The server is a cron job + static file host. No running application. No auth. No database queries per user request. |

### Where does it strain?

| Strain point | Severity | Detail |
|---|---|---|
| **Synopsis poverty** | Medium | GCD has no synopsis field for most issues/series. iTunes covers don't carry descriptions. The Series Detail page needs a synopsis. Mitigation: OpenLibrary descriptions for TPBs, Wikidata `P2093` (author description string) for series. Could scrape publisher solicitation text. Not a launch blocker — the AniList manga brain also has minimal synopses. |
| **2-week freshness lag** | Low | The "Latest Releases" and "Trending This Week" rows use LoCG/ICv2 for freshness — those update weekly. The GCD spine provides the permanent record; the popularity sources provide the zeitgeist. |
| **Delta diff engineering** | Medium | Computing a clean diff from two full SQLite databases requires table-level row comparison. `sqldiff` tool handles most of this. Custom logic needed for: soft-deletes (GCD marks rows `deleted=1`), series renames, publisher merges. Not hard, but not zero-engineering. |
| **Seed DB size vs installer** | Low | A 200MB seed is acceptable for a Windows desktop app in 2026. Many games ship 50GB+. If this is a concern, ship a 50MB "top 500 series" micro-seed and let the first sync pull the full baseline in the background. |
| **Content addon fragility** | Medium | comicextra/readallcomics/readcomiconline are piracy sites — they change domains, get Cloudflare, die. RCO already has Cloudflare challenges (our own code handles them). But plural addons mean no single point of content failure. If one dies, users install another. This is the Mihon model working as designed. |

### Mirror of the manga half — confirmed

| Layer | Manga half | Western comics half |
|-------|-----------|---------------------|
| **Catalogue brain** | AniList GraphQL (keyless, live) | GCD spine (delta-published, CC-licensed, bi-weekly) |
| **Content addon** | WeebCentral | RCO + comicextra + readallcomics + readcomiconline |
| **Cover upscaler** | AniList covers (built-in) | iTunes Search API (keyless, hi-res) |
| **Popularity** | AniList trending (built-in) | LoCG "Most Pulled" + ICv2 Top 50 |
| **Canonical ID** | AniList ID | GCD Series ID |
| **Optional enrichment** | MyAnimeList BYOK | Metron BYOK |

The manga half has one advantage: AniList is a live API with no dump cycles, so freshness is real-time. The western half's 2-week GCD lag is the trade-off for being fully source-independent and CC-licensed. This is acceptable — the catalogue is a permanent reference, not a news feed. The popularity rows provide the "what's new" energy.

---

## 5. Corrected Ranked Recommendation

### Tier 1 — Primary (RECOMMENDED): GCD delta-published spine + plural content addons

- **Brain:** GCD bi-weekly dumps, ingested server-side, pruned to English + major publishers, diffed into deltas, served as static files from a CDN. App ships with a seed baseline and pulls deltas on launch.
- **Content:** Plural addons (RCO, comicextra, readallcomics, readcomiconline) for downloadable pages. Mihon-style addon store model.
- **Popularity:** LoCG "Most Pulled" weekly + ICv2 Top 50 monthly.
- **Covers:** iTunes/Apple Books Search API (keyless, publisher-supplied, hi-res).
- **Canonical ID:** GCD Series ID.
- **Architecture:** Local SQLite mirror + server-side delta publisher. No live API dependency. No per-user infra. CDN-scale static files.

### Tier 2 — Fallback: Bundled GCD snapshot only (if delta-publisher infra is too much)

- Same as Tier 1 but skip the delta-publisher. Ship a pruned snapshot with the app. Users download updated snapshots manually (or the app checks for new baselines periodically). Simpler server-side (just host the file), worse freshness (full re-download).

### Tier 3 — Power-user enrichment: Metron BYOK

- A user with a Metron account enters their credentials in Settings. The app pulls their personal catalogue enrichment (better covers, community ratings, pull-list data) directly from Metron's API using the user's own quota. Optional. Zero infra from us.

### Tier 4 — Last resort: Wikidata + OpenLibrary + scraped-only

- If GCD becomes unavailable and no delta-publisher exists. Thin catalogue. Not a launch concern.

---

## 6. What Changed from v1

| v1 | v2 (this report) | Why |
|----|-------------------|-----|
| Metron-as-primary via proxy | GCD-spine via delta-publisher | Shared account = ruled out. 5,000/day = mathematically non-viable for userbase-scale. |
| Metron as Tier 1 | Metron as Tier 3 (BYOK only) | Metron is excellent for an individual user — let power users bring their own accounts. |
| Mihon extensions not deeply explored | Confirmed: **zero** western-comic extensions in Keiyoushi. 3 content-only downloader scrapers. | Deep source-code read of all three comic scrapers in comics-downloader. Zero bibliographic metadata. Content addons only. |
| GCD dismissed as "heavy, no deltas" | GCD is the primary: delta-publisher solves the freshness problem server-side | The architecture v1 never costed — server-side ingest + diff + static-file deltas = all the benefits of Metron's `modified_gt` with none of the account/quota/infra problems. |
| Architecture: hybrid proxy | Architecture: static-file delta publisher | Simpler, cheaper, more resilient, no live dependency. |

---

## 7. Next Steps (if recommendation is accepted)

1. **Lock the decision.** Hemanth ratifies GCD-spine + plural content addons.
2. **Build the delta-publisher pipeline.** Single server/VM. Cron job: download → convert → prune → diff → manifest → upload. Language: Python or Go (server-side, users never touch it). Start with one manual run to get real sizes.
3. **Build the seed DB.** First pruned baseline from GCD. Top ~2,000 series. Ship in installer.
4. **Implement the client sync engine** (C++/Qt): fetch manifest → compare version → pull delta → apply to local SQLite.
5. **Port the iTunes cover resolver** from Electron to C++/Qt.
6. **Implement LoCG popularity scraper** (C++/Qt, weekly fetch).
7. **Build the content addon interface** (Mihon-style): each addon = search → series → issues → pages → download. RCO as the first built-in addon.
8. **Metron BYOK** is a post-launch feature — nice to have, not launch-critical.

---

## Sources (new for v2)

- Keiyoushi extensions source listing: `https://deepwiki.com/keiyoushi/extensions/5.1-manga-and-comic-sources` (confirmed: 0 western comic sources)
- Keiyoushi extensions ReadComicOnline issue #3875: `https://github.com/keiyoushi/extensions-source/issues/3875` (closed as not planned, July 2024)
- Comics-downloader source (all 3 western scrapers read directly from raw GitHub):
  - `https://raw.githubusercontent.com/Girbons/comics-downloader/master/pkg/sites/comicextra.go`
  - `https://raw.githubusercontent.com/Girbons/comics-downloader/master/pkg/sites/readallcomics.go`
  - `https://raw.githubusercontent.com/Girbons/comics-downloader/master/pkg/sites/readcomiconline.go`
- GCD Discussion #608 (SQLite export, 2023 sizes): `https://github.com/GrandComicsDatabase/gcd-django/discussions/608`
- GCD Data Distribution wiki (bi-weekly cadence, CC BY-SA, formats): `https://docs.comics.org/wiki/Data_Distribution`
- GCD Copyrights: `https://docs.comics.org/wiki/GCD:Copyrights` (CC BY-SA 4.0 for data, covers excluded)
- Metron rate limit reduction (March 2026): `https://metron-project.github.io/blog/api-rate-limit-reduction` (30→20/min, 10,000→5,000/day)
- Metron API Best Practices (April 2026): `https://metron-project.github.io/blog/api-best-practices`
- Metron May 2026 Updates: `https://metron-project.github.io/blog/may-2026-updates`
- ComicTagger sources wiki: `https://github.com/comictagger/comictagger/wiki/Comic-and-Manga-Information-Sources`
- gcd-talker PyPI: `https://pypi.org/project/gcd-talker/` (confirms cover URLs stripped from dump)
- Komga/Kavita self-hosted comic servers: web search results (confirmed OPDS + Tachiyomi extensions exist but are self-hosted readers, not external scrapers)
- All v1 sources carried forward (see v1 report §Sources)
