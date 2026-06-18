# Western-Comics Catalogue Brain — Research Brief for Brother 1 (DeepSeek Mode)

Date: 2026-06-17
For: a DeepSeek-backed **Agent 1** session in the Claude Code harness
From: Agent 1 (Opus), after a brainstorming arc with Hemanth
Status: research task — **no product code**, read-only reconnaissance

---

## 0. Who you are / how to use this

You are **Brother 1 (Agent 1)** woken in **DeepSeek Mode**. You have the full Claude Code
toolset (WebSearch, WebFetch, Read, Grep, Glob, Bash) and the full project context. Your job
is **technical reconnaissance**, not a literature review: *actually probe the candidate sources*,
read real responses, and judge feasibility against the bar below.

Follow `agents/DEEPSEEK_MODE.md`: read-before-write, verify with real evidence, cite raw
responses/URLs (not vibes), and flag uncertainty honestly. This is read-only — see Anti-scope (§10).

---

## 1. Mission (one paragraph)

Tankoban 3's **Comics** mode is a unified mode holding **Manga** and **Comics** as subcategories,
built on a **"Stremio/Netflix, but for everything"** philosophy — a *curated, image-forward catalogue*,
**not** a Mihon-style per-source browser. The manga half's catalogue brain is solved (**AniList**:
free, no account). The Western-comics half has **no equivalent**. Your mission: find the best way to
build a **stable, source-independent, no-account "catalogue brain"** for Western comics — either by
turning an existing website/dataset into an API-like catalogue, or by hosting/bundling one — and come
back with a decision-ready recommendation.

---

## 2. Context you already own (decided — do not relitigate)

- **Unified Comics mode**: manga + comics as subcategories, mirroring how Video holds Movies + Shows.
- **Download-only, Mihon-style download mechanism**: install a source → download chapter/issue/volume
  to disk → read from your downloads. No read-online / no progressive page streaming.
- **Two separate layers, and they must stay separate:**
  - **Content/source layer = plural addons** (a Mihon/Stremio-style addon store; same model as Video's
    stream addons). *No single source is special.* RCO (ReadComicOnline) is just one content addon
    among many — content survives any one source dying.
  - **Discovery + identity layer = ONE curated metadata "brain"** (the Cinemeta/AniList role). This is
    what you are researching. It must be the *opposite* of the content layer: **stable and
    source-independent**, so it survives the whack-a-mole churn of piracy content sources.
- **The "Stremio/Netflix for everything" philosophy is locked.** The curated brain is non-negotiable;
  a pure-Mihon "no brain, just browse each source" model is explicitly rejected.

---

## 3. Hard requirements (the bar every answer must clear)

1. **No end-user account. No end-user API key. No manual configuration** before the library works.
   "It just works" is the core value.
2. **Source-independent / resilient.** Must NOT make any single fragile piracy site the identity
   authority. If a content source dies/moves domains, the catalogue must not go empty.
3. **Powers a curated Stremio-style Home**: hero carousel, "Most Popular" / "Latest" rows, fast search,
   and **canonical series identity** (one "Batman (2011)" regardless of which addon supplied the issues).
4. **Rich metadata + structure**: title, synopsis, creators (writer/artist), publisher, dates, and
   **especially issue + collected-edition (TPB/volume) structure** — Western comics' hardest part
   (runs, reboots, legacy numbering, annuals, variants, omnibuses).
5. **Covers are a solved side-problem.** Hi-res covers are backfilled via the iTunes/Apple Books
   search API (already proven in the Electron build). So cover *quality* is not the blocker — a usable
   base cover + good metadata/structure is what matters.
6. **Legally workable for a freely distributed desktop app** (caching/ingest/redistribution posture).

---

## 4. Ruled out — do NOT re-propose without genuinely new evidence

- **RCO (or any single piracy site) as the metadata spine** — anchoring identity to one whack-a-mole
  site = recurring empty-library events. (RCO stays welcome as *one content addon*, not the brain.)
- **ComicVine as the default brain** — per-user API key, restrictive ToS on caching/redistribution,
  weak search, awkward "volume" terminology.
- **Shared app-level Metron/ComicVine account/key** — doesn't scale, ban risk, quota collapse.
- **Marvel API as a general backbone** — Marvel-only; can't cover DC/Image/Dark Horse/Boom/IDW/indie.
- **Pure Mihon (no brain, per-source catalogues only)** — rejected by the locked philosophy.

---

## 5. Research questions (the deliverable answers these)

**Q1 — HEADLINE. GCD's real update mechanism.**
GCD (Grand Comics Database, comics.org) is the current leading candidate for the brain. Hemanth's
proposal: bundle a local GCD-derived SQLite DB that **self-updates** to track the live online GCD,
turning a "frozen snapshot" into a "living catalogue."
- Does GCD support **incremental sync** (deltas / "recent changes" feed / API), or **only full
  database dumps** on a schedule? *This decides whether the self-updating idea is golden or just moves
  the bandwidth problem.*
