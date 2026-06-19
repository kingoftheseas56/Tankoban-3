# Sources List — Native Qt model/view rebuild (design)

**Date:** 2026-06-19
**Author:** Agent 4 (TB3 player/Theatre track)
**Status:** Design — awaiting Hemanth review → implementation plan
**Governs under:** `feedback_tb3_qt_native_surface_per_job_motto` (the TB3 UI motto)

---

## 0. Motto & the hard gate (the reason this work exists)

> **Right Qt surface per job; Harbor's experience, not its DOM; native-desktop performance is the acceptance bar — and reach for the fewest surfaces that clear it.**

The sources page builds one live `QWidget` (`StremioRow`) per source. On a 500-source title (e.g. Spider-Noir) this means hundreds of stylesheet-styled widgets in a `QVBoxLayout` inside a `QScrollArea` — the classic "web DOM traced into Qt" anti-pattern. Result: rough scroll, multi-second freeze on filter switch, and brief not-responding. The fix is to use Qt's native idiom for long lists: **model/view virtualization** (only ~visible rows ever render).

**Definition of Done (hard gate — fail ANY item → not done, no matter how pixel-close to Harbor):**

1. Clicking never freezes the window.
2. Scrolling stays stable (smooth, no hitching).
3. Rows don't jitter as results stream in.
4. No live widgets for off-screen rows.
5. No hidden panels secretly alive / eating memory.
6. Images/icons cached + resized sensibly (`QPixmapCache`, not per-paint rasterization).
7. Long torrent names don't seize layout.
8. Results arriving one-by-one don't thrash the UI.

---

## 1. Goal & scope

**Goal:** Replace the source-row rendering engine of the sources page with a native Qt model/view stack, so the page meets the Definition of Done — while looking and behaving **pixel-for-pixel identical** to today (Guardrail 2: flex the plumbing, never the look).

**In scope (the "list engine" only):**
- The source-row rendering inside `StreamList` (today: `QVBoxLayout` of `StremioRow` widgets → tomorrow: `QListView` + model + delegate).
- Filtering (addon + quality) routed through a proxy model.
- Async/incremental result handling at the data layer.
- Reverting the opaque-viewport diagnostic and restoring the see-through backdrop.

**Out of scope (do NOT touch):**
- `PlayPickerPage` shell (header, filter bar widgets, layout) — unchanged except the backdrop revert.
- `StreamService` and `MainWindow::buildPicker` scoring/ranking — unchanged; they keep producing the ranked `ScoredStream` list.
- The player track (MG1–4, mpv, PlaybackClock) — untouched.
- **Badge visuals stay as today's text-chips** (the Harbor image-badge-pack parity port is a separate deferred slice; the delegate paints the *current* text-chip badges 1:1).
- Catalog/grid/home surfaces — this is the sources list only.

---

## 2. Architecture

Four units, each with one job and a clean interface.

```
PlayPickerPage (unchanged shell)
  └── StreamList (public API unchanged: setStreams / clearStreams / streamActivated)
        ├── filter bar widgets (addon button, quality pills, pending pill)  [stay widgets]
        └── QListView
              ├── SourceFilterProxy (QSortFilterProxyModel)  — sort by score, filter by addon+quality
              │     └── SourceListModel (QAbstractListModel) — the ScoredStream data, no widgets
              └── SourceRowDelegate (QStyledItemDelegate)    — paints one row pixel-1:1
```

