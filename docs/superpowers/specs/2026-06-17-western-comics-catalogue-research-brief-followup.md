# Western-Comics Catalogue Brain — Follow-up Research Brief (Brother 1, DeepSeek Mode)

Date: 2026-06-17
For: a DeepSeek-backed **Agent 1** session in the Claude Code harness
From: Agent 1 (Opus), after reviewing your first report with Hemanth
Status: **corrective follow-up** — builds on report v1, read-only recon, no product code

---

## 0. Read these first

- Your first report: `2026-06-17-western-comics-catalogue-research-report.md`
- The original brief: `2026-06-17-western-comics-catalogue-research-brief.md`
- Vision: `2026-06-17-comics-unified-mode-vision.md`

This is **not a redo.** Your v1 research was good — keep it. This brief tells you what to *keep*,
what to *drop*, and the **two vectors v1 skipped** that you now go deep on.

---

## 1. What v1 got right — KEEP, do not re-research

- **Q1 (GCD update mechanism)** — bi-weekly full dumps, no delta feed, ~2.4GB→4.1GB, CC BY-SA data /
  non-CC covers, schema specifics. Solid and cited. Banked.
- **Source probes** — Metron endpoints + `modified_gt`, GCD schema, ComicVine ToS bar, Wikidata/OpenLibrary
  roles, the iTunes cover solution, and the Electron-code reads (anilist.js / itunesCovers.js / rco.js).
  All keepers. Don't re-derive them.

## 2. What v1 got wrong — DROP

**The Tier-1 recommendation (Metron via a shared-account caching proxy) is rejected.** It violates the
hard constraints and fails on its own evidence:

- A **shared app-level account** is on the brief's ruled-out list. The user-invisible proxy doesn't change that.
- **The math is fatal:** 5,000 requests/**day** shared across the *entire* userbase through one account —
  and Metron *cut* its limits in March 2026 due to server load. Dead on arrival as a primary at any real scale.
- It reintroduces **per-user infra we host + babysit + pay for** and **concentrates fragility** in one
  donation-run project — the exact opposite of "source-independent, no infra, just works."

**Corrected ranking premise:** Metron is **optional BYOK only** (a power user connects *their own* Metron
account for enrichment) — **never** a shared proxy, **never** the primary. Re-rank everything on our actual
values: *no account, no per-user infra, source-independent, scales to many users, offline-capable.*

---

## 3. Vector A (PRIMARY FOCUS) — Mihon comic extensions as ingest sources

This is the angle v1 missed and it may be the strongest path. The thesis:

> The Mihon / Keiyoushi (Tachiyomi-lineage) extension ecosystem already contains **working, maintained,
> community-vetted parsers** for real comic sources. Each extension *is* a de-facto API spec for a site
> that may have no formal API. Instead of hunting for a clean western-comics metadata API (which doesn't
> exist), enumerate the **comic** sources these extensions already solve, and evaluate which we could
> **ingest into our own served catalogue API.**

Research tasks:

1. **Enumerate the extension ecosystem.** Keiyoushi (the maintained post-Tachiyomi repo), any
   comic-specific extension repos/forks, and multi-medium sources (e.g. Comick). For each repo, how many
   **western-comic** (not manga) sources does it carry? **Be honest and quantify** — the ecosystem skews
   heavily manga, so the western-comic list is expected to be thin. "How thin, and exactly which ones" is
   a core deliverable.
2. **Map each comic source.** For every western-comic source an extension targets, report:
   - The site it hits, and the endpoints the extension uses (catalogue / popular / latest / search /
     series-detail / issue-list).
   - **Data shape & bibliographic quality** — does it cleanly separate series/runs, issues, and collected
     editions, or is it a flat reading-site list? (Same structure bar as GCD: test against Batman 2011,
     Amazing Spider-Man legacy numbering, a title with annuals.)
   - Metadata richness: title, synopsis, creators, publisher, dates, cover (and resolution).
   - Stable IDs vs. fragile slugs/URLs; markup stability; anti-bot posture.
   - **Could this site be ingested into our own catalogue API** (metadata only), and could the same
     extension logic feed a **content addon** for downloads?
3. **Role classification.** For each source: is it viable as a **metadata** contributor, a **content**
   addon, or **both**? (In Mihon, a source provides both — assess whether its *metadata* is good enough to
   feed identity, or only good enough for content.)
4. **Legality posture.** These are largely aggregator/piracy sites. We ingest **metadata** into our own
   API, not scans. Note the gray areas; do **not** fetch page images or bypass access controls.

The point: confirm or kill the idea that a small set of proven extension-backed sources, normalized into
**our own catalogue API**, can serve western-comics identity better/cheaper than GCD — or, more likely,
**complement** a GCD spine.

---

## 4. Vector B (PRIMARY FOCUS) — Server-side GCD delta-publisher architecture

v1 dismissed GCD as "heavy: full dumps, no deltas." But we never wanted the self-update in the *app*. The
architecture v1 never costed:

> **We** run the GCD ingest **server-side**. Each bi-weekly dump is ingested, pruned, and **diffed against
> the prior** to produce a small **delta** + manifest. The app pulls deltas on launch and applies them to
> its local SQLite. Initial seed = a pruned baseline (shipped, or first-run download).

This gives GCD the *delta-freshness* Metron was praised for — **with no account, no per-user infra, no
5,000/day ceiling, and full offline capability.** It's a static-file host + a periodic ingest job, far
lighter than running a live API.

Research tasks:

1. **Realistic sizes** (verify, don't reuse the stale 2023 figures): current full GCD dump size; a
   **pruned baseline** (English + major publishers) compressed; and a **typical bi-weekly delta** size.
2. **Pipeline feasibility:** can a reliable diff be computed from successive full dumps (no built-in change
   tracking)? What does the ingest/prune/diff job look like (server-side; language-agnostic, NOT bundled
   in the app)?
3. **Hosting cost/complexity** vs. the rejected Metron proxy — and vs. the simpler "just ship bi-weekly
   full snapshots" alternative. Honest trade-off.
4. **Does it fully satisfy** no-account + offline + scale-to-many-users? Where does it strain?

---

## 5. The architecture to validate (the likely answer)

Pressure-test this combined shape and tell me if it holds or breaks:

```
Identity / catalogue brain  = GCD spine, kept fresh by a server-side delta-publisher (Vector B)
Content (downloadable pages) = plural addons, sourced from the proven Mihon-extension sites (Vector A)
Supplementary metadata/covers = Mihon-source fields + iTunes hi-res covers
Popularity / trending rows   = LoCG "Most Pulled" + ICv2 Top 50 (from v1)
Canonical ID                 = GCD Series ID (from v1)
Optional power-user enrich   = Metron BYOK (user's own account)
```

Mirror of the manga half: AniList = brain, WeebCentral = content addon. Here: GCD-delta-spine = brain,
Mihon-extension sites = content addons.

---

## 6. Deliverable

1. **Corrected ranked recommendation** on our actual values (shared-proxy removed; Metron BYOK-only).
2. **Mihon comic-extension source map** — the table from §3, with the honest western-comic count.
3. **Server-side GCD-delta-publisher feasibility** — §4, with real sizes, a pipeline sketch, and the
   honest trade-off vs. full-snapshot shipping.
4. **A verdict on the §5 combined architecture** — does it hold? Where does it break?

Concrete, current (2026), evidence-cited. "I hit this endpoint / read this extension and saw X."

---

## 7. Anti-scope

- No product code, no UI, no schema implementation.
- No fetching comic page images / scans. No bypassing Cloudflare / CAPTCHA / paywalls / anti-bot.
- Do not re-propose the shared-account Metron proxy. Do not bundle covers of unverified license.
- Metadata-only reconnaissance.