- Dump format, cadence, full size, and realistic **pruned + compressed** size (English + major
  publishers only).
- Quality of **collected-edition / reprint / variant** relationships in the schema.
- Cover image **licensing** (metadata is CC-ish; covers reportedly are not freely reusable — confirm).
- Feasibility of the self-update as **C++/Qt or a server-side delta-publisher** (NOT a bundled Python
  runtime — that conflicts with the native stack).

**Q2 — What other sites/datasets could become an "API-like catalogue" we build ourselves?**
For each candidate, report: data richness (issue + TPB structure, creators, publisher, dates, synopsis),
auth model, base cover availability, ToS for ingest/caching/redistribution, API-vs-scrape feasibility,
stability, and a stable-ID system if any. Candidates to probe (not exhaustive — hunt for more):
GCD, ComicVine (as an *ingest/mirror* source, not per-user), Metron, League of Comic Geeks, Comick
(comick.io), MangaUpdates, Wikidata/Wikipedia (SPARQL/REST), Inducks, OpenLibrary/Internet Archive,
Shortboxed, Comichron, any open comic ontology / academic / GitHub dataset.

**Q3 — Popularity / freshness signals.** A Stremio Home needs "popular / trending / latest." GCD is
bibliographic and likely has weak popularity signals. Where can we get no-account
popular/trending/latest-release signals for Western comics (Comichron sales, Shortboxed new releases,
LoCG community popularity, etc.)?

**Q4 — Canonical identity.** Is there a stable, open ID authority for Western comics (GCD series ID,
ComicVine volume ID, Metron ID, Wikidata QID) we can anchor cross-source identity on — the way AniList
IDs anchor manga?

**Q5 — Architecture of the brain.** Compare: (a) **bundled local SQLite + self-update**, (b) a
**server-side catalogue API we host** (ingesting from these sources; the app queries it), (c) **hybrid**.
Trade-offs for a no-account "just works" desktop app: app size, offline behavior, infra we'd have to
babysit, update latency, legal exposure, failure modes.

---

## 6. Method

- **Probe, don't summarize.** Use WebFetch to hit real endpoints / dump-index pages / API docs and read
  the actual responses, schemas, sizes, rate-limit headers. Quote raw evidence.
- **Read our own code** for what we already do: the Electron prototype at
  `C:\Users\Suprabha\Desktop\Tankoban Electron` — especially `src/main/anilist.js`,
  `src/main/itunesCovers.js`, and the comics source/series code.
- **Read-only, metadata-only.** Do not fetch comic page scans; do not bypass Cloudflare/CAPTCHA/paywalls.
- Verify dump sizes / cadences against the actual GCD downloads page, not blog hearsay.

---

## 7. Reference material

| What | Where |
|---|---|
| Unified Comics mode vision | `Tankoban 3/docs/superpowers/specs/2026-06-17-comics-unified-mode-vision.md` |
| Western-comics metadata strategy (GCD-first doc) | the `tankoban-western-comics-metadata-strategy.md` memo |
| Electron prototype (AniList, iTunes covers, comics source) | `C:\Users\Suprabha\Desktop\Tankoban Electron` |
| AniList GraphQL (the proven manga brain) | `https://graphql.anilist.co` |
| GCD | `https://www.comics.org` (+ its downloads/dumps page) |

---

## 8. Deliverable

A single decision-ready report containing:

1. **A definitive answer to Q1** — GCD's update mechanism — with raw evidence (dump page, sizes,
   cadence, any delta/API feed). A clear **go / adjust / stop** on the self-updating GCD mirror.
2. **A comparison table** across all Q2 candidates on the §3 dimensions.
3. **A ranked recommendation** for the Western-comics brain: the single best path + a fallback, with the
   §5(Q5) architecture choice (bundled-self-update vs hosted-API vs hybrid) argued from this app's real
   constraints — not generic best practice.
4. **Popularity/identity answers** (Q3, Q4) folded in.
5. **Honest trade-offs and anything legally gray**, flagged plainly.

Keep it concrete and current (2026). Prefer "I hit this endpoint and got X" over "sources suggest."

---

## 9. The decision this feeds

Whatever you recommend becomes the Western-comics half of the unified Comics-mode catalogue brain — the
Cinemeta/AniList equivalent that powers the curated Home, search, and canonical identity, while plural
content addons (RCO and others) supply the downloadable pages. Build nothing yet; this is groundwork
toward a locked decision.

---

## 10. Anti-scope

- No product code. No UI. No schema implementation.
- Do not bypass Cloudflare / CAPTCHA / paywalls / login walls / anti-bot controls.
- Do not fetch comic page images or issue scans.
- Do not re-propose §4 ruled-out options without genuinely new evidence.
- Do not bundle covers from any source whose license you haven't verified.
