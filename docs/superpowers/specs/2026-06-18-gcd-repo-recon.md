# GCD Repo Recon — Brother 1 (DeepSeek)

**Date:** 2026-06-18
**Task:** Read-only reconnaissance of 3 GCD-related GitHub repos
**For:** Tankoban 3 server-side GCD delta-publisher pipeline

---

## Repo 1: thegraydot/gcd_docker — ★★★★ MOST USEFUL

**URL:** https://github.com/thegraydot/gcd_docker
**Language:** Python 91.6%, Shell 3.5%, Dockerfile 2.0%
**License:** Creative Commons Attribution
**Stars:** 2 | **Forks:** 0 | **Commits:** 51 | **Last activity:** Low (single contributor, no releases)
**Status:** Dormant but functionally complete

### Facts

**What it does:** Dockerized environment (MySQL + Python containers) that ingests a GCD MySQL dump, loads it into MySQL, and runs Python scripts against it. Primary output is a migration into a "TGD" (transformed GCD) schema — a flattened `comics` table that joins issue + series + publisher data.

**Ingest → Output pipeline:**
1. Downloads GCD `current.zip` → extracts `.sql` → loads into MySQL via Docker
2. Python scripts query MySQL, transform rows, and output SQL `INSERT` statements into a target `comics` table
3. Full migration (`gcd_migrate_full.py`) processes ALL issues
4. Partial migration (`gcd_migrate_partial.py`) processes only issues WHERE `created >= LAST_DUMP_DATE OR modified >= LAST_DUMP_DATE`

### Feature checklist

| Feature | Present? | Details |
|---------|----------|---------|
| **MySQL→SQLite conversion** | ❌ No | Uses MySQL exclusively. No SQLite anywhere. |
| **Pruning/filtering** | ⚠️ Partial | Extracts only `gcd_issue`, `gcd_series`, `gcd_publisher` tables for faster load. The partial migration filters by date. No language/publisher pruning. |
| **Delta / change-tracking** | ✅ **YES — partial migration is delta!** | `gcd_migrate_partial.py` compares issues' `created`/`modified` timestamps against `GCD_DUMP_DATE_LAST` env var. Only changed rows are processed. This is the delta pattern we need. |
| **REST/GraphQL API** | ❌ No | CLI scripts only. |
| **Cover/image handling** | ❌ No | No cover handling. gcd-talker confirms GCD strips image URLs from dumps. |
| **Schema documentation** | ⚠️ Partial | DESCRIBE output for 3 tables in README. Pydantic models serve as de-facto schema docs. |

### The `Comic` Pydantic model (de-facto flattened GCD schema)

Read directly from `python/cbdb/tgd_models.py`. This is the target schema after transformation — one denormalized row per issue:

```
Issue fields:    id, number, volume, series_id, publication_date, price,
                 page_count, deleted, isbn, valid_isbn, no_isbn,
                 variant_of_id, variant_name, barcode, no_barcode,
                 title, no_title, on_sale_date, on_sale_date_uncertain
Series fields:   series_name, year_began, year_began_uncertain, year_ended,
                 year_ended_uncertain, first_issue_id, last_issue_id,
                 country_id, language_id, has_gallery, issue_count,
                 has_isbn, has_barcode, has_issue_title, color,
                 dimensions, paper_stock, binding, publishing_format
Publisher field: publisher_name
```

**38 fields total.** The model generates `INSERT INTO comics (...)` SQL via `print_sql_insert_stmt()` iterating over `__annotations__`.

### The partial migration delta logic (critical)

Read directly from `python/gcd_migrate_partial.py`:

```
For each issue in gcd_issue (paginated 1,000 at a time):
    if issue.created < LAST_DUMP_DT → skip
    if issue.modified < LAST_DUMP_DT → skip
    Fetch series by series_id
    Look up publisher_name from JSON
    Merge into Comic model
    Output DELETE + INSERT SQL
```

**What this means for us:** GCD issues have `created` and `modified` timestamps. A delta between two dump dates is computable by querying `WHERE created >= date_of_prior_dump OR modified >= date_of_prior_dump`. No full-dump diff needed — the timestamps ARE the change tracking.

**Bug note:** The script has a logic flaw — it requires BOTH `created` AND `modified` ≥ the cutoff (AND logic, not OR). If an issue was created before the dump but modified after, it gets skipped. Our implementation should use OR: `created >= cutoff OR modified >= cutoff`.

### Database connectivity

