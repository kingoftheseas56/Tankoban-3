# Sources List — Native Qt model/view rebuild — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the source-row rendering of the sources page with a Qt-native model/view stack (`QAbstractListModel` + `QSortFilterProxyModel` + `QListView` + a painting delegate), so it clears the native-performance Definition of Done while staying pixel-for-pixel identical to today.

**Architecture:** Source `ScoredStream` data lives in `SourceListModel` (no widgets). `SourceFilterProxy` preserves the already-ranked `MainWindow::buildPicker()` order while filtering by addon/quality. `SourceListView` (a thin `QListView`) shows them, and `SourceRowDelegate` paints each visible row 1:1 with `StremioRow`. `StreamList` keeps its exact public API and filter-bar widgets, swapping only its internals. Pure paint/badge logic is extracted to `StreamRowPaint` and unit-tested.

**Tech Stack:** C++20 (repo CMake standard), Qt 6 Widgets (model/view), CTest self-test (`Tankoban3SelfTest`), Ninja/MSVC via `build.bat`.

**Spec:** `docs/superpowers/specs/2026-06-19-sources-list-native-model-view-design.md`

**Governs under:** `feedback_tb3_qt_native_surface_per_job_motto` — the 8-item Definition of Done in the spec §0 is a hard gate.

---

## Conventions for every task

- Build the app: `cmd /c "C:\Users\Suprabha\Desktop\Tankoban 3\build.bat"` → expect `BUILD OK`.
- Run the self-test: `cd "C:/Users/Suprabha/Desktop/Tankoban 3" && ctest --test-dir out -R selftest --output-on-failure` → expect `SELFTEST OK` / `Passed`.
- After adding a NEW source file to `CMakeLists.txt`, the Ninja reconfigure is triggered by the `CMakeLists.txt` mtime change, so a plain `build.bat` picks it up. If a build ever fails to see a new file, delete `out/build.ninja` and rebuild.
- TB3 commits push to origin (Rule 23): each commit step is `git add <files> && git commit -m "..." && git push origin HEAD`.
- Work on the current branch (`scratch/stream-engine`) unless told otherwise.

---

## Codex review amendments (Agent 4/Codex, 2026-06-19)

These are execution-hardening changes found during pre-execution review. Treat them as part of the plan, not optional polish.

- **Preserve `MainWindow::buildPicker()` ranking.** The incoming `picker.all` vector is already ranked by `StreamScorer::rankAndPick()` and `PlayPickerPage` sets `Options::preserveOrder = true`. Do not silently re-rank by score-only in the proxy. The model stores the incoming ranked ordinal per key and exposes it as `RankRole`; the proxy sorts by `RankRole` ascending. Score/native-index are fallback tiebreaks only. This avoids breaking `respectAddonOrder` and any future scorer ranking nuance.
- **Make proxy sorting react to emitted roles.** Set the proxy sort role to `SourceListModel::RankRole`, and emit `dataChanged(..., {StreamRole, RankRole})` when fields or rank change. A `dataChanged` vector containing only `StreamRole` is not enough if the proxy's sort role is still the default `DisplayRole`.
- **Deduplicate incoming streams by `streamRowKey` before merging.** Keep the first ranked occurrence for a key unless the scorer contract explicitly changes. Duplicate keys in one partial must not create duplicate model rows.
- **Hit-testing must honor action availability.** Clicking the play rect no-ops for unplayable streams; clicking/cursor over copy no-ops unless a direct URL or external URL exists. The old widget had disabled/hidden buttons; the painted view keeps that behavior.
- **Store hover/copied by stable key, not proxy row number.** Proxy row numbers change under filter/sort/partial updates. Delegate state uses `streamRowKey(s)` (or an empty key) so hover/copy feedback cannot jump to a different row during live updates.
- **Self-test fixtures must use the repo's sentinel values.** A default `ScoredStream` has `Source::Unknown` / `Codec::Unknown` / `AudioCodec::Unknown`, while the moved `qualityConfidence` helper checks `Other`; tests expecting `NO LABEL` must set those fields to `Other`.
- **CMake wiring remains both-target for pure/model/proxy files.** `StreamRowPaint.cpp`, `SourceListModel.cpp`, and `SourceFilterProxy.cpp` go into both `Tankoban3` and `Tankoban3SelfTest`. Widget/Svg files (`SourceRowDelegate.cpp`, `SourceListView.cpp`) stay app-only.

---

## As-built note (2026-06-20, Tasks 4-6 shipped — commit c0a382b)

The implementation **diverged** from the Task 4/5/6 code below; the divergences were found during Hemanth's eye-smoke and are the correct as-built:
- `SourceListModel::KeyRole` **never existed** — the model exposes only `StreamRole` + `RankRole`. Wherever the code below reads `KeyRole`, the as-built keys off **`streamRowKey(s)`** (the Task 1 helper) instead.
- Real Stremio source labels are **multi-line**, so `SourceRowDelegate` is **variable-height + word-wrapped** (NOT the fixed `kRowHeight` / single-line `drawText` of Task 4). `sizeHint` measures from `opt.widget`'s viewport width; `setUniformItemSizes` is **removed**.
- `SourceListView`'s viewport is **transparent** (`WA_TranslucentBackground`) so the page shows through instead of an opaque dark block.

Full root-cause writeup: memory `project_tankoban3_sources_page_native_modelview_fix`.

---

## File Structure

