# RCO + iTunes — Enrichment Contract (reverse-engineered from the Tankoban Electron prototype)

Date: 2026-06-18
Author: Agent 1 (Opus), from a direct read of the working Electron prototype
Source files: `Tankoban Electron/src/main/rco.js`, `src/main/itunesCovers.js`,
`src/renderer/src/lib/comics.js`, `src/renderer/src/pages/comics/ComicsSeries.jsx`

Purpose: document *exactly* what ReadComicOnline (rcostation.xyz) exposes and what the iTunes
cover resolver does — primary evidence that corrects the v2 research report's "western sites are
content-only / zero metadata" claim.

---

## 1. The headline correction

The v2 report read the **comics-downloader** (a bare Go *downloader*) and concluded the western sites
carry "zero bibliographic metadata — name + issue only." **That's the downloader, not the site.** The
working prototype proves rcostation.xyz exposes a full bibliographic **and relational** surface. RCO is
rich; the third-party downloaders are just lazy parsers.

(Note: RCO has **no JSON API** — it's a server-rendered ASP.NET-MVC site scraped with cheerio. "Rich"
means the *HTML pages* carry the data, and the prototype already parses it into clean objects.)

---

## 2. RCO discovery axes (each returns `[{ id, slug, title, cover }]`)

| Client fn | Endpoint | Role |
|-----------|----------|------|
| `popular` | `/ComicList/MostPopular` | **Built-in popularity row** — no LoCG/ICv2 scrape needed |
| `latest`  | `/ComicList/LatestUpdate` | **Built-in freshness row** — no 2-week lag |
| `newest`  | `/ComicList/Newest` | New series |
| `byGenre(slug)` | `/Genre/<slug>` | Genre rails (wired) |
| `search(text)`  | `/Search/Comic?keyword=` | Search |

## 3. RCO series detail (`series({slug})`)

Returns: `{ id, slug, title, cover, author(=writer), status, description, genres[], publisher,
writer, artist, publicationDate }` plus `issues({slug})` → `[{ id, number, title, date }]`
(newest-first; UI reverses to issue-1-first).

So per series we already get: **title, synopsis, writer, artist, publisher, status, genres, publication
date, full ordered issue list.** That is *more* than GCD surfaces without traversal (GCD has no synopsis
for most issues and buries creators at the story level).

## 4. The relational graph — the "more like this" engine

RCO's detail page links every series to first-class browse pages:

- `/Writer/<name>` → **all works by that writer**
- `/Artist/<name>` → **all works by that artist**
- `/Publisher/<name>` → **more from that publisher**
- `/Genre/<slug>` → **same-genre rail** (already wired as `byGenre`)

`rco.js` already *parses* the writer/artist/publisher links on the detail page (`a[href^="/Writer/"]`
etc.); it just hasn't wired client functions for them yet. Adding `byWriter` / `byArtist` /
`byPublisher` is **trivial** — identical to the working `byGenre` (same `parseList`). These four axes
ARE the "see all works by author/artist", "more from this publisher", and "more like this" rails
Hemanth described. **The site supports them today.**

This is exactly the Stremio-discovery relational richness — and it's the dimension GCD is *weakest* on.

## 5. RCO covers (low-res) + the region nuance

- Covers come on cards/detail as absolute https URLs (`/Uploads/...` absolutized, or external blogspot
  CDNs) — load directly in `<img>`, no Referer, no proxy.
- They're **low-res** → upscaled via iTunes (§6).
- Page *images* (content/download) are region-gated and harvested by driving the reader's own JS in a
  hidden BrowserWindow (descrambler rotates per load). That's the **content layer**, separate from
  metadata — relevant to the download addon, not the catalogue.

---

## 6. iTunes cover resolver contract (`itunesCovers.js`)

Keyless, proven, directly portable to C++/Qt:

- **Endpoint:** `https://itunes.apple.com/search?media=ebooks&entity=ebook&country=US&limit=15&term=<title [+ creator]>`
- **Genre filter:** keep only results whose `genreIds` ∈ {9026, 10015} (Comics & Graphic Novels) — kills
  prose-novel false positives.
- **Scoring:** exact title = 100, startsWith = 70, includes = 50, else token-overlap × 40; **+ year
  proximity** (`max(0, 10 − |Δyear|)`); **+8** if it's a Vol 1 / #1 cover. **Reject if best score < 45**
  (guards against wrong covers).
- **Hi-res rewrite:** `…/100x100bb.jpg` → `…/2000x2000bb.jpg` (clamps to source max; bigger → HTTP 400).
- **Match key:** `title + writer + year` (year parsed from `publicationDate`). ComicsSeries.jsx passes
  `{ title: meta.title, creator: meta.writer, year }`.
- **Politeness:** max 4 concurrent, ≥120 ms apart (~20 req/min soft limit); 403 → silently fall back to
  the RCO cover.
- **Cache:** hits persist to disk (`itunes-covers.json`), misses kept in-memory only (coverage improves
  on later runs); in-flight dedup by key.
- Covers are publisher-supplied marketing art → **legally cleaner** than GCD's non-CC cover scans.

---

## 7. What this changes for the catalogue decision

The v2 report's GCD-as-primary recommendation rested partly on a **false premise** ("the sites are
metadata-poor"). With RCO's true surface on the table:

- RCO supplies, live and with zero infra: **popularity, latest, genre rails, descriptions, creator/
  publisher relations, search** — i.e. most of the Stremio Home, including the relational rails GCD
  *lacks*, and with **no 2-week lag and no 800MB–1.2GB download.**
- GCD's surviving, real advantages are now narrower but still real: **reliability/permanence** (a
  Netflix-grade catalogue that doesn't go dark when a piracy domain churns) and **canonical cross-source
  identity/dedup** (one "Batman (2011)" across plural content addons, via stable GCD Series IDs).

So the honest framing going forward: this is **not** "GCD because the sites are empty." It's a genuine
trade — **RCO-class richness + freshness + zero-infra** vs. **GCD-grade reliability + canonical identity**
— and the RCO-class layer is doing far more of the discovery work than the report credited. Decision
deferred pending the real GCD dump size (the last hard number). See the catalogue-brain thread.