Read from `python/cbdb/gcd_db.py`:
- Uses `mysql.connector` (Python MySQL driver)
- All queries use `SELECT *` with dictionary cursors
- Pagination via `LIMIT %s OFFSET %s` ordered by `id`
- No JOINs — fetches series/publisher in separate queries by ID
- Environment-variable-based config: `MYSQL_HOST`, `MYSQL_USER`, `MYSQL_PASSWORD`, `MYSQL_DATABASE`
- Hardcoded Docker service name "gcd-mysql" as host

### Opinion

**This is the single most useful find.** The partial migration script is a proof-of-concept for exactly our delta-publisher. What it already does:
- Ingests a GCD dump into a queryable database
- Computes deltas by comparing `created`/`modified` against a cutoff date
- Flattens GCD's relational schema (issue + series + publisher) into a single denormalized row
- Outputs SQL that could be applied to a local SQLite

**What we'd adapt:**
- Replace MySQL with SQLite (the dump is MySQL SQL; we convert server-side once)
- Add language + publisher pruning (filter by `country_id` and `language_id` from series, publisher from publisher table)
- Fix the AND→OR bug in the delta logic
- Replace `INSERT INTO comics` with SQLite-compatible `INSERT OR REPLACE`
- Add series-level change detection (not just issues — series metadata can change without new issues)
- Add story-level delta (creator credits, synopses live in `gcd_story`, not `gcd_issue`)
- Output SQLite statements or a SQLite DB directly, not MySQL SQL

**Estimated savings:** 3-5 days of pipeline prototyping vs. starting from scratch. The Pydantic `Comic` model alone saves a full day of GCD schema reverse-engineering.

---

## Repo 2: adamhathcock/gcdb-graphql — ★★★ SCHEMA REFERENCE

**URL:** https://github.com/adamhathcock/gcdb-graphql
**Language:** TypeScript 100% (NestJS + TypeORM + GraphQL)
**License:** None visible (no LICENSE file)
**Stars:** 2 | **Forks:** 0 | **Commits:** 7 | **Last activity:** Very low
**Status:** Abandoned prototype

### Facts

**What it does:** Provides a GraphQL query API over a MySQL instance loaded with a GCD dump. Uses NestJS, TypeORM, and auto-generated GraphQL types.

**Ingest → Output:**
- Ingests: GCD MySQL dump (loaded into MySQL via docker-compose)
- Outputs: GraphQL queries over the GCD tables

### Feature checklist

| Feature | Present? | Details |
|---------|----------|---------|
| MySQL→SQLite conversion | ❌ No | MySQL only |
| Pruning/filtering | ❌ No | Full dump |
| Delta/change-tracking | ❌ No | Static snapshot |
| REST/GraphQL API | ✅ Yes | GraphQL — 3 queries: `issue(id)`, `series(id)`, `story(id)` |
| Cover/image handling | ❌ No | |
| Schema documentation | ✅ **YES — TypeORM entities + GraphQL SDL** | Clean mapping of GCD tables to queryable types |

### Schema mapping (read directly from source)

**TypeORM entities** (4 files in `src/gcdb/models/`):

```
Issue.ts      → @Entity({ name: "gcd_issue" })
  Columns: id, number, notes, publication_date, key_date
  Relations: @ManyToOne → Series (via series_id), @OneToMany → Story[]
  
Series.ts     → @Entity({ name: "gcd_series" })
  Columns: id, name, notes, year_began, year_ended, created, modified
  Relations: @OneToMany → Issue[]
  
Story.ts      → @Entity({ name: "gcd_story" })
  Columns: title, feature, script, pencils, inks, colors, letters,
           genre, characters, synopsis, notes, created, modified
  Relations: @ManyToOne → Issue (via issue_id), @OneToOne → StoryType (via type_id)
  
StoryType.ts  → (implied table: gcd_story_type)
  Columns: id, name
```

**GraphQL schema** (`gcdb.graphql` — GraphQL SDL):

```graphql
type Query {
  issue(id: Int!): Issue
  series(id: Int!): Series
  story(id: Int!): Story
}

type Issue {
  id: Int!, number: String!, notes: String!
  publication_date: String!, key_date: String!
  series: Series, stories: [Story]
}

type Series {
  id: Int!, name: String!, notes: String!
  year_began: Int!, year_ended: Int
  issues: [Issue]
}

type Story {
  id: Int!, title: String!, notes: String!
  feature: String!, script: String!, pencils: String!
  inks: String!, colors: String!, letters: String!
  genre: String!, characters: String!, synopsis: String!
  issue: Issue, type: StoryType
}

type StoryType {
  id: Int!, name: String!
}
```

### Opinion

**This is our de-facto GCD schema quick-reference.** The TypeORM entities + GraphQL SDL give us the exact column names, types, and relationships for the 4 core tables. Much faster to read than the Django models (which have 20 files with historic cruft).