**New files:**
- `src/ui/StreamRowPaint.h` / `.cpp` — pure, widget-free helpers: `streamRowKey`, `streamBadges`, `qualityConfidence`, `qualityGroupOf`, `isPlayable`, `addonInstanceKey`. (Moved out of `StremioRow.cpp`'s anonymous namespace so the delegate and tests can call them.)
- `src/ui/SourceListModel.h` / `.cpp` — `QAbstractListModel` holding `ScoredStream` rows; incremental merge by key.
- `src/ui/SourceFilterProxy.h` / `.cpp` — `QSortFilterProxyModel`; ranked-order sort + addon/quality filter.
- `src/ui/SourceRowDelegate.h` / `.cpp` — `QStyledItemDelegate`; paints a row 1:1; cached icons; exposes sub-rect geometry.
- `src/ui/SourceListView.h` / `.cpp` — `QListView` subclass; hover tracking + play/copy hit-testing.

**Modified:**
- `src/ui/StreamList.h` / `.cpp` — internals swapped to the view stack; public API + filter bar retained.
- `src/ui/PlayPickerPage.cpp` — revert the opaque-viewport diagnostic (restore translucent backdrop).
- `src/selftest_main.cpp` — add tests for the pure helpers + model merge.
- `CMakeLists.txt` — register new sources in both `Tankoban3` and `Tankoban3SelfTest`.

**Retired (last task):**
- `src/ui/StremioRow.h` / `.cpp` — deleted once the delegate is verified pixel-1:1.

---

### Task 1: Extract pure paint/badge helpers (`StreamRowPaint`) + unit-test them

**Files:**
- Create: `src/ui/StreamRowPaint.h`, `src/ui/StreamRowPaint.cpp`
- Modify: `CMakeLists.txt` (add to both targets), `src/selftest_main.cpp`

- [ ] **Step 1: Create the header**

`src/ui/StreamRowPaint.h`:

```cpp
// Tankoban 3 - pure, widget-free source-row logic (badges, confidence, identity,
// quality group, playability). Extracted from StremioRow so SourceRowDelegate and
// the self-test can share it. No QWidget dependency.
#pragma once

#include <QString>
#include <QStringList>

#include "core/StreamModels.h"

namespace tankoban {

enum class RowConfidence { Labeled, Unverified, Unlabeled };

// Stable per-stream identity used for model-row reuse (addon + hash/url/title).
QString streamRowKey(const ScoredStream& s);

// Addon-instance identity for the addon filter (addonUrl else addonId).
QString addonInstanceKey(const ScoredStream& s);

// "4K" / "1080p" / "720p" / "SD" bucket for the quality filter + pills.
QString qualityGroupOf(const ScoredStream& s);

// Harbor format-badge.tsx parity: confidence + the ordered chip labels.
RowConfidence qualityConfidence(const ScoredStream& s);
QStringList streamBadges(const ScoredStream& s);

// Direct URL OR torrent infoHash (torrents stream via the engine) => playable.
bool isPlayable(const ScoredStream& s);

} // namespace tankoban
```

- [ ] **Step 2: Create the implementation (moved verbatim from `StremioRow.cpp`)**

`src/ui/StreamRowPaint.cpp`:

```cpp
// Tankoban 3 - see StreamRowPaint.h. Logic moved verbatim from StremioRow.cpp.
#include "ui/StreamRowPaint.h"

#include <QSet>

namespace tankoban {

QString streamRowKey(const ScoredStream& s)
{
    const QString id = !s.infoHash.isEmpty()
        ? QStringLiteral("h:%1:%2").arg(s.infoHash).arg(s.fileIdx)
        : (!s.url.isEmpty() ? QStringLiteral("u:%1").arg(s.url)
                            : QStringLiteral("t:%1").arg(s.title.isEmpty() ? s.name : s.title));
    return s.addonId + QLatin1Char('|') + id;
}

QString addonInstanceKey(const ScoredStream& s)
{
    return s.addonUrl.isEmpty() ? s.addonId : s.addonUrl.toString();
}

QString qualityGroupOf(const ScoredStream& s)
{
    switch (s.resolution) {
    case Resolution::FourK:  return QStringLiteral("4K");
    case Resolution::FullHD: return QStringLiteral("1080p");
    case Resolution::HD:     return QStringLiteral("720p");
    default:                 return QStringLiteral("SD");
    }
}

RowConfidence qualityConfidence(const ScoredStream& s)
{
    const bool nothingDetected =
        (s.resolution == Resolution::SD || s.resolution == Resolution::None)
        && s.source == Source::Other && s.codec == Codec::Other
        && s.hdrFormat == HdrFormat::None && s.audio.codec == AudioCodec::Other;
    if (nothingDetected)
        return RowConfidence::Unlabeled;

    for (const ScoreReason& r : s.reasons) {
        const QString& sig = r.signal;
        if (sig.startsWith(QStringLiteral("fresh-fake-"))
            || sig == QStringLiteral("fresh-soft-flag")
            || sig == QStringLiteral("fresh-prerelease-soft")
            || sig == QStringLiteral("fresh-prebluray-suspect")
            || sig == QStringLiteral("size-mismatch")
            || sig == QStringLiteral("title-says-hires-filename-says-cam")) {
            return RowConfidence::Unverified;
        }
    }

    const bool claimsHighRes =
        s.resolution == Resolution::FourK || s.resolution == Resolution::FullHD;
    const bool directStream = !s.url.isEmpty() && s.infoHash.isEmpty();
    if (claimsHighRes && s.size <= 0 && s.source == Source::Other && !directStream)
        return RowConfidence::Unverified;
    return RowConfidence::Labeled;
}

QStringList streamBadges(const ScoredStream& s)
{
    QStringList out;

    QString capture;
    if (s.source == Source::CAM)                              capture = QStringLiteral("CAM");
    else if (s.source == Source::TS || s.source == Source::HDTS) capture = QStringLiteral("TS");
    else if (s.source == Source::TC)                          capture = QStringLiteral("TC");
    else if (s.source == Source::SCR)                         capture = QStringLiteral("SCR");

    const RowConfidence conf = qualityConfidence(s);
    if (!capture.isEmpty()) {
        out << capture;
    } else if (conf == RowConfidence::Unlabeled) {
        out << QStringLiteral("NO LABEL");
    } else if (conf == RowConfidence::Unverified) {
        out << QStringLiteral("UNKNOWN");
    } else {
        switch (s.resolution) {
        case Resolution::FourK:  out << QStringLiteral("4K"); break;
        case Resolution::FullHD: out << QStringLiteral("1080p"); break;
        case Resolution::HD:     out << QStringLiteral("720p"); break;
        case Resolution::SD_480:
            out << (s.source == Source::DVDRip ? QStringLiteral("DVD") : QStringLiteral("480p"));
            break;
        case Resolution::SD:
            out << (s.source == Source::DVDRip ? QStringLiteral("DVD") : QStringLiteral("SD"));
            break;
        case Resolution::None: break;
        }
    }

    if (capture.isEmpty()) {
        if (s.remux)
            out << QStringLiteral("REMUX");
        else if (s.source == Source::BluRay || s.source == Source::BDRip)
            out << QStringLiteral("BluRay");
        else if (s.source == Source::WEB_DL || s.source == Source::WEBRip
                 || s.source == Source::HDRip)
            out << QStringLiteral("WEB-DL");
        else if (s.source == Source::HDTV)
            out << QStringLiteral("HDTV");
    }

    if (s.codec == Codec::HEVC)     out << QStringLiteral("HEVC");
    else if (s.codec == Codec::AV1) out << QStringLiteral("AV1");

    if (s.hdrFormat == HdrFormat::DV || s.hdrFormat == HdrFormat::DV_HDR10) out << QStringLiteral("DV");
    else if (s.hdrFormat == HdrFormat::HDR10Plus)                          out << QStringLiteral("HDR10+");
    else if (s.hdrFormat == HdrFormat::HDR10)                             out << QStringLiteral("HDR10");
    else if (s.hdrFormat == HdrFormat::HLG)                               out << QStringLiteral("HLG");

    switch (s.audio.codec) {
    case AudioCodec::Atmos:     out << QStringLiteral("ATMOS"); break;
    case AudioCodec::TrueHD:    out << QStringLiteral("TRUEHD"); break;
    case AudioCodec::DTS_HD_MA: out << QStringLiteral("DTS-HD"); break;
    case AudioCodec::DTS:       out << QStringLiteral("DTS"); break;
    case AudioCodec::DDPlus:    out << QStringLiteral("DD+"); break;
    case AudioCodec::FLAC:      out << QStringLiteral("FLAC"); break;
    case AudioCodec::AAC:       out << QStringLiteral("AAC"); break;
    default:
        if (s.audio.channels == 1) out << QStringLiteral("MONO");
        break;
    }
    return out;
}

bool isPlayable(const ScoredStream& s)
{
    const bool hasDirectUrl = !s.url.isEmpty() && s.url != QStringLiteral("#");
    return hasDirectUrl || !s.infoHash.isEmpty();
}

} // namespace tankoban
```

- [ ] **Step 3: Register the new source in both CMake targets**

In `CMakeLists.txt`, add `src/ui/StreamRowPaint.cpp` to the `qt_add_executable(Tankoban3 ...)` source list (next to the other `src/ui/*.cpp`), AND to `add_executable(Tankoban3SelfTest ...)` (line ~205), so:

```cmake
add_executable(Tankoban3SelfTest src/selftest_main.cpp src/util/TimeFormat.cpp src/util/VolumeCurve.cpp src/ui/StreamRowPaint.cpp)
```

- [ ] **Step 4: Write the failing tests**

Append to `src/selftest_main.cpp` (add `#include "ui/StreamRowPaint.h"` at top; add a `checkInt`/`checkBool` helper and these cases inside `main()` before the final print):

```cpp
// ---- StreamRowPaint (sources-list rebuild) ----
{
    using namespace tankoban;
    ScoredStream a;
    a.addonId = "torrentio";
    a.infoHash = "abc";
    a.fileIdx = 0;
    check(streamRowKey(a), "torrentio|h:abc:0");

    ScoredStream u;
    u.addonId = "comet";
    u.url = "http://x/y.mkv";
    check(streamRowKey(u), "comet|u:http://x/y.mkv");

    ScoredStream q;
    q.resolution = Resolution::FullHD;
    check(qualityGroupOf(q), "1080p");
    q.resolution = Resolution::None;
    check(qualityGroupOf(q), "SD");

    // Unlabeled stream -> NO LABEL chip, not Labeled.
    ScoredStream bare;
    bare.source = Source::Other;
    bare.codec = Codec::Other;
    bare.audio.codec = AudioCodec::Other;
    check(streamBadges(bare).join("|"), "NO LABEL");

    // 1080p HEVC ATMOS -> ordered chips.
    ScoredStream rich;
    rich.resolution = Resolution::FullHD;
    rich.codec = Codec::HEVC;
    rich.audio.codec = AudioCodec::Atmos;
    rich.size = 5; // non-zero so confidence stays Labeled
    check(streamBadges(rich).join("|"), "1080p|HEVC|ATMOS");

    ScoredStream tor;
    tor.infoHash = "deadbeef";
    checkBool(isPlayable(tor), true, "torrent playable");
    ScoredStream dead;
    dead.url = "#";
    checkBool(isPlayable(dead), false, "placeholder not playable");
}
```

Add near the other helpers in `selftest_main.cpp`:

```cpp
static void check(const QString& got, const char* want, const char* label)
{
    if (got != QString::fromUtf8(want)) {
        std::printf("FAIL: %s got '%s' want '%s'\n", label, got.toUtf8().constData(), want);
        ++failures;
    }
}

static void checkBool(bool got, bool want, const char* label)
{
    if (got != want) { std::printf("FAIL: %s got %d want %d\n", label, got, want); ++failures; }
}
```

- [ ] **Step 5: Run the self-test to verify it FAILS to build/link**

Run: `cmd /c "C:\Users\Suprabha\Desktop\Tankoban 3\build.bat"`
Expected: builds (the helpers exist); then `ctest --test-dir out -R selftest --output-on-failure` → expect `SELFTEST OK` (the moved logic is correct by construction). If any chip-order assertion fails, fix the expectation to match `streamBadges` output — do NOT change the logic (it is a verbatim move).

- [ ] **Step 6: Point `StremioRow.cpp` at the shared helpers (no behavior change yet)**

In `src/ui/StremioRow.cpp`, delete the now-duplicated anonymous-namespace `qualityConfidence`/`streamBadges`/`addonInstanceKey`/`qualityGroupOf` definitions and `#include "ui/StreamRowPaint.h"`; call `tankoban::streamBadges(stream)` etc. (StremioRow still compiles + renders identically; this keeps one source of truth until StremioRow is retired in Task 8.) Build → `BUILD OK`.

- [ ] **Step 7: Commit**

```bash
git add src/ui/StreamRowPaint.h src/ui/StreamRowPaint.cpp src/selftest_main.cpp src/ui/StremioRow.cpp CMakeLists.txt
git commit -m "refactor(picker): extract pure StreamRowPaint helpers + self-test"
git push origin HEAD
```

---

### Task 2: `SourceListModel` (data, incremental merge) + unit-test the merge

**Files:**
- Create: `src/ui/SourceListModel.h`, `src/ui/SourceListModel.cpp`
- Modify: `CMakeLists.txt` (both targets), `src/selftest_main.cpp`

- [ ] **Step 1: Create the header**

`src/ui/SourceListModel.h`:

```cpp
// Tankoban 3 - source rows as DATA (no widgets). Incremental merge by stream key so
// streaming partials never rebuild the view. ScoredStream is exposed via a single role.
#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QVector>

#include "core/StreamModels.h"

namespace tankoban {

class SourceListModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        StreamRole = Qt::UserRole + 1,
        RankRole,
        KeyRole
    };

    explicit SourceListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;

    // Merge the full ranked set: insert new keys, update changed rows/ranks in place,
    // remove keys no longer present. Never a blanket reset (preserves view state).
    void setStreams(const QVector<ScoredStream>& streams);
    void clear();

    ScoredStream at(int row) const;   // convenience for delegate/tests

private:
    QVector<ScoredStream> m_rows;
    QVector<int> m_rankByRow;          // current incoming ranked ordinal per row
    QHash<QString, int> m_indexByKey;   // streamRowKey -> row
    void reindex();
};

} // namespace tankoban

Q_DECLARE_METATYPE(tankoban::ScoredStream)
```

`Q_DECLARE_METATYPE` belongs after the fully defined `ScoredStream` is visible (this header includes `core/StreamModels.h`) and outside the namespace. No `qRegisterMetaType` is required for `QVariant::fromValue` in this same-thread model/view path; add registration only if a future queued signal transports `ScoredStream` by value.

- [ ] **Step 2: Create the implementation**

`src/ui/SourceListModel.cpp`:

```cpp
// Tankoban 3 - see SourceListModel.h.
#include "ui/SourceListModel.h"
#include "ui/StreamRowPaint.h"

#include <QSet>

namespace tankoban {

SourceListModel::SourceListModel(QObject* parent) : QAbstractListModel(parent) {}

int SourceListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

QVariant SourceListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return {};
    if (role == StreamRole)
        return QVariant::fromValue(m_rows[index.row()]);
    if (role == RankRole)
        return m_rankByRow.value(index.row(), index.row());
    if (role == KeyRole)
        return streamRowKey(m_rows[index.row()]);
    return {};
}

ScoredStream SourceListModel::at(int row) const
{
    return (row >= 0 && row < m_rows.size()) ? m_rows[row] : ScoredStream{};
}

void SourceListModel::reindex()
{
    m_indexByKey.clear();
    for (int i = 0; i < m_rows.size(); ++i)
        m_indexByKey.insert(streamRowKey(m_rows[i]), i);
}

void SourceListModel::setStreams(const QVector<ScoredStream>& streams)
{
    // 0) Deduplicate by key while preserving the first ranked occurrence.
    QVector<ScoredStream> unique;
    unique.reserve(streams.size());
    QSet<QString> seen;
    for (const ScoredStream& s : streams) {
        const QString key = streamRowKey(s);
        if (seen.contains(key))
            continue;
        seen.insert(key);
        unique.push_back(s);
    }

    // 1) Update existing + collect genuinely-new. The proxy preserves the incoming
    //    rank via RankRole, so we store the current ranked ordinal beside each row.
    QSet<QString> incomingKeys;
    incomingKeys.reserve(unique.size());
    QVector<ScoredStream> fresh;
    QVector<int> freshRanks;
    for (int rank = 0; rank < unique.size(); ++rank) {
        const ScoredStream& s = unique[rank];
        const QString key = streamRowKey(s);
        incomingKeys.insert(key);
        const auto it = m_indexByKey.constFind(key);
        if (it != m_indexByKey.constEnd()) {
            m_rows[it.value()] = s;   // update in place (score/fields may have changed)
            m_rankByRow[it.value()] = rank;
            const QModelIndex idx = index(it.value());
            emit dataChanged(idx, idx, {StreamRole, RankRole, KeyRole});
        } else {
            fresh.push_back(s);
            freshRanks.push_back(rank);
        }
    }

    // 2) Remove rows whose key vanished (e.g. dedup at streamsReady). Remove from the
    //    back so indices stay valid.
    for (int i = m_rows.size() - 1; i >= 0; --i) {
        if (!incomingKeys.contains(streamRowKey(m_rows[i]))) {
            beginRemoveRows(QModelIndex(), i, i);
            m_rows.remove(i);
            m_rankByRow.remove(i);
            endRemoveRows();
        }
    }

    // 3) Append the fresh ones.
    if (!fresh.isEmpty()) {
        const int first = m_rows.size();
        beginInsertRows(QModelIndex(), first, first + fresh.size() - 1);
        m_rows += fresh;
        m_rankByRow += freshRanks;
        endInsertRows();
    }

    reindex();
}

void SourceListModel::clear()
{
    if (m_rows.isEmpty())
        return;
    beginResetModel();
    m_rows.clear();
    m_rankByRow.clear();
    m_indexByKey.clear();
    endResetModel();
}

} // namespace tankoban
```

- [ ] **Step 3: Register in both CMake targets**

Add `src/ui/SourceListModel.cpp` to `Tankoban3` sources AND append to the `Tankoban3SelfTest` source list.

- [ ] **Step 4: Write the failing merge test**

Append to `selftest_main.cpp` `main()` (add `#include "ui/SourceListModel.h"`):

```cpp
// ---- SourceListModel incremental merge ----
{
    using namespace tankoban;
    auto mk = [](const char* addon, const char* hash, int score) {
        ScoredStream s; s.addonId = addon; s.infoHash = hash; s.fileIdx = 0; s.score = score; return s;
    };
    SourceListModel m;
    QVector<ScoredStream> p1{ mk("a","h1",10), mk("a","h2",20) };
    m.setStreams(p1);
    checkInt(m.rowCount(), 2, "merge p1 count");
    checkInt(m.index(0,0).data(SourceListModel::RankRole).toInt(), 0, "p1 h1 rank");

    // p2 keeps h1 (updated score), drops h2, adds h3 -> rows: h1, h3.
    QVector<ScoredStream> p2{ mk("a","h3",30), mk("a","h1",15), mk("a","h3",99) };
    m.setStreams(p2);
    checkInt(m.rowCount(), 2, "merge p2 count");
    check(m.at(0).infoHash, "h1", "h1 kept");
    checkInt(m.index(0,0).data(SourceListModel::RankRole).toInt(), 1, "h1 reranked");
    check(m.at(1).infoHash, "h3", "h3 appended");
    m.clear();
    checkInt(m.rowCount(), 0, "clear");
}
```

Add helper:

```cpp
static void checkInt(int got, int want, const char* label)
{
    if (got != want) { std::printf("FAIL: %s got %d want %d\n", label, got, want); ++failures; }
}
```

- [ ] **Step 5: Build + run self-test**

Run: `cmd /c "...\build.bat"` then `ctest --test-dir out -R selftest --output-on-failure`
Expected: `SELFTEST OK`. If `merge p2 count` fails, the remove/append ordering is wrong — re-check Step 2's loop directions.

- [ ] **Step 6: Commit**

```bash
git add src/ui/SourceListModel.h src/ui/SourceListModel.cpp src/selftest_main.cpp CMakeLists.txt
git commit -m "feat(picker): SourceListModel with incremental key-merge + test"
git push origin HEAD
```

---

### Task 3: `SourceFilterProxy` (rank-preserving sort + addon/quality filter)

**Files:**
- Create: `src/ui/SourceFilterProxy.h`, `src/ui/SourceFilterProxy.cpp`
- Modify: `CMakeLists.txt` (both targets), `src/selftest_main.cpp`

- [ ] **Step 1: Header**

`src/ui/SourceFilterProxy.h`:

```cpp
// Tankoban 3 - preserves buildPicker rank + filters (addon, quality) the SourceListModel for
// the view. Filter changes are O(visible) repaints, not widget rebuilds.
#pragma once

#include <QSortFilterProxyModel>
#include <QString>

namespace tankoban {

class SourceFilterProxy : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit SourceFilterProxy(QObject* parent = nullptr);

    void setAddonFilter(const QString& key);     // "all" or an addonInstanceKey
    void setQualityFilter(const QString& group); // "all" / "4K" / "1080p" / "720p" / "SD"

protected:
    bool lessThan(const QModelIndex& l, const QModelIndex& r) const override;
    bool filterAcceptsRow(int srcRow, const QModelIndex& srcParent) const override;

private:
    QString m_addon = QStringLiteral("all");
    QString m_quality = QStringLiteral("all");
};

} // namespace tankoban
```

- [ ] **Step 2: Implementation**

`src/ui/SourceFilterProxy.cpp`:

```cpp
// Tankoban 3 - see SourceFilterProxy.h.
#include "ui/SourceFilterProxy.h"
#include "ui/SourceListModel.h"
#include "ui/StreamRowPaint.h"

namespace tankoban {

SourceFilterProxy::SourceFilterProxy(QObject* parent) : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
    setSortRole(SourceListModel::RankRole);
    sort(0, Qt::AscendingOrder);   // lessThan keeps current buildPicker rank first
}

void SourceFilterProxy::setAddonFilter(const QString& key)
{
    if (m_addon == key) return;
    m_addon = key;
    invalidateFilter();
}

void SourceFilterProxy::setQualityFilter(const QString& group)
{
    if (m_quality == group) return;
    m_quality = group;
    invalidateFilter();
}

bool SourceFilterProxy::lessThan(const QModelIndex& l, const QModelIndex& r) const
{
    const int ar = l.data(SourceListModel::RankRole).toInt();
    const int br = r.data(SourceListModel::RankRole).toInt();
    if (ar != br) return ar < br;                          // preserve buildPicker order

    const auto a = l.data(SourceListModel::StreamRole).value<ScoredStream>();
    const auto b = r.data(SourceListModel::StreamRole).value<ScoredStream>();
    if (a.score != b.score) return a.score > b.score;      // fallback only
    return a.nativeIdx < b.nativeIdx;                       // stable fallback
}

bool SourceFilterProxy::filterAcceptsRow(int srcRow, const QModelIndex& srcParent) const
{
    const QModelIndex idx = sourceModel()->index(srcRow, 0, srcParent);
    const auto s = idx.data(SourceListModel::StreamRole).value<ScoredStream>();
    if (m_addon != QStringLiteral("all") && addonInstanceKey(s) != m_addon) return false;
    if (m_quality != QStringLiteral("all") && qualityGroupOf(s) != m_quality) return false;
    return true;
}

} // namespace tankoban
```

- [ ] **Step 3: Register in both CMake targets** (`src/ui/SourceFilterProxy.cpp`).

- [ ] **Step 4: Write the failing proxy test**

Append to `selftest_main.cpp` (add `#include "ui/SourceFilterProxy.h"`):

```cpp
// ---- SourceFilterProxy rank-preserving sort + filter ----
{
    using namespace tankoban;
    auto mk = [](const char* addon, const char* hash, int score, Resolution res) {
        ScoredStream s; s.addonId = addon; s.infoHash = hash; s.fileIdx = 0;
        s.score = score; s.resolution = res; return s;
    };
    SourceListModel m;
    // Incoming order is already buildPicker-ranked; score order is intentionally different.
    m.setStreams({ mk("a","h1",10,Resolution::HD),
                   mk("b","h2",30,Resolution::FourK),
                   mk("a","h3",20,Resolution::FullHD) });
    SourceFilterProxy proxy;
    proxy.setSourceModel(&m);
    // Sorted by incoming rank, not score desc: h1, h2, h3.
    check(proxy.index(0,0).data(SourceListModel::StreamRole).value<ScoredStream>().infoHash, "h1", "rank0");
    check(proxy.index(2,0).data(SourceListModel::StreamRole).value<ScoredStream>().infoHash, "h3", "rank2");
    // Filter addon "a" (addonUrl empty -> addonInstanceKey == addonId) -> 2 rows.
    proxy.setAddonFilter("a");
    checkInt(proxy.rowCount(), 2, "addon filter a");
    proxy.setAddonFilter("all");
    proxy.setQualityFilter("4K");
    checkInt(proxy.rowCount(), 1, "quality filter 4K");
}
```

- [ ] **Step 5: Build + run self-test** → `SELFTEST OK`. (Requires a `QGuiApplication`-free path; `QSortFilterProxyModel` needs no event loop for synchronous sort/filter.)

- [ ] **Step 6: Commit**

```bash
git add src/ui/SourceFilterProxy.h src/ui/SourceFilterProxy.cpp src/selftest_main.cpp CMakeLists.txt
git commit -m "feat(picker): SourceFilterProxy rank-sort + addon/quality filter + test"
git push origin HEAD
```

---

### Task 4: `SourceRowDelegate` — paint a row pixel-1:1 (+ cached icons + sub-rect geometry)

**Files:**
- Create: `src/ui/SourceRowDelegate.h`, `src/ui/SourceRowDelegate.cpp`
- Modify: `CMakeLists.txt` (Tankoban3 only — uses QtWidgets/Svg, not in the self-test)

> No unit test (painting). Verified by build + eye-smoke in Task 6/9. This task just lands the delegate so Task 6 can wire it.

- [ ] **Step 1: Header (with the shared sub-rect geometry the view needs)**

`src/ui/SourceRowDelegate.h`:

```cpp
// Tankoban 3 - paints one source row 1:1 with the old StremioRow, using QPainter.
// Fixed row height for stable virtualization; icons cached in QPixmapCache. Exposes
// play/copy sub-rects so SourceListView can hit-test clicks + drive hover.
#pragma once

#include <QStyledItemDelegate>
#include <QRect>
#include <QString>

namespace tankoban {

class SourceRowDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    static constexpr int kRowHeight = 112;   // fixed; fits headline + 2 desc lines + badges + status

    explicit SourceRowDelegate(QObject* parent = nullptr);

    void paint(QPainter* p, const QStyleOptionViewItem& opt, const QModelIndex& idx) const override;
    QSize sizeHint(const QStyleOptionViewItem& opt, const QModelIndex& idx) const override;

    // Geometry helpers (pure, given the row rect) for SourceListView hit-testing.
    static QRect playRect(const QRect& row);
    static QRect copyRect(const QRect& row);

    void setHoveredKey(const QString& key) { m_hoveredKey = key; }
    void setCopiedKey(const QString& key)  { m_copiedKey = key;  }  // shows the check glyph on that row

private:
    QString m_hoveredKey;
    QString m_copiedKey;
};

} // namespace tankoban
```

- [ ] **Step 2: Implementation (faithful port of `StremioRow` paint)**

`src/ui/SourceRowDelegate.cpp` — paints the row reproducing `StremioRow`'s geometry/colors. Key contract (match `StremioRow.cpp`):

```cpp
// Tankoban 3 - see SourceRowDelegate.h.
#include "ui/SourceRowDelegate.h"
#include "ui/SourceListModel.h"
#include "ui/StreamRowPaint.h"

#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPixmapCache>
#include <QSvgRenderer>

namespace tankoban {
namespace {
const QColor kRowBg(26, 29, 36);          // a=0.40 applied at fill
const QColor kTile(26, 29, 36);
const QColor kHead(0xf3, 0xf1, 0xea);
const QColor kMuted(0xaa, 0xb1, 0xbd);
const QColor kGold(0xe8, 0xb9, 0x23);
const QColor kInk(0x12, 0x13, 0x17);

// Render an SVG glyph once into a cached pixmap (QPixmapCache, dpr-aware).
QPixmap glyph(const QString& key, const QString& inner, const QColor& color, int px, bool filled, qreal dpr)
{
    const QString ck = QStringLiteral("srd:%1:%2:%3").arg(key, color.name()).arg(int(px * dpr));
    QPixmap pm;
    if (QPixmapCache::find(ck, &pm)) return pm;
    const QString svg = filled
        ? QStringLiteral("<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='%1' stroke='none'>%2</svg>").arg(color.name(), inner)
        : QStringLiteral("<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='none' stroke='%1' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'>%2</svg>").arg(color.name(), inner);
    QSvgRenderer r(svg.toUtf8());
    pm = QPixmap(int(px * dpr), int(px * dpr));
    pm.fill(Qt::transparent);
    { QPainter pp(&pm); r.render(&pp); }
    pm.setDevicePixelRatio(dpr);
    QPixmapCache::insert(ck, pm);
    return pm;
}
const QString kPlayGlyph = QStringLiteral("<polygon points='8 5 19 12 8 19'/>");
const QString kCopyGlyph = QStringLiteral("<rect width='14' height='14' x='8' y='8' rx='2' ry='2'/><path d='M4 16c-1.1 0-2-.9-2-2V4c0-1.1.9-2 2-2h10c1.1 0 2 .9 2 2'/>");
const QString kCheckGlyph = QStringLiteral("<path d='M20 6 9 17l-5-5'/>");
} // namespace

SourceRowDelegate::SourceRowDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

QSize SourceRowDelegate::sizeHint(const QStyleOptionViewItem& opt, const QModelIndex&) const
{
    return QSize(opt.rect.width(), kRowHeight);
}

QRect SourceRowDelegate::playRect(const QRect& row)
{
    return QRect(row.right() - 20 - 64, row.center().y() - 32, 64, 64);
}
QRect SourceRowDelegate::copyRect(const QRect& row)
{
    return QRect(row.right() - 20 - 64 - 8 - 36, row.center().y() - 18, 36, 36);
}

void SourceRowDelegate::paint(QPainter* p, const QStyleOptionViewItem& opt, const QModelIndex& idx) const
{
    const auto s = idx.data(SourceListModel::StreamRole).value<ScoredStream>();
    const QString key = idx.data(SourceListModel::KeyRole).toString();
    const bool hovered = key == m_hoveredKey;
    const bool playable = isPlayable(s);
    p->save();
    p->setRenderHint(QPainter::Antialiasing);

    // Card background (rgba(26,29,36,0.40), 1px border, 16px radius) inset by ~no extra
    // margin — the view's spacing provides the gap.
    QRect card = opt.rect.adjusted(0, 4, 0, -4);
    QPainterPath bg; bg.addRoundedRect(card, 16, 16);
    QColor fill = kRowBg; fill.setAlphaF(hovered ? 0.55 : 0.40);
    p->fillPath(bg, fill);
    p->setPen(QPen(QColor(255, 255, 255, 13), 1)); p->drawPath(bg);

    const int left = card.left() + 20;
    // Logo tile 56x56.
    QRect tile(left, card.center().y() - 28, 56, 56);
    QPainterPath tp; tp.addRoundedRect(tile, 12, 12);
    p->fillPath(tp, kTile);
    p->setPen(QPen(QColor(255,255,255,16),1)); p->drawPath(tp);
    const QString addon = !s.addonName.isEmpty() ? s.addonName : s.addonId;
    p->setPen(kMuted);
    { QFont f("Inter"); f.setPixelSize(22); f.setWeight(QFont::Bold); p->setFont(f); }
    p->drawText(tile, Qt::AlignCenter, addon.trimmed().left(1).toUpper());

    // Content column.
    const int cx = left + 56 + 20;
    const int cw = SourceRowDelegate::copyRect(card).left() - 12 - cx;
    int y = card.top() + 20;
    // Headline.
    p->setPen(kHead);
    { QFont f("Inter"); f.setPixelSize(16); f.setWeight(QFont::DemiBold); p->setFont(f); }
    const QString head = !s.name.trimmed().isEmpty() ? s.name.trimmed() : addon;
    p->drawText(QRect(cx, y, cw, 22), Qt::AlignVCenter | Qt::AlignLeft,
                p->fontMetrics().elidedText(head, Qt::ElideRight, cw));
    y += 24;
    // Description (filename), elided to one line.
    const QString desc = !s.title.trimmed().isEmpty() ? s.title.trimmed() : s.description.trimmed();
    if (!desc.isEmpty()) {
        p->setPen(kMuted);
        { QFont f("Inter"); f.setPixelSize(14); p->setFont(f); }
        p->drawText(QRect(cx, y, cw, 20), Qt::AlignVCenter | Qt::AlignLeft,
                    p->fontMetrics().elidedText(desc, Qt::ElideRight, cw));
        y += 22;
    }
    // Badge chips.
    int bx = cx;
    { QFont f("Inter"); f.setPixelSize(11); f.setWeight(QFont::DemiBold); p->setFont(f); }
    static const QSet<QString> danger = {"CAM","TS","TC","SCR","NO LABEL","UNKNOWN"};
    for (const QString& label : streamBadges(s)) {
        const int w = p->fontMetrics().horizontalAdvance(label) + 14;
        QRect chip(bx, y, w, 18);
        QPainterPath cp; cp.addRoundedRect(chip, 7, 7);
        p->fillPath(cp, QColor(18,19,23,217));
        const bool d = danger.contains(label);
        p->setPen(d ? QColor(0xe5,0x8a,0x8d) : kMuted);
        if (d) { p->setPen(QPen(QColor(229,72,77,115),1)); p->drawPath(cp); p->setPen(QColor(0xe5,0x8a,0x8d)); }
        p->drawText(chip, Qt::AlignCenter, label);
        bx += w + 6;
        if (bx > cx + cw - 40) break;   // don't overflow into actions
    }
    y += 22;
    // Status line.
    QString status;
    const bool isTorrent = !s.infoHash.isEmpty();
    const bool hasDirect = !s.url.isEmpty() && s.url != QStringLiteral("#");
    if (isTorrent && !hasDirect) status = QStringLiteral("Torrent · P2P stream");
    else if (!playable && !s.externalUrl.isEmpty()) status = QStringLiteral("External source");
    if (!status.isEmpty()) {
        p->setPen(QColor(0x8b,0x90,0x9b));
        { QFont f("Inter"); f.setPixelSize(13); f.setWeight(QFont::DemiBold); p->setFont(f); }
        p->drawText(QRect(cx, y, cw, 16), Qt::AlignVCenter | Qt::AlignLeft, status);
    }

    // Copy button (glyph only).
    const qreal dpr = p->device()->devicePixelRatioF();
    const QRect cr = SourceRowDelegate::copyRect(card);
    const bool showCopy = hasDirect || !s.externalUrl.isEmpty();
    if (showCopy) {
        const bool copied = key == m_copiedKey;
        const QPixmap g = copied ? glyph("check", kCheckGlyph, kGold, 16, false, dpr)
                                 : glyph("copy", kCopyGlyph, QColor(0x8b,0x90,0x9b), 16, false, dpr);
        p->drawPixmap(cr.center() - QPoint(8,8), g);
    }
    // Play button (64 circle).
    const QRect pr = SourceRowDelegate::playRect(card);
    QPainterPath circ; circ.addEllipse(pr);
    if (playable) { QColor g = kGold; if (hovered) g.setAlphaF(0.90); p->fillPath(circ, g); }
    else { p->fillPath(circ, QColor(26,29,36,179)); p->setPen(QPen(QColor(255,255,255,16),1)); p->drawPath(circ); }
    const QPixmap pg = glyph("play", kPlayGlyph, playable ? kInk : QColor(0x8b,0x90,0x9b), 26, true, dpr);
    p->drawPixmap(pr.center() - QPoint(13,13), pg);

    p->restore();
}

} // namespace tankoban
```

- [ ] **Step 3: Register `src/ui/SourceRowDelegate.cpp`** in the `Tankoban3` source list. Ensure `Qt6::Svg` is already linked (it is — `StremioRow` used `QSvgRenderer`).

- [ ] **Step 4: Build** → `BUILD OK`. (No runtime wiring yet; Task 6 mounts it.)

- [ ] **Step 5: Commit**

```bash
git add src/ui/SourceRowDelegate.h src/ui/SourceRowDelegate.cpp CMakeLists.txt
git commit -m "feat(picker): SourceRowDelegate paints source row 1:1 (cached icons)"
git push origin HEAD
```

---

### Task 5: `SourceListView` — hover tracking + play/copy hit-testing

**Files:**
- Create: `src/ui/SourceListView.h`, `src/ui/SourceListView.cpp`
- Modify: `CMakeLists.txt` (Tankoban3)

- [ ] **Step 1: Header**

`src/ui/SourceListView.h`:

```cpp
// Tankoban 3 - QListView that drives delegate hover + maps clicks to the play/copy
// sub-rects (the rows are painted, not widgets). Emits stream-level signals.
#pragma once

#include <QListView>

namespace tankoban {

class SourceRowDelegate;

class SourceListView : public QListView {
    Q_OBJECT
public:
    explicit SourceListView(QWidget* parent = nullptr);

signals:
    void playActivated(const QModelIndex& index);
    void copyActivated(const QModelIndex& index);

protected:
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void leaveEvent(QEvent* e) override;

private:
    SourceRowDelegate* rowDelegate() const;
};

} // namespace tankoban
```

- [ ] **Step 2: Implementation**

`src/ui/SourceListView.cpp`:

```cpp
// Tankoban 3 - see SourceListView.h.
#include "ui/SourceListView.h"
#include "ui/SourceListModel.h"
#include "ui/SourceRowDelegate.h"
#include "ui/StreamRowPaint.h"

#include <QMouseEvent>

namespace tankoban {

SourceListView::SourceListView(QWidget* parent) : QListView(parent)
{
    setMouseTracking(true);
    setFrameShape(QFrame::NoFrame);
    setSelectionMode(QAbstractItemView::NoSelection);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setUniformItemSizes(true);   // fixed row height -> fast virtualization
    setSpacing(8);
    setViewMode(QListView::ListMode);
    setFlow(QListView::TopToBottom);
    setResizeMode(QListView::Adjust);
    setSizeAdjustPolicy(QAbstractScrollArea::AdjustIgnored); // never expand to all 500 rows
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumHeight(420);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

SourceRowDelegate* SourceListView::rowDelegate() const
{
    return qobject_cast<SourceRowDelegate*>(itemDelegate());
}

void SourceListView::mouseMoveEvent(QMouseEvent* e)
{
    const QModelIndex idx = indexAt(e->pos());
    if (auto* d = rowDelegate()) { d->setHoveredKey(idx.isValid() ? idx.data(SourceListModel::KeyRole).toString() : QString()); viewport()->update(); }
    const QRect row = visualRect(idx);
    const auto s = idx.isValid() ? idx.data(SourceListModel::StreamRole).value<ScoredStream>() : ScoredStream{};
    const bool playable = idx.isValid() && isPlayable(s);
    const bool copyable = idx.isValid()
        && ((!s.url.isEmpty() && s.url != QStringLiteral("#")) || !s.externalUrl.isEmpty());
    const bool overBtn = idx.isValid()
        && ((playable && SourceRowDelegate::playRect(row).contains(e->pos()))
            || (copyable && SourceRowDelegate::copyRect(row).contains(e->pos())));
    setCursor(overBtn ? Qt::PointingHandCursor : Qt::ArrowCursor);
    QListView::mouseMoveEvent(e);
}

void SourceListView::mouseReleaseEvent(QMouseEvent* e)
{
    const QModelIndex idx = indexAt(e->pos());
    if (idx.isValid()) {
        const QRect row = visualRect(idx);
        const auto s = idx.data(SourceListModel::StreamRole).value<ScoredStream>();
        const bool playable = isPlayable(s);
        const bool copyable = ((!s.url.isEmpty() && s.url != QStringLiteral("#")) || !s.externalUrl.isEmpty());
        if (playable && SourceRowDelegate::playRect(row).contains(e->pos())) { emit playActivated(idx); return; }
        if (copyable && SourceRowDelegate::copyRect(row).contains(e->pos())) { emit copyActivated(idx); return; }
    }
    QListView::mouseReleaseEvent(e);
}

void SourceListView::leaveEvent(QEvent* e)
{
    if (auto* d = rowDelegate()) { d->setHoveredKey(QString()); viewport()->update(); }
    QListView::leaveEvent(e);
}

} // namespace tankoban
```

- [ ] **Step 3: Register `src/ui/SourceListView.cpp`** in the `Tankoban3` source list. Build → `BUILD OK`.

- [ ] **Step 4: Commit**

```bash
git add src/ui/SourceListView.h src/ui/SourceListView.cpp CMakeLists.txt
git commit -m "feat(picker): SourceListView hover + play/copy hit-testing"
git push origin HEAD
```

---

### Task 6: Swap `StreamList` internals to the view stack (keep public API + filter bar)

**Files:**
- Modify: `src/ui/StreamList.h`, `src/ui/StreamList.cpp`

- [ ] **Step 1: Replace the rows container + reconcile machinery with the view stack**

In `StreamList.h`: remove `m_rowsContainer`/`m_rowsLayout`/`m_rowByKey`/`reconcileRows`/`visibleStreams`; add members `SourceListView* m_view`, `SourceListModel* m_model`, `SourceFilterProxy* m_proxy`, `SourceRowDelegate* m_delegate`. Forward-declare them. Keep the public API (`setStreams`, `clearStreams`, `streamActivated`) and the filter-bar members (`m_addonButton`, `m_qualityBar`, `m_pending`, etc.).

In `StreamList.cpp` constructor: build `m_model` → `m_proxy(setSourceModel(m_model))` → `m_view(setModel(m_proxy), setItemDelegate(m_delegate))`; insert `m_view` into the column where `m_rowsContainer` used to go. Wire:

Important: the `QListView` must own the source-list scrolling viewport. Do not set `AdjustToContents`, do not size it to `rowCount * kRowHeight`, and do not reintroduce a rows container that expands to every source. A bounded/expanding list viewport is what keeps only visible delegates painted and satisfies DoD #4/#5.

```cpp
connect(m_view, &SourceListView::playActivated, this, [this](const QModelIndex& i) {
    emit streamActivated(i.data(SourceListModel::StreamRole).value<ScoredStream>());
});
connect(m_view, &SourceListView::copyActivated, this, [this](const QModelIndex& i) {
    const auto s = i.data(SourceListModel::StreamRole).value<ScoredStream>();
    const QString link = (!s.url.isEmpty() && s.url != "#") ? s.url : s.externalUrl;
    if (!link.isEmpty()) {
        QGuiApplication::clipboard()->setText(link);
        m_delegate->setCopiedKey(i.data(SourceListModel::KeyRole).toString());
        m_view->viewport()->update();
        QTimer::singleShot(1200, this, [this]{ m_delegate->setCopiedKey(QString()); m_view->viewport()->update(); });
    }
});
```

- [ ] **Step 2: Route `setStreams`/`clearStreams` to the model; filters to the proxy**

```cpp
void StreamList::setStreams(const QVector<ScoredStream>& streams, const Options& options)
{
    m_streams = streams;            // kept for the addon/quality bar counts
    m_options = options;
    // Preserve the user's addon filter if still present, else drop to "all" (unchanged logic).
    // ... existing m_addonFilter validation ...
    refreshAddonButton();
    rebuildQualityBar();
    m_model->setStreams(streams);   // incremental merge; view repaints visible rows only
    m_proxy->setAddonFilter(m_addonFilter);
    m_proxy->setQualityFilter(m_qualityFilter);
    updatePending();
}

void StreamList::clearStreams()
{
    m_streams.clear();
    m_addonFilter = "all"; m_qualityFilter = "all";
    refreshAddonButton(); rebuildQualityBar();
    m_model->clear();
    updatePending();
}
```

In the addon-menu handler and quality-pill handler, replace the old `rebuildRows()` calls with `m_proxy->setAddonFilter(m_addonFilter)` / `m_proxy->setQualityFilter(m_qualityFilter)` (instant, no rebuild).

- [ ] **Step 3: Build** → `BUILD OK`.

- [ ] **Step 4: Eye-smoke (Hemanth) — pixel parity + responsiveness**

Launch, open Spider-Noir (500 sources). Verify: rows look identical to before; scrolling is smooth; switching All/4K/1080p/SD is instant; play button plays; copy button copies + shows the check glyph; hover brightens the play button. (Formal DoD pass is Task 9.)

- [ ] **Step 5: Commit**

```bash
git add src/ui/StreamList.h src/ui/StreamList.cpp
git commit -m "feat(picker): StreamList uses native model/view list (virtualized rows)"
git push origin HEAD
```

---

### Task 7: Restore the see-through backdrop (revert the opaque diagnostic)

**Files:**
- Modify: `src/ui/PlayPickerPage.cpp`

- [ ] **Step 1: Revert the diagnostic**

Restore the original translucent scroll setup: the QSS `QScrollArea#PlayPickerScroll { background: transparent; }` + `> QWidget { background: transparent; }`, and the viewport attributes `setAttribute(WA_TranslucentBackground, true)` + `viewport()->setAutoFillBackground(false)` + `viewport()->setAttribute(WA_TranslucentBackground, true)` (the pre-diagnostic state).

- [ ] **Step 2: Build + eye-smoke against DoD #2**

Open Spider-Noir; the backdrop is visible again; **scroll must stay smooth** (the list is now virtualized). If — and only if — scrolling regresses with the translucent backdrop, STOP and apply the documented fallback (backdrop fixed behind the header, list on opaque dark) in a follow-up task; do not silently keep the opaque diagnostic.

- [ ] **Step 3: Commit**

```bash
git add src/ui/PlayPickerPage.cpp
git commit -m "fix(picker): restore see-through backdrop (revert opaque scroll diagnostic)"
git push origin HEAD
```

---

### Task 8: Retire `StremioRow`

**Files:**
- Delete: `src/ui/StremioRow.h`, `src/ui/StremioRow.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Confirm no remaining references**

Run: `grep -rn "StremioRow" src/` — expect only the files being deleted. (If `StreamList.cpp` still includes it, remove the include.)

- [ ] **Step 2: Delete + deregister**

Remove `src/ui/StremioRow.cpp` from `CMakeLists.txt`; delete both files. Build → `BUILD OK`.

- [ ] **Step 3: Commit**

```bash
git add -A src/ui/StremioRow.h src/ui/StremioRow.cpp CMakeLists.txt
git commit -m "chore(picker): retire StremioRow (replaced by SourceRowDelegate)"
git push origin HEAD
```

---

### Task 9: Definition-of-Done acceptance pass

**Files:** none (verification only)

- [ ] **Step 1: Run the full self-test**

Run: `ctest --test-dir out -R selftest --output-on-failure` → `SELFTEST OK`.

- [ ] **Step 2: Eye-smoke the 8 DoD items on Spider-Noir (500 sources)**

Walk the checklist in spec §8 and confirm each:
1. No window freeze while results stream in.
2. Scroll smooth, no hitching.
3. No row jitter as partials arrive.
4. Switch All→4K→1080p→SD repeatedly → instant.
5. No hundreds of live row widgets (off-screen rows are data only).
6. Icons cached (no per-paint SVG rasterization).
7. Monster-long torrent name elides, row height stable.
8. One-by-one results don't thrash the UI.

Any failure → return to the owning task; do not mark done.

- [ ] **Step 3: Request cross-engine review (player/stream/render work — do not self-certify)**

Package the diff for Codex/Opus review (correctness of the model merge, the delegate hit-testing, the proxy sort stability, and that no DoD item regressed). Address findings before merging to master.

- [ ] **Step 4: Final commit / merge-to-master per Rule 23 (after review)**

```bash
git push origin HEAD
# merge to master when build-green + Hemanth-smoked + review-clean (merge-at-checkpoint)
```

---

## Self-Review (filled in by the plan author)

**Spec coverage:** §2 components → Tasks 2–6; §3 data flow → Task 6 Step 2 + Task 2 merge; §4 paint contract → Task 4; §4.2 pure helpers → Task 1; §4.3 fixed-height/elision → Task 4 (`kRowHeight` + `elidedText`); §5 hover/click → Tasks 4–5; §6 backdrop → Task 7; §7 files → all tasks; §8 DoD → Task 9. No gaps.

**Placeholder scan:** no TBD/TODO; every code step shows real code; test code is concrete.

**Type consistency:** `streamRowKey`, `addonInstanceKey`, `qualityGroupOf`, `streamBadges`, `qualityConfidence`, `isPlayable` (Task 1) are the exact names used in Tasks 2/3/4/5. `SourceListModel::StreamRole`/`RankRole`/`KeyRole`, `setStreams`, `at`, `clear` consistent across Tasks 2/3/5/6. `SourceRowDelegate::playRect`/`copyRect`/`setHoveredKey`/`setCopiedKey`/`kRowHeight` consistent across Tasks 4/5/6. `SourceFilterProxy::setAddonFilter`/`setQualityFilter` consistent across Tasks 3/6.

**Open follow-ups (not blocking):** Harbor image-badge-pack + addon-logo parity (separate slice); backdrop fixed-header fallback (only if Task 7 regresses scroll).