### 2.1 `SourceListModel : QAbstractListModel`
- **Owns** the accumulated `ScoredStream` records (insertion order; ranking is the proxy's job).
- **Roles:** a single `Qt::UserRole` returns the `ScoredStream` (registered via `Q_DECLARE_METATYPE`) — the delegate reads it directly. (No per-field roles; the delegate paints everything.)
- **Incremental update API** (`setStreams(const QVector<ScoredStream>&)`):
  - New stream-key → `beginInsertRows` append.
  - Existing key with changed score/fields → update in place + `dataChanged(index)`.
  - Key absent from the incoming set (e.g. after dedup at `streamsReady`) → `beginRemoveRows`.
  - Never a blanket reset during streaming (a reset is permitted only on a fresh `open()` / `clearStreams`). This satisfies DoD #3 and #8.
- **Stream key:** `addonId | (infoHash:fileIdx | url | title)` — the same identity used in the current reconciliation, hoisted to the data layer.

### 2.2 `SourceFilterProxy : QSortFilterProxyModel`
- **`lessThan`:** sort descending by the stream's score (reproduces `rankAndPick` order on screen without reordering the source model).
- **`filterAcceptsRow`:** apply the addon filter (`addonInstanceKey`) and quality filter (`4K/1080p/720p/SD`).
- **Filter change** = `setAddonFilter()/setQualityFilter()` → `invalidate()` → the view repaints only visible rows. This is what turns the multi-second filter freeze into an instant switch (DoD #1).

### 2.3 `SourceRowDelegate : QStyledItemDelegate`
- **`paint`** reproduces today's `StremioRow` pixel-for-pixel with `QPainter` (see §4 for the geometry contract).
- **`sizeHint`** returns a **fixed uniform height** (§4.3) — required for fast, stable virtualization (DoD #2, #7).
- **Icon caching:** the play / copy SVG glyphs are rasterized **once** into `QPixmap`s held in `QPixmapCache` (keyed by glyph+color+dpr), never re-rendered per paint (DoD #6).
- **Hover + hit-testing:** see §5.

### 2.4 `StreamList` (thin wrapper — public face unchanged)
- Keeps its current public surface so `PlayPickerPage` is untouched:
  - `void setStreams(const QVector<ScoredStream>&, const Options&)` → forwards to the model + sets pending state.
  - `void clearStreams()` → model reset.
  - `signal streamActivated(const ScoredStream&)`.
- Internally: holds the `QListView` + model + proxy + delegate, plus the existing filter-bar widgets (addon button, quality pills, pending pill) which now drive the proxy instead of rebuilding rows.

---

## 3. Data flow

```
StreamService partial/ready
  → MainWindow::buildPicker()            (parse + score + rank — UNCHANGED)
  → StreamList::setStreams(ranked)
  → SourceListModel incremental merge    (insert new / update changed by key)
  → SourceFilterProxy re-sort + re-filter (data-only, microseconds)
  → QListView repaints ONLY visible rows
```

No widget is created or destroyed on a partial; streaming results move data, not widgets, so the row jitter (which was widget teardown) cannot occur (DoD #3, #8). Filtering and quality switching run entirely through the proxy (DoD #1).

---

## 4. Pixel-1:1 row paint contract

The delegate reproduces `StremioRow`'s current layout exactly. Reference geometry (from `src/ui/StremioRow.cpp`):

### 4.1 Horizontal structure (left → right)
- **Logo column** — 68px wide; a 56×56 rounded tile (`#1a1d24`, 1px border, 12px radius) with the addon's initial letter centered (22px, weight 700, `#aab1bd`).
- **Content column** (stretch) — vertically centered stack:
  - **Headline** (`stream.name`, fallback addon name) — 16px, weight 600, `#f3f1ea`.
  - **Description** (`stream.title` else `stream.description`) — 14px, `#aab1bd`, **elided** to fit (§4.3).
  - **Badge row** — text chips (label, `#aab1bd` on `rgba(18,19,23,0.85)`, 1px border, 7px radius, 11px/weight 600, 2×7 padding); danger labels (`CAM/TS/TC/SCR/NO LABEL/UNKNOWN`) use `#e58a8d` text + red border. Chips come from the existing `streamBadges()` logic (ported into a pure helper).
  - **Status line** — 13px; `#e5484d` for failed, `#8b909b` muted for "Torrent · P2P stream" / source notes.
- **Actions column** — copy button (36×36, transparent, copy glyph) + play button (64×64, gold `#e8b923` circle with dark play glyph when playable; muted `rgba(26,29,36,0.70)` when not).

Row background: `rgba(26,29,36,0.40)`, 1px border `rgba(255,255,255,0.05)`, 16px radius; failed rows tint red. Outer row margins 20px, inner spacing 20px.

### 4.2 Painting source of truth
The chip/badge selection, confidence, and playability logic currently in `StremioRow::setStream`/`streamBadges`/`qualityConfidence` are extracted into **pure, widget-free helpers** (input: `ScoredStream` → output: strings/enums) that the delegate calls and that are unit-testable in isolation (no widget needed).

### 4.3 Fixed height + elision (the one approved parity nuance)
- Rows are a **fixed uniform height** (finalized in impl to match the current common-case visual — headline + up to 2 description lines + badge row + status). 
- Long descriptions/filenames are **elided with "…"** rather than wrapping to a third line. Visually identical for the vast majority of rows; only monster-long names truncate instead of seizing layout — which is both more Harbor-faithful (Harbor truncates) and required by DoD #7 + "stable row heights."

---

## 5. Row interaction (hover + click — "flex the plumbing, not the look")

Because rows are painted, not widgets, interaction is delegate/view-driven but **visually and behaviorally identical**:

- **Hover:** the `QListView` enables mouse tracking; the delegate paints the hovered row's hover state (play-button brighten, copy-button reveal) using the view's current hovered index. Same feedback as today's `:hover` QSS.
- **Click hit-testing:** on click, map the cursor to the row's sub-rects (play rect / copy rect / row body). Play rect (and a playable stream) → `emit streamActivated(stream)`; copy rect → copy `stream.url`/`externalUrl` to clipboard + show the transient check-glyph (a per-index "copied at" timestamp drives the glyph for ~1.2s). Implemented via the view's `mousePressEvent`/`clicked` + the delegate's rect math (standard Qt delegate-button pattern).
- **Cursor:** pointing-hand over actionable rects.

No `QPushButton` lives per row; the gold play button and copy button are painted. This is the only place "native plumbing" differs from the widget version, and it is invisible to the user.

---

## 6. Backdrop

- The opaque-viewport change in `PlayPickerPage` was a **diagnostic** — it is **reverted**: the see-through translucent backdrop is **restored** to the original M5/M6 design.
- With model/view, each scroll frame paints ~8 delegate rows over the (already-cached) backdrop, so the translucent viewport should be affordable. **Verify against DoD #2** (stable scroll) on Spider-Noir.
- **Fallback (only if the restored backdrop still fails DoD #2):** adopt Harbor's own layout — backdrop fixed behind the header, the list scrolling on opaque dark (which lets the viewport blit-scroll). Try the restore first; escalate to the fallback only on measured failure.

---

## 7. Files

**New:**
- `src/ui/SourceListModel.{h,cpp}` — `QAbstractListModel`.
- `src/ui/SourceFilterProxy.{h,cpp}` — `QSortFilterProxyModel`.
- `src/ui/SourceRowDelegate.{h,cpp}` — `QStyledItemDelegate` (paints the row; owns the pure badge/paint helpers or includes them from a shared header).
- (Possibly) `src/ui/StreamRowPaint.{h,cpp}` — extracted pure helpers (`streamBadges`, `qualityConfidence`, playability) shared/unit-tested.

**Changed:**
- `src/ui/StreamList.{h,cpp}` — internals swapped to `QListView` + model/proxy/delegate; public API + filter-bar widgets retained.
- `src/ui/PlayPickerPage.cpp` — revert the opaque-viewport diagnostic (restore translucent backdrop).
- `CMakeLists.txt` — register new sources.

**Retired:**
- `src/ui/StremioRow.{h,cpp}` — removed once the delegate reproduces it (its paint/badge logic is preserved in the extracted helpers + delegate). Keep until the delegate is verified pixel-1:1, then delete.

---

## 8. Acceptance criteria

The 8 DoD items in §0 are the pass/fail gate. Concretely, on a 500-source title (Spider-Noir):
- Open sources → no window freeze while results stream in (DoD #1, #8).
- Scroll top-to-bottom → smooth, no hitching (DoD #2).
- Watch rows as partials arrive → no jitter (DoD #3).
- Switch All → 4K → 1080p → SD repeatedly → instant, no freeze (DoD #1).
- Inspect memory/handles → no hundreds of live row widgets; off-screen rows are data only (DoD #4, #5).
- A monster-long torrent name → elides, row height unchanged (DoD #7).
- Visual diff vs. current build → indistinguishable rows (Guardrail 2).

---

## 9. Risks & open questions

- **Pixel-1:1 fidelity of the painted delegate** — the highest-effort part; mitigated by extracting the existing paint/badge logic into shared helpers and diffing against the current build visually before deleting `StremioRow`.
- **Re-rank churn under the proxy** — sorting is data-only and cheap; the view repaints visible rows. Expected smooth, but verified against DoD #3 on the heaviest title.
- **Hover/click parity** — the delegate hit-testing must match the widget buttons' feel (hover brighten, copied-glyph timing). Verified by eye-smoke.
- **Backdrop** — restore-first; fixed-header fallback documented (§6).
- **Addon logos** — out of scope here (initial-letter tile retained, matching current); image-badge-pack + addon-logo parity is a later slice.