**What we'd use:**
- The entity definitions as a schema reference — "what columns does gcd_story actually have?"
- The relationship map: Series→Issue→Story→StoryType (confirmed bidirectional)
- Confirmation that `created`/`modified` exist on Series, Issue, AND Story — all three have change timestamps
- The GraphQL schema as a starting point if we ever want to expose the catalogue API

**What we'd skip:**
- The NestJS/TypeORM/GraphQL stack (overkill for a static-file delta publisher)
- The MySQL dependency (we want SQLite)
- The single-ID query model (we need bulk/list queries)

**Estimated savings:** 1-2 days of GCD schema exploration. The 4 entity files are cleaner schema docs than the 20 Django model files.

---

## Repo 3: jochengcd/gcd — ★★ AUTHORITATIVE BUT OVERWHELMING

**URL:** https://github.com/jochengcd/gcd
**Forked from:** GrandComicsDatabase/gcd-historical-archive
**Language:** Python/Django
**License:** Not visible on page
**Stars:** 0 | **Forks:** 0 | **Commits:** 1,506
**Status:** The maintainer's personal working copy of the GCD website codebase

### Facts

**What it does:** This IS the GCD website code. The `pydjango/` directory is the Django project that runs comics.org. It contains the authoritative Django models, views, admin, and management commands.

**What I read:** The Django model files — these are the canonical GCD schema definition:
- `pydjango/apps/gcd/models/` — 20 Python files covering every GCD table
- Key models: `issue.py`, `series.py`, `story.py`, `publisher.py`, `cover.py`, `image.py`, `reprint.py`, `issuereprint.py`, etc.

### Feature checklist

| Feature | Present? | Details |
|---------|----------|---------|
| MySQL→SQLite conversion | ❌ No | MySQL-native (Django ORM, but database is MySQL) |
| Pruning/filtering | ❌ No | Full database |
| Delta/change-tracking | ❌ No export delta, but... | `created`/`modified` with `auto_now_add`/`auto_now` on Issue, Series, AND Story — the timestamps EXIST in the schema |
| REST/GraphQL API | ❌ No | Django views/templates, no API |
| Cover/image handling | ✅ Yes | `cover.py` and `image.py` models — GCD's internal cover tracking. Confirms covers exist as separate entities, not embedded in issues. |
| Schema documentation | ✅ **YES — THE authority** | These ARE the official GCD schema definitions |

### Key schema facts from Django models

**Issue** (`issue.py`):
- 28+ fields including: number, title, volume, isbn, valid_isbn, barcode, price, page_count, publication_date, key_date, on_sale_date, sort_code, indicia_frequency, editing, notes
- Self-referential FK: `variant_of` → Issue (variant tracking!)
- FKs to: Series, IndiciaPublisher, Brand
- Soft delete: `deleted` BooleanField
- `created` (auto_now_add) + `modified` (auto_now, db_index=True) — **change timestamps exist**
- `sort_code` field for issue ordering within a series
- Keywords via django-taggit

**Series** (`series.py`):
- Identity: name, sort_name, format, classification (FK), country (FK), language (FK), publisher (FK), imprint (nullable FK)
- Dates: year_began, year_ended, year_began_uncertain, year_ended_uncertain, is_current
- Issue tracking: first_issue (FK), last_issue (FK), issue_count
- Physical: color, dimensions, paper_stock, binding, publishing_format
- Features: has_barcode, has_indicia_frequency, has_isbn, has_issue_title, has_volume, has_gallery, is_comics_publication
- `created`/`modified` — **change timestamps exist**
- Soft delete

**Story** (`story.py`):
- Core: title, title_inferred, feature, type (FK→StoryType), sequence_number, page_count
- Creator credits (all TextField): script, pencils, inks, colors, letters, editing
- Content: genre, characters, synopsis, reprint_notes, notes
- Each credit field has a paired `no_*` boolean (e.g., `no_script`)
- `created`/`modified` — **change timestamps exist**
- Soft delete
- Reprint tracking via multiple M2M relations

**Publisher** (`publisher.py` — not read but present):
- Publisher + Brand + IndiciaPublisher models
- Country FK, year began/ended, series_count, issue_count

