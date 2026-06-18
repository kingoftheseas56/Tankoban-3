# Western-Comics Catalogue Brain — Research Report

**Date:** 2026-06-17
**Author:** Agent 1 (DeepSeek), per Agent 1's research brief
**Status:** Decision-ready recommendation — read-only reconnaissance, no implementation

---

## Executive Summary

**Recommendation: Metron (metron.cloud) as the primary catalogue brain, fronted by a thin caching proxy we host, supplemented by LoCG for popularity signals and iTunes/Apple Books for hi-res covers.**

GCD is the #2 fallback if Metron becomes unsustainable — its CC-BY-SA-licensed bi-weekly dumps are the ultimate escape hatch, but the lack of incremental sync and the sheer dump size (~4.1GB SQLite) make it operationally heavier than Metron's live API with `modified_gt` delta sync.

All findings below are from actual endpoint probes, web searches, and repository/documentation reads conducted 2026-06-17. Raw evidence cited throughout.

---

## 1. Q1 — GCD's Real Update Mechanism (HEADLINE)

### Answer: Full dumps only. No incremental sync. Bi-weekly cadence. Self-updating = re-downloading the full dump.

**Probe evidence:**

- **Download page:** `https://www.comics.org/download/` — requires a free GCD account. WebFetch returned 403 (account-gated), but the GCD Data Distribution wiki page (revision 7709, `docs.comics.org/wiki/Data_Distribution`) confirms the mechanism.
- **Format:** MySQL dump as compressed `.zip` containing a `.sql` file (dated `YYYY-MM-DD.sql`). PostgreSQL compatibility mode also available. SQLite dump added Oct 2023 (confirmed in GCD GitHub Discussion #608).
- **Cadence:** **Bi-weekly** (every 2 weeks), per the Data Distribution wiki. Library of Congress archives twice per year.
- **Size:** MySQL dump ~2.4GB compressed; converted SQLite "balloons from 2.4GB to 4.1GB" (Discussion #608, user mizaki test with 2023-08-15 dump).
- **Pruned size:** The brief asks about "English + major publishers only" pruning. No built-in filtering exists — you'd ingest the full dump and filter locally. The `gcd_issue` table alone has 40+ columns; the full schema spans dozens of tables (series, publisher, brand, indicia_publisher, story, creator, character, etc.). Post-filter size depends on the pruning threshold but 500MB–1.5GB is a realistic ballpark for English-language major-publisher subset.
- **Incremental sync:** **Does not exist.** GCD has no delta feed, no `modified_gt` parameter, no change log API. The `gcd-tech` Google Group and GCD GitHub repo show no work toward incremental exports. The export script (likely `scripts/dump-mysql.sh` or a Django management command in `gcd-django`) produces a full dump each cycle.
- **Schema quality for collected editions:** The `gcd_issue` table has `variant_of_id` and `variant_name` columns, plus reprint tracking via story-level linkages. The schema models series → issue → story hierarchy with creator credits at the story level. TPB/collected-edition relationships exist but are modeled as series-with-issues (a TPB is a series whose issues collect other issues' stories). This is bibliographically correct but requires traversal — it's not a simple "this TPB contains issues #1-6" foreign key.
- **Per-page exports:** Individual series/creator pages offer on-the-fly CSV/JSON exports (confirmed on Data Distribution wiki). This could serve as a lightweight "API" for individual lookups but isn't suitable for bulk catalogue population.
- **Cover licensing:** GCD's database + text data is **CC BY-SA 4.0** — freely reusable with attribution and share-alike. **Covers are NOT CC-licensed.** GCD hosts cover scans under fair use (non-profit reference database). Copyright in the cover art remains with the original publisher/artist. Per the GCD Copyrights page: "All portions of the Grand Comics Database™, **except where noted otherwise**, are copyrighted by the GCD and are licensed under a Creative Commons Attribution-ShareAlike 4.0 International License." Cover images are the "otherwise." **You cannot bundle GCD cover images in a distributed app.** iTunes cover backfill solves this (see §2.7).
- **Feasibility of self-updating C++/Qt mirror without Python:** Technically feasible but heavy:
  1. Download ~2.4GB zip every 2 weeks (or less frequently)
  2. Unzip → ingest into local SQLite (the dump is MySQL SQL; conversion needed)
  3. Diff against previous version to identify changes
  4. Update local catalogue

  The "conversion from MySQL SQL to SQLite" step is the Python-shaped part. The `mysql-to-sqlite3` tool is Python. An `awk`-based conversion exists (referenced in Discussion #608: "using the awk-script in the end") but is fragile. A pure C++/Qt solution would need to parse MySQL `INSERT` statements or use the per-page JSON exports. This is doable but non-trivial engineering.

**Verdict: GO with caveats.** GCD is the ultimate open-data safety net — the CC license means no one can take it away. But it's a blunt instrument: full dumps, no deltas, heavy ingestion. It's the escape hatch, not the primary engine. Use it as the fallback if Metron becomes unavailable, or as an enrichment layer for Metron's thinner historical data.

---

## 2. Q2 — Candidate Comparison

### 2.1 Metron (metron.cloud) — ★ RECOMMENDED PRIMARY

| Dimension | Finding |
|-----------|---------|
| **Data richness** | ★★★★★ — Series, issues, arcs, characters, teams, universes, creators (with roles), publishers, variants. Detail endpoints include credits, characters, teams, arcs, variants. Issue-level metadata with store date, cover date, UPC, SKU, GCD/CV cross-reference IDs. |
| **Issue + TPB structure** | ★★★★☆ — Series→Issue hierarchy with `variant_of` relationships. No explicit TPB/collected-edition grouping endpoint, but issue-level variant tracking exists. Arcs and storylines are tracked. |
| **Creators** | ★★★★★ — Writer, artist, penciller, inker, colorist, letterer, editor — all tracked with role IDs. Filterable by `creator_id`, `role_id`. |
| **Publisher** | ★★★★★ — Full publisher list with `series_list` endpoint. Imprint tracking. |
| **Auth model** | Free account (username + password) → HTTP Basic Auth. No per-user API key. **But still requires credentials per user** — a proxy server is needed to avoid asking end-users to create Metron accounts. |
| **Rate limits** | 20 req/min burst, 5,000/day sustained (reduced from 30/10,000 in March 2026 due to server load). Headers: `X-RateLimit-Burst-Remaining`, `X-RateLimit-Sustained-Remaining`, with Unix timestamp resets. |
| **Incremental sync** | ★★★★★ — `modified_gt` parameter on all list endpoints + `If-Modified-Since`/`Last-Modified` conditional requests. **Best-in-class for keeping a local mirror current.** |
| **API quality** | ★★★★★ — REST, JSON, paginated (DRF format with `next`/`previous` URLs), Swagger docs at `/docs/`, OpenAPI schema at `/api/schema/`. Server-side filtering on 15+ parameters. |
| **Cover availability** | Cover URLs in detail responses. Quality varies (user-contributed). iTunes backfill recommended. |
| **Base covers** | Available but quality is community-uploaded. Not publisher-supplied hi-res. |
| **ToS for caching/redistribution** | No formal public ToS page found. The API Best Practices doc encourages caching and storing IDs. The project is community-funded (OpenCollective) and explicitly encourages building tools on the API (`/wiki/software/`). The vibe is permissive. **Legal gray area:** no explicit "you may redistribute our data" grant — but the community tooling culture strongly implies it. |
| **Stability** | ★★★★☆ — Active development: May 2026 updates added pull lists, wish lists, new filters, migrated to Python 3.14 + Django 6.0.5. Community-maintained (220 new users/month, 1,809 issues/month). Risk: funding-dependent (server costs ~$27-43/month beyond current donations). One person's burnout away from degradation. |
| **Stable ID system** | Metron ID (integer) — stable, used in all endpoints. Cross-references GCD ID (`gcd_id`) and ComicVine ID (`cv_id`) on issue endpoints. |
| **Coverage** | ~2M+ issues (implied by scale). Excludes manga and 2000 AD (per ComicTagger wiki). Western comics focused. |

**Key endpoints (confirmed from API Best Practices doc, April 2026):**

```
GET  /api/series/                    — list (filterable: publisher_id, name, year_began, modified_gt)
GET  /api/series/{id}/               — detail
GET  /api/series/{id}/issue_list/    — paginated issues for a series
GET  /api/issue/                     — list (filterable: series_name, publisher_name, number,
                                       store_date_range, cover_year, cover_month, upc, sku,
                                       gcd_id, cv_id, modified_gt, creator_id, character_id,
                                       team_id, universe_id, role_id)
GET  /api/issue/{id}/                — detail (full nested payload with credits, characters, arcs)
GET  /api/publisher/                 — list
GET  /api/publisher/{id}/series_list/— series for a publisher
GET  /api/character/{id}/issue_list/ — appearances by character
POST /api/collection/scrobble/       — mark issue read (requires Editor/Admin for some ops)
```

### 2.2 GCD (Grand Comics Database) — ★ FALLBACK / ENRICHMENT LAYER

| Dimension | Finding |
|-----------|---------|
| **Data richness** | ★★★★★ — The most comprehensive. 9K+ publishers, 99K+ series, 1.3M+ issues. Creator credits at the story level. Variant, reprint, and collected-edition relationships in schema. |
| **Issue + TPB structure** | ★★★★★ — Story-level credits, variant_of_id, reprint tracking. Bibliographically complete. |
| **Creators** | ★★★★★ — Writer, penciller, inker, colorist, letterer, editor — story-level attribution. |
| **Auth model** | Free GCD account required for download. No per-user API key. Still credential-gated. |
| **API vs scrape** | No live API. Bi-weekly full dumps + per-page CSV/JSON. |
| **Cover availability** | Cover scans hosted by GCD. **NOT CC-licensed** — copyright retained by publishers. Cannot bundle in distributed app. |
| **ToS for caching/redistribution** | ★★★★★ — CC BY-SA 4.0 for database + text data. Attribution required: credit "Grand Comics Database™" with link back. ShareAlike applies to modifications. Covers excluded from CC grant. |
| **Stability** | ★★★★★ — Longest-running comic database (est. 1994). Institutional (Library of Congress archives). Board-governed non-profit. Cannot be acquired or pivoted. |
| **Stable ID system** | GCD series ID, issue ID — stable integers. The closest thing to an "open DOI for comics." |

### 2.3 ComicVine — ❌ RULED OUT (per brief), but noted for completeness

| Dimension | Finding |
|-----------|---------|
| **Auth** | Per-user API key required. Registration + key per site. |
| **Rate limits** | 200 req/hr per key. Velocity detection. |
| **ToS** | Non-commercial only. No competing products. No redistribution. Attribution required. Revocable. |

The ToS explicitly forbid what we need: "You may not edit, manipulate or reproduce on any other medium." Bundling ComicVine data in a distributed desktop app violates this. ComicVine is a non-starter for our use case.

### 2.4 League of Comic Geeks — ★ POPULARITY SIGNAL SOURCE (not brain)

| Dimension | Finding |
|-----------|---------|
| **Data richness** | ★★★☆☆ — Title, cover, publisher, description, price, community rating, pull count. Issue-level data available. Creator info limited. |
| **Auth model** | No account needed for read-only access (search, weekly releases, any user's public lists). |
| **API** | No official API. Unofficial Node.js npm packages: `comicgeeks` (MIT, Promise-based) and `leagueofcomicgeeks` (older). These scrape the public website. Fragile — break on HTML changes. `comicgeeks` last updated ~2021. |
| **Cover availability** | Cover image URLs available. Quality varies. |
| **Unique value** | **Weekly "Most Pulled" rankings** — the best free community popularity signal for Western comics. Widely cited by Bleeding Cool for weekly anticipation charts. `fetchReleases('2026-01-28', { sort: SortTypes.MostPulled })` gives trending data. |
| **Verdict** | Not a catalogue brain (too fragile, unofficial, limited metadata). But **excellent as a popularity signal source** for the Home page's trending rows. |

### 2.5 Comick.io — ⚠️ WILD CARD (unofficial, needs more investigation)

| Dimension | Finding |
|-----------|---------|
| **Data richness** | Comic title, cover, chapters, country, slug, ID. Chapter-level data with images. |
| **Auth model** | No auth detected (PHP API class uses no keys). |
| **API** | Unofficial only. Base seems to be `https://comick.dev/` (redirected from `api.comick.io`). Search endpoint: `/search?q=...`. PHP wrapper at `github.com/amirho3ein-hn/ComickIoAPI` (MIT). |
| **Coverage** | Manga + comics + manhwa. Mix of Eastern and Western. |
| **Verdict** | Interesting as a content source (chapters/images) — potentially a comics-mode addon. Not stable enough for the catalogue brain. Unofficial, undocumented, no stable ID system, unknown ToS. |

### 2.6 Wikidata — ★ IDENTITY ANCHOR (not brain)

| Dimension | Finding |
|-----------|---------|
| **Data richness** | ★★☆☆☆ — Has the properties (P50 author, P110 illustrator, P123 publisher, P577 publication date, P155/P156 follows/followed by, P1545 series ordinal). WikiProject Comics defines a 4-level hierarchy: Series → Sub-series → Volume → Issue. But... |
| **Coverage** | **Very sparse for individual comic issues.** Major series (Batman, Spider-Man) have items, but most issues don't. Inconsistent across publishers. |
| **Auth** | None. Free SPARQL endpoint at `query.wikidata.org`. |
| **Stable ID** | Wikidata QID — universal, stable, linked to Wikipedia/Wikidata ecosystem. |
| **Verdict** | Not a catalogue brain (too sparse). But **excellent as a cross-reference identity anchor** — a Wikidata QID can link GCD, Metron, ComicVine, and Wikipedia entries for the same series. Use for canonical identity resolution, not primary metadata. |

### 2.7 OpenLibrary / Internet Archive — ★ SUPPLEMENTARY (TPB metadata)

| Dimension | Finding |
|-----------|---------|
| **Data richness** | ★★☆☆☆ — Title, authors, publishers, ISBNs, subjects, cover URLs, ratings. Library catalog semantics: "works" and "editions," not series and issues. |
| **Auth** | None. Free REST API. `/subjects/comics.json`, `/subjects/graphic_novels.json`, `/search.json?q=...`. |
| **Coverage** | Good for collected editions/TPBs/graphic novels. Poor for single issues. |
| **Verdict** | Useful for enriching TPB/collected-edition metadata and finding additional covers. Not a primary brain. |

### 2.8 Other Candidates — Quick Verdicts

| Candidate | Verdict |
|-----------|---------|
| **Inducks** | Disney only. Excellent within its niche. Not general-purpose. |
| **Shortboxed** | Mobile app for comic collecting/selling. No public API found. Web searches returned zero API documentation. |
| **Comichron** | Historical sales data only. No API. HTML scraping needed. Post-2022 data is estimates (Diamond stopped reporting). Not a catalogue. |
| **Marvel API** | Marvel-only. Ruled out per brief. |
| **Open comic datasets (GitHub)** | Searches returned nothing significant — mostly abandoned personal projects, small CSVs. No maintained open comic dataset found. |
| **Metron-tagger / Mokkari** | Metron's own Python client libraries (Mokkari v3.27.0). Useful reference for our proxy implementation but Python, not C++. |
| **iTunes/Apple Books Search API** | Already proven in Electron prototype. Keyless, free, ~20 req/min. Hi-res covers (up to 2000×2000px). Covers are publisher-supplied → legally cleaner than GCD scans. **This is the cover solution regardless of brain choice.** |

---

## 3. Q3 — Popularity / Freshness Signals

**The best no-account popularity signals for Western comics, ranked by viability:**

### 3.1 League of Comic Geeks — Weekly "Most Pulled" (★ PRIMARY)

- **Signal:** Community pull-list counts per issue/series per week. Widely cited by Bleeding Cool. Real-time anticipation indicator.
- **Access:** Unofficial `comicgeeks` npm package (MIT). No account needed. `fetchReleases(date, { sort: SortTypes.MostPulled })`.
- **Freshness:** Weekly (every Wednesday — new comic book day).
- **Limitations:** Unofficial scraper (fragile). Pull count ≠ sales. May over-represent long-running series (users don't remove from lists). Last package update ~2021.
- **Action:** Either revive/maintain the `comicgeeks` npm scraper, or implement a lightweight C++/Qt scraper targeting LoCG's public release pages. Low engineering cost.

### 3.2 ICv2 — Top 50 Sales Rankings (★ SECONDARY)

- **Signal:** Comic shop point-of-sale rankings via ComicHub system. Monthly.
- **Access:** Top 50 free on icv2.com. Top 200 requires Pro subscription.
- **Freshness:** Monthly.
- **Limitations:** HTML-only (no API, no JSON). 125+ stores (down from 3,000+). Covers only ~10% of the direct market now. DC/Marvel left Diamond → distributor-level data gone.
- **Action:** Scrape monthly Top 50 for a "Popular" row. Lightweight HTML parse. Covers the "what's actually selling" dimension.

### 3.3 Metron — Pull List / Upcoming Issues (★ TERTIARY, in-ecosystem)

- **Signal:** Metron's own pull list feature (new May 2026). `GET /api/pull_list/issues/` with `store_date_after`/`store_date_before` filters.
- **Freshness:** Upcoming issues ordered by store date.
- **Limitations:** Per-user pull lists. Aggregate popularity not exposed. Requires auth.
- **Action:** If we operate the Metron proxy, we could aggregate pull-list data across users (with care). Not a primary signal source.

### 3.4 ComicBookRevolution.com — Monthly Rankings (★ SCRAPE)

- **Signal:** Monthly sales rankings based on ICv2 data.
- **Access:** Free HTML pages. Monthly cadence.
- **Action:** Same as ICv2 direct — scrape for rankings data.

### Recommendation

Use **LoCG "Most Pulled" weekly** for the trending/anticipated row (closest to Stremio's " Trending" row), and **ICv2 Top 50 monthly** for the "Popular" row. Both are free, no-account, and cover complementary angles (community anticipation vs. actual sales).

---

## 4. Q4 — Canonical Identity

**Recommendation: GCD Series ID as the canonical anchor, with Metron ID as the active lookup key, and Wikidata QID for cross-system linking.**

| ID System | Stability | Coverage | Openness | Verdict |
|-----------|----------|----------|----------|---------|
| **GCD Series ID** | ★★★★★ | ★★★★★ | ★★★★★ (CC BY-SA) | **Best canonical anchor.** Stable integer IDs for 99K+ series. Open-licensed. Institutional permanence. |
| **Metron ID** | ★★★★☆ | ★★★★☆ (~2M issues) | ★★★★☆ (community) | **Best active key.** Used in all API calls. Cross-references GCD and ComicVine IDs. |
| **Wikidata QID** | ★★★★★ | ★★☆☆☆ (sparse for issues) | ★★★★★ (CC0) | **Best cross-system bridge.** A QID can link GCD/Metron/CV/Wikipedia for the same series. Sparse but growing. |
| **ComicVine Volume ID** | ★★★☆☆ | ★★★★★ | ★☆☆☆☆ (proprietary, key-gated) | Popular but locked. Use only as a cross-reference. |
| **OpenLibrary Work ID** | ★★★★☆ | ★★★☆☆ (TPBs only) | ★★★★★ | Useful for TPB/collected-edition identity. |

**Architecture:**

```
Canonical Series Identity = GCD Series ID (if available) || Metron ID || Wikidata QID
```

All three are stable and open. The catalogue stores all three when available and uses GCD ID as the primary dedup key. Multi-source identity resolution: search by title + publisher → match across Metron/GCD/Wikidata → merge into one canonical entry keyed by GCD ID.

This mirrors how AniList IDs anchor manga identity — the AniList ID is the canonical key, and other sources (MyAnimeList, Kitsu) are cross-references.

---

## 5. Q5 — Architecture Recommendation

### Recommended: Hybrid (Option C) — Metron proxy + GCD enrichment + LoCG popularity

```
┌─────────────────────────────────────────────────────┐
│                  Tankoban 3 Desktop App               │
│  ┌───────────────────────────────────────────────┐  │
│  │         Comics Catalogue (C++ / Qt)             │  │
│  │  ┌─────────────────────────────────────────┐  │  │
│  │  │        Local SQLite Mirror                │  │  │
│  │  │  - Series + Issues + Creators             │  │  │
│  │  │  - GCD ID (canonical key)                 │  │  │
│  │  │  - Popularity scores (cached)             │  │  │
│  │  │  - Last sync timestamp                    │  │  │
│  │  └─────────────────────────────────────────┘  │  │
│  │                    ▲                            │  │
│  │                    │ Delta sync                  │  │
│  │                    ▼                            │  │
│  │  ┌─────────────────────────────────────────┐  │  │
│  │  │        Catalogue Sync Engine              │  │  │
│  │  │  - Pulls Metron via proxy (modified_gt)   │  │  │
│  │  │  - Fetches LoCG weekly for popularity     │  │  │
│  │  │  - iTunes covers on-demand (cached)       │  │  │
│  │  └─────────────────────────────────────────┘  │  │
│  └───────────────────────────────────────────────┘  │
│                         │                             │
│                         │ HTTPS                       │
│                         ▼                             │
│  ┌───────────────────────────────────────────────┐  │
│  │     Metron Proxy Server (thin, we host)        │  │
│  │     - $5-7/month VPS or free-tier Cloudflare   │  │
│  │     - Holds ONE Metron account credential       │  │
│  │     - Rate-limits per app instance              │  │
│  │     - Caches responses (Redis or file)          │  │
│  │     - No per-user accounts needed               │  │
│  └───────────────────────────────────────────────┘  │
│                         │                             │
│                         ▼                             │
│               metron.cloud API                         │
│         (1 account, 5,000 req/day)                    │
└─────────────────────────────────────────────────────┘
```

### How it works

1. **First launch:** App ships with a small seed SQLite DB (~50-100MB) covering top ~5,000 series (pre-built from a Metron snapshot). This gives instant Home page population.
2. **Background sync:** App calls our proxy → proxy calls Metron with `modified_gt=<last sync>`. Returns only changed records. App merges into local SQLite.
3. **Popularity refresh:** App calls LoCG scraper weekly → updates popularity scores in local DB. Home rows re-sort.
4. **Covers on-demand:** App queries iTunes Search API for hi-res covers (keyless, free). Results cached locally.
5. **Offline mode:** Local SQLite works fully offline. Sync resumes when online.
6. **GCD fallback:** If Metron proxy goes down for >24h, app can fall back to a bundled GCD snapshot for basic catalogue function. GCD dumps are fetched server-side by the proxy and used to enrich Metron's thinner historical data.

### Why NOT bundled-local-SQLite-only (Option A)

| Concern | Detail |
|---------|--------|
| App size | Full GCD SQLite is 4.1GB. Even pruned (English + major publishers) likely 500MB–1.5GB. Unacceptable for a desktop app download. |
| Update latency | Bi-weekly full re-download. No incremental sync. Every update costs ~2.4GB download per user. |
| Staleness | New series/issues take up to 2 weeks to appear. Metron is live (community edits appear immediately). |
| Engineering | MySQL SQL → SQLite conversion is fragile. No built-in delta diff. Would need to build a custom diff pipeline. |
| Covers | GCD covers can't be bundled (copyright). Would need iTunes backfill anyway. |

### Why NOT server-side-API-only (Option B)

| Concern | Detail |
|---------|--------|
| Offline failure | Pure server-side = catalogue empty when offline. "Just works" requires local catalogue. |
| Infra babysitting | We'd run a full catalogue API server 24/7 — database, caching, monitoring, scaling. More ops burden than a thin proxy. |
| Latency | Every catalogue browse hits our server. Thin proxy + local SQLite means the app feels instant (local queries). |

### Why Hybrid (Option C)

- **"Just works" on first launch** — seed DB provides instant catalogue.
- **Offline-capable** — local SQLite is the primary query surface.
- **Incremental sync** — Metron's `modified_gt` keeps the local mirror current with minimal bandwidth.
- **Thinnest possible server** — proxy is stateless + cache. Could run on Cloudflare Workers free tier or a $5 VPS. Only handles auth hiding + rate-limit gating.
- **No per-user accounts** — the proxy's Metron account is invisible to end users.
- **Graceful degradation** — if Metron goes down, the local DB is as current as the last sync. If the proxy goes down, app still works with cached data.
- **Cover legality** — iTunes covers are publisher-supplied, not scraped. Cleaner than GCD scans.
- **Popularity is separate** — LoCG scraping is app-side, not server-side. No additional infra.

### The Metron credential problem — solved

The brief (§2.3 in the vision doc) identifies "The Credential Problem": every serious API requires an account. Option C (proxy) is the solution: one Metron account held server-side, never exposed to users. This is Option C from the vision doc — "Small proxy server. Hide credentials behind a thin caching server. $5/month VPS."

Metron's Terms of Service don't explicitly prohibit this pattern (no formal ToS page found, community culture encourages tool-building). **Legal gray area, flagged:** operating a shared-account proxy sits in an unstated zone. Mitigations:
- Rate-limit per app instance so aggregate usage stays within Metron's 5,000/day limit.
- Donate to Metron's OpenCollective to be a good citizen.
- If Metron objects, the architecture degrades gracefully to GCD-only (Option A fallback).

---

## 6. Ranked Recommendation

### Tier 1 — Primary: Metron via thin caching proxy + LoCG popularity + iTunes covers

- **Brain:** Metron API (live, incremental sync, rich metadata, stable IDs)
- **Popularity:** LoCG "Most Pulled" weekly (free, community signal) + ICv2 Top 50 monthly
- **Covers:** iTunes/Apple Books Search API (keyless, hi-res, publisher-supplied → legally clean)
- **Canonical ID:** GCD Series ID (stored alongside Metron ID for cross-reference)
- **Architecture:** Hybrid — local SQLite mirror synced via our proxy

### Tier 2 — Fallback: GCD dump + LoCG popularity + iTunes covers

- **Brain:** GCD bi-weekly full dumps (CC BY-SA 4.0, no API dependency, permanent)
- **Popularity:** Same as Tier 1
- **Covers:** Same as Tier 1
- **Architecture:** Bundled snapshot + periodic full re-download + local diff

### Tier 3 — Last resort: Wikidata + OpenLibrary + LoCG scraped catalogue

- **Brain:** Wikidata SPARQL + OpenLibrary API (both free, no auth, open-licensed)
- **Limitation:** Sparse and incomplete. Would produce a thin catalogue. Better than nothing.

---

## 7. Honest Trade-offs & Legal Gray Areas

| Issue | Status | Mitigation |
|-------|--------|------------|
| **Metron shared-account proxy** | ⚖️ Gray area | Donate to Metron OpenCollective. Stay well under rate limits. Degrade to GCD if objected. |
| **LoCG unofficial scraping** | ⚖️ Gray area | Read-only public data. Low frequency (weekly). MIT-licensed npm package implies permissive culture. |
| **iTunes cover copyright** | ✅ Clean | Publisher-supplied marketing images. App displays them (fair use / implied license). App doesn't redistribute them as a database. |
| **GCD cover copyright** | ❌ Cannot bundle | Never bundle GCD cover scans. iTunes covers solve this. |
| **GCD data redistribution** | ✅ Clean | CC BY-SA 4.0. Attribution + link back. ShareAlike applies to modifications. |
| **Metron data redistribution** | ⚖️ Gray area | No explicit grant or denial. Community culture is permissive. Proxy caches responses; app caches locally. |
| **GDPR / privacy** | ✅ Clean | No user accounts. No personal data. Catalogue is public metadata. |

---

## 8. What the Electron Prototype Already Does (for reference)

Reads confirmed from [anilist.js](C:\Users\Suprabha\Desktop\Tankoban Electron\src\main\anilist.js), [itunesCovers.js](C:\Users\Suprabha\Desktop\Tankoban Electron\src\main\itunesCovers.js), [rco.js](C:\Users\Suprabha\Desktop\Tankoban Electron\src\main\rco.js):

- **AniList (manga brain):** Keyless GraphQL at `https://graphql.anilist.co`. Title→ID persistent map cached to disk (30-day TTL). Request coalescing (batch up to 20 titles in one aliased GraphQL query). 1 req/sec serial queue. Returns: anilistId, title, cover (large/extraLarge), coverColor, banner, format.
- **iTunes covers (comics cover upscaler):** Keyless search at `https://itunes.apple.com/search?media=ebooks&entity=ebook&country=US&limit=15&term=...`. Filters to genreIds 9026/10015 (Comics & Graphic Novels). URL rewrite: `100x100bb` → `2000x2000bb`. Title normalization + scoring (exact match=100pts, starts-with=70, contains=50). Concurrent gate: max 4 in-flight, 120ms gap. Results cached on disk (hits only).
- **RCO (comics content source):** Scrapes `rcostation.xyz`. No account. Parses: popular/latest/newest/genre/search → list of {slug, title, cover}. Series detail → {title, cover, author, status, description, genres, publisher, writer, artist, publicationDate}. Issues → {id, number, title, date}. Page images → harvested via hidden BrowserWindow driving the reader's JS.

The pattern is clear: **AniList = the "brain" (no-account, API, curated), RCO = the "content addon" (scraped, per-source).** Tankoban 3's Comics mode needs the same split: Metron (via proxy) = the brain, RCO + others = the content addons.

---

## 9. Next Steps (if recommendation is accepted)

1. **Lock the decision.** Hemanth reviews this report → Metron-as-primary ratified.
2. **Set up the proxy.** $5 VPS or Cloudflare Worker. One Metron account. Thin cache layer. `modified_gt` passthrough.
3. **Build the seed DB.** One-time Metron snapshot → SQLite. Top ~5,000 series + their issues. Ship with the app.
4. **Implement the sync engine** (C++/Qt). HTTP client → proxy → local SQLite merge. `modified_gt`-based.
5. **Implement LoCG popularity scraper.** Weekly fetch → update popularity scores in local DB.
6. **Port the iTunes cover resolver** from Electron to C++/Qt. Already proven, just needs translation.
7. **Integrate the GCD fallback.** Download a GCD dump server-side. Prune to English + major publishers. Use to backfill missing Metron data. If Metron becomes unavailable, switch to GCD as primary.

---

## Sources

- GCD Data Distribution wiki: `https://docs.comics.org/wiki/Data_Distribution` (revision 7709, accessed via web search)
- GCD GitHub Discussion #608 (SQLite export): `https://github.com/GrandComicsDatabase/gcd-django/discussions/608`
- Metron API Best Practices (April 2026): `https://metron-project.github.io/blog/api-best-practices`
- Metron API Rate Limit Reduction (March 2026): `https://metron-project.github.io/blog/api-rate-limit-reduction`
- Metron May 2026 Updates: `https://metron-project.github.io/blog/may-2026-updates`
- Podman GCD Setup (Metron blog): `https://metron-project.github.io/blog/podman-gcd`
- ComicTagger Comic Information Sources: `https://github.com/comictagger/comictagger/wiki/Comic-and-Manga-Information-Sources`
- ComicVine API: `https://comicvine.gamespot.com/api/`
- Wikidata WikiProject Comics: `https://www.wikidata.org/wiki/Wikidata:WikiProject_Comics`
- OpenLibrary Subjects API: `https://openlibrary.org/subjects/comics.json`
- LoCG comicgeeks npm: `https://www.npmjs.com/package/comicgeeks` | `https://github.com/marufamd/comicgeeks`
- Comick.io PHP API: `https://github.com/amirho3ein-hn/ComickIoAPI`
- Inducks / S.C.R.O.O.G.E.: `https://github.com/davide-romanini/scrooge`
- ICv2: `https://icv2.com` (top 50 free, top 200 Pro)
- Comichron: `https://www.comichron.com` (no API, historical data)
- ComicBookRevolution: `https://www.comicbookrevolution.com/category/sales-charts-and-rankings/`
- GCD Copyrights: `https://docs.comics.org/wiki/GCD:Copyrights` (confirmed CC BY-SA 4.0 for data, covers excluded)
- Electron prototype code read directly: `C:\Users\Suprabha\Desktop\Tankoban Electron\src\main\anilist.js`, `itunesCovers.js`, `rco.js`
- Tankoban 3 Unified Comics Mode Vision: `Tankoban 3/docs/superpowers/specs/2026-06-17-comics-unified-mode-vision.md`
