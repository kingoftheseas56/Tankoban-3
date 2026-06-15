# Tankoban 3

A native **C++ / Qt6** desktop media app — a faithful, *lean* recreation of
**Harbor**'s video mode, built **incrementally**, then customized into our own.

## Why
- **Isolated from Tankoban 2** (which stays a clean archive + last-resort engine
  donor). Ownership comes from adapting Harbor (React/Tailwind) into native Qt —
  like adapting a novel into a TV show: it becomes its own thing and reaches places
  the original never could.
- **Harbor's code is the spec.** Recreate it faithfully; trim only genuinely
  pointless niche extras. Later, expand the addon store with our own addons (the
  Tankorent index, a DDL / HTTPS-streaming addon).

## The spine
`Home → Detail → Play-Picker → Player (libmpv)`, fed by an **addon store** — the
Stremio addon protocol (catalog / meta / stream / subtitles). Install Cinemeta +
a stream addon and the catalogs fill.

## Build
Requires: Qt6 (`C:\tools\qt6sdk\6.10.2\msvc2022_64`), MSVC 2022 BuildTools, CMake, Ninja.

    build.bat        # configure + build (Release) + windeployqt -> out\Tankoban3.exe

## Status (incremental)
- [x] **Step 1** — the foundation window (toolchain + empty dark window).
- [ ] Step 2 — the app shell (Harbor sidebar + window chrome).
- [ ] Step 3 — the addon engine slice (Cinemeta catalog -> home rows render).
- [ ] Step 4+ — detail -> play-picker -> libmpv player (end-to-end play).