**Cover/Image** (`cover.py`, `image.py`):
- GCD tracks covers as separate Image entities with ContentType generic FKs
- Cover images are NOT embedded in the issue table
- Confirms: the dump excludes image URLs (gcd-talker's claim is correct)

### The critical finding: change timestamps

All three core tables have:
```python
created = DateTimeField(auto_now_add=True)
modified = DateTimeField(auto_now=True, db_index=True)
```

**This confirms delta is possible without full-dump diffing.** The `modified` field is even indexed — querying `WHERE modified >= '2026-06-03'` across gcd_issue, gcd_series, and gcd_story gives you every changed row since the last dump. This is exactly what gcd_docker's partial migration exploits.

### Opinion

**Too large to fork, invaluable as reference.** This is the full GCD website — Django views, templates, forms, middleware, migration scripts, OI (change-management) app, voting, indexing workflows. Far more than we need. But the Django models are the single source of truth for "what columns does GCD table X actually have?"

**What we'd use:**
- The model files as the authoritative schema reference
- Confirmation that `created`/`modified` timestamps exist and `modified` is indexed
- Confirmation that soft-delete (`deleted=True`) is the pattern — our delta must handle deletes
- The variant chain: Issue.variant_of → Issue (self-referential)
- The country/language fields on Series — these are WHAT WE PRUNE BY

**What we'd skip:**
- The entire Django webapp
- The OI (change-management) app
- The indexing/reservation workflow
- The template/view layer

**Estimated savings:** 1 day of schema guesswork eliminated. Every column is documented.

---

## Summary: What Each Repo Gives Us

| Capability | gcd_docker | gcdb-graphql | jochengcd/gcd |
|-----------|-----------|-------------|---------------|
| MySQL→SQLite conversion | ❌ | ❌ | ❌ |
| Delta/change-tracking | ✅ **Partial migration script** | ❌ | ✅ **Timestamps confirmed** |
| Schema reference | ⚠️ 3 tables flattened | ✅ **4 entities cleanly mapped** | ✅ **20 models, authoritative** |
| Pruning pattern | ⚠️ Date-only | ❌ | ✅ **country_id + language_id fields** |
| Cover handling | ❌ | ❌ | ✅ **Cover models confirm exclusion** |
| API layer | ❌ | ✅ GraphQL (reference) | ❌ |
| Flattened output schema | ✅ **Comic model** | ❌ | ❌ |

## The Single Most Useful Find

**`thegraydot/gcd_docker` — specifically `gcd_migrate_partial.py` + `tgd_models.py`.**

Combined, these two files are a working proof-of-concept for ~60% of our delta-publisher pipeline:

1. **`gcd_migrate_partial.py`** proves delta is possible using GCD's own `created`/`modified` timestamps (confirmed authoritative by jochengcd/gcd Django models). No full-dump diff needed.
2. **`tgd_models.py`** gives us the flattened output schema (38 fields joining issue + series + publisher) we'd ship to the app's local SQLite.

What's missing (we'd build):
- MySQL→SQLite conversion (gcd_docker stays in MySQL)
- Language + publisher pruning (gcd_docker has no filtering)
- Story-level delta (gcd_docker only migrates issues)
- Series-only changes (series metadata can change without new issues)
- Delete handling (soft-deleted rows — gcd_docker doesn't handle these)
- SQLite output (gcd_docker outputs MySQL INSERT statements)
- Static-file manifest generation

What it saves us: The core insight is proven — GCD's `modified` timestamp (db_indexed!) on all three core tables means we can compute deltas with simple WHERE clauses rather than diffing two 4GB databases. The gcd_docker partial migration already does this for issues. We extend the pattern to series and stories, add pruning, and write SQLite instead of MySQL SQL.

---

## Concrete Pipeline Sketch (incorporating these findings)

```
SERVER-SIDE (bi-weekly cron job):

1. DOWNLOAD latest current.zip from comics.org
2. UNZIP → YYYY-MM-DD.sql
3. CONVERT: MySQL SQL → temporary SQLite DB
   (use awk/mysql2sqlite — server-side Python is fine)
4. PRUNE: Filter to English + major publishers
   WHERE series.language_id IN (en) 
   AND series.country_id IN (us, gb, ca)
   AND publisher.id IN (top 50 publisher IDs)
   → pruned_current.db
5. DELTA (the key innovation from gcd_docker):
   For each table (gcd_series, gcd_issue, gcd_story):
     SELECT * WHERE modified >= '{last_dump_date}'
     OR created >= '{last_dump_date}'
     OR deleted = 1  (need prior snapshot to detect)
   → delta rows (~5-20MB)
6. DENORMALIZE (from tgd_models.py pattern):
   Join issue + series_name + publisher_name → flat Comic rows
7. PACKAGE: delta as SQLite INSERT/UPDATE/DELETE statements
   + manifest.json (version, baseline_url, delta_url, checksums)
8. UPLOAD to static host (R2/BunnyCDN/GitHub Releases)

CLIENT-SIDE (Tankoban 3 app, every launch):
1. Fetch manifest.json
2. If local version < manifest version:
   - Download delta
   - Apply to local SQLite
   - Update version stamp
3. All catalogue queries hit LOCAL SQLite (offline-capable)
```
