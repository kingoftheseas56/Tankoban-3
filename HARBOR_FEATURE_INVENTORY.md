# Harbor Feature Inventory for Tankoban 3

Inventory date: 2026-06-16

Purpose: make Harbor's visible product surface explicit before further Tankoban 3 scope decisions. This document is a source-backed inventory, not an implementation plan. Status labels reflect the current Tankoban 3 discussion and should be changed only by explicit product decision.

Status legend:

- `Keep`: in Tankoban 3 scope.
- `Cut`: out of Tankoban 3 scope.
- `Defer`: not first milestone, but not cut.
- `Needs decision`: visible Harbor feature not yet clearly accepted or rejected.

## Current Locked Decisions

Keep:

- Core Harbor video spine: Home/rows -> Detail -> Play Picker -> Player.
- Home hero carousel.
- Stremio addon protocol: manifest, catalog, meta, stream, subtitles.
- Addons app store/community marketplace.
- Advanced stream parsing/scoring.
- Local files.
- Watchlist, history, continue-watching.
- Native Qt6 desktop implementation with Harbor's visual language.

Cut:

- Live TV/IPTV.
- Playlists and playlist VOD.
- Multiview.
- Calendar.
- Award browsers, Browse by Award, award pages, award tiles.
- Watch Together.
- Multi-profile/profile picker.
- Parental controls.
- Trakt, AniList, Simkl.
- Discord/webhooks.
- Theme presets/theme studio.
- Relay/account/cloud sync.
- Featured & Recommended / thumbs-based recommendation tuning for now.
- TMDB/API-key metadata paths, TMDB rows, TMDB key nudges, TMDB watch-provider links, and TMDB people search.

Current cuts introduced during roadmap discussion, still worth confirming later:

- People search.
- Home row customization.
- Critics Pick.
- Discovery Queue.

## App Shell and Navigation

Sources:

- `src/App.tsx`
- `src/lib/view.tsx`
- `src/chrome/sidebar.tsx`
- `src/chrome/topbar.tsx`
- `src/chrome/floating-back.tsx`
- `src/chrome/stremio-rail.tsx`
- `src/chrome/topdock.tsx`
- `src/chrome/minui-dock/*`

Inventory:

| Feature | Harbor behavior | Tankoban 3 status |
|---|---|---|
| Frame stack navigation | `home/settings/anime/discover/addons/... -> meta -> picker -> player` push stack | Keep |
| Sidebar nav | Home, Discover, Movies, Shows, Anime, Library, Downloads, Addons, Settings, plus cut routes in Harbor | Keep trimmed nav |
| Alternate chrome layouts | sidebar, Dracula, Nord, Forest, Stremio rail, top dock, royal topbar, side rail, minui, custom | Cut for v1 via theme preset cut |
| Floating back | Back affordance for pushed routes | Keep if needed |
| Topbar search | Global search opener and top overlay | Keep |
| Offline banner/update/custom code/memory HUD | Global utility overlays | Cut or Defer; needs explicit decision per utility |
| Context menu and hover preview | Global context/preview layer | Hover preview Needs decision; context menu Defer |

## Home

Sources:

- `src/views/home.tsx`
- `src/views/home/home-rows.ts`
- `src/components/hero-carousel.tsx`
- `src/views/home/cw-section.tsx`
- `src/components/streaming-rail.tsx`
- `src/components/collections-row.tsx`
- `src/views/home/customizable-rows.tsx`
- `src/views/home/customize-bar.tsx`

Inventory:

| Feature | Harbor behavior | Tankoban 3 status |
|---|---|---|
| Hero carousel | Required first-viewport feature; opens metas | Keep |
| Continue Watching | Rows from Stremio/history/Simkl; advance-next logic | Keep local/history version |
| Streaming provider rail | Netflix/Disney/etc provider shortcuts | Needs decision |
| Top 10 row | Rank-card presentation from first row | Keep if data available |
| Cinemeta rows | Fallback rows and genre rows | Keep |
| TMDB rows | Richer rows if TMDB key exists | Cut |
| Addon catalog rows | Installed addon catalog rows merged into Home | Keep |
| Anime rows on Home | Anime rows injected unless hidden | Needs decision |
| Arabic home rows | Arabic-localized rows | Cut unless Arabic support is restored |
| Trakt/Simkl rows | External account rows | Cut |
| Collections row | Franchise collections shortcut | Needs decision |
| Home row customization | reorder/hide/rename/numerals/hero source | Current cut |
| TMDB nudge | Prompt for TMDB key | Cut |

## Discover

Sources:

- `src/views/discover.tsx`
- `src/components/featured-banner.tsx`
- `src/components/featured-banner/*`
- `src/lib/feed/*`
- `src/lib/discover/*`
- `src/components/discovery-queue-cta.tsx`
- `src/components/critics-pick.tsx`
- `src/components/award-tiles.tsx`
- `src/components/genre-tiles.tsx`
- `src/components/language-tiles.tsx`

Inventory:

| Feature | Harbor behavior | Tankoban 3 status |
|---|---|---|
| Featured & Recommended | Large recommendation carousel with side panel | Cut for now |
| Thumbs tuning | Up/down votes tune recommendations | Cut for now |
| Taste/affinity tracking | Local event store tracks opens, votes, play/watchlist/watched | Cut for now |
| Daily rows | Selected feed rails based on settings/taste/provider data | Needs decision; only keep if backed by Cinemeta/addons, not TMDB |
| Genre tiles | Quick genre entry points | Needs decision |
| Language tiles | Browse by original language | Needs decision |
| Discovery Queue | Recommendation queue CTA | Current cut |
| Critics Pick | Editorial critic feature | Current cut |
| Award tiles | Browse by awards | Cut |
| Back-to-top and scroll memory | Navigation quality-of-life | Keep |

## Movies

Sources:

- `src/views/movies.tsx`
- `src/components/cinema-hero.tsx`
- `src/components/top-rank-card.tsx`
- `src/lib/feed/moods.ts`
- `src/lib/feed/sections.ts`

Inventory:

| Feature | Harbor behavior | Tankoban 3 status |
|---|---|---|
| Cinema hero | Featured movie hero | Keep or Defer after Home hero |
| Top 10 Movies Today | ranked cards | Keep if data available |
| Trending/In theaters/Popular rows | Cinemeta/addon-backed rows | Keep if data is available without TMDB |
| Mood rows | time/day-based row specs | Needs decision |
| Critics/acclaim/hidden gems/decade/language/doc rows | TMDB discovery rows | Cut |
| View all grid | Row opens paged grid view | Keep |

## Shows

Sources:

- `src/views/shows.tsx`
- `src/views/shows/hero-curation.ts`
- `src/components/peek-hero.tsx`
- `src/components/continue-card.tsx`

Inventory:

| Feature | Harbor behavior | Tankoban 3 status |
|---|---|---|
| Shows masthead | Page heading/subtitle | Keep |
| Peek hero | Featured series hero | Keep or Defer after Home hero |
| Pick up where left off | Series continue-watching row | Keep local/history version |
| Top 10 series | ranked row | Keep if data available |
| Network rows | HBO/Netflix/Apple/AMC/etc | Cut if TMDB-dependent |
| Genre rows | comedy/crime/scifi/doc/etc | Keep with Cinemeta first |
| View all grid | row opens paged grid | Keep |

## Anime

Sources:

- `src/views/anime.tsx`
- `src/views/anime/anime-rows.tsx`
- `src/components/anime-hero.tsx`
- `src/components/anime-genre-picker.tsx`
- `src/views/anime/anilist-*`
- `src/lib/use-anime-top-picks.ts`
- `src/lib/providers/jikan.ts`
- `src/lib/providers/kitsu.ts`

Inventory:

| Feature | Harbor behavior | Tankoban 3 status |
|---|---|---|
| Anime hero | Hero with top picks | Keep hero with non-TMDB art; cut top-picks panel |
| Anime top picks / tune picks | Local favorite genres and picks | Cut for v1 with recommendations |
| Jikan/Kitsu rows | Airing, new, popular, upcoming, etc | Keep |
| Addon anime rows | Anime rows from installed addon catalogs | Keep |
| Continue watching anime | Anime CW row | Keep if local/history supports it |
| AniList rows/top100 | External account integration | Cut |
| Anime awards rows | awards | Cut |
| Adult anime filtering | hide adult content | Keep |

## Detail Page

Sources:

- `src/views/detail.tsx`
- `src/views/detail/*`
- `src/lib/providers/tmdb/tmdb-details.ts`
- `src/lib/providers/cinemeta-details.ts`
- `src/lib/watchlist.ts`
- `src/lib/media-favorites.tsx`
- `src/lib/resume.ts`

Inventory:

| Feature | Harbor behavior | Tankoban 3 status |
|---|---|---|
| Backdrop hero | full hero, gradients, title/logo, metadata pills | Keep |
| Play / smart play | movie or resume/first episode opens picker | Keep |
| Watchlist | add/remove saved title | Keep |
| Favorites | separate favorite star | Needs decision |
| Trailer button/overlay | YouTube/native trailer overlay | Defer/Cut from v1 |
| Download action | episode/movie source download | Defer; separate owner plan |
| Ratings beyond basic IMDb/Cinemeta | RT/MDBList/RPDB/MAL and other extra badges | Defer/Cut from v1 |
| Watch providers / Watch On | TMDB provider links | Cut |
| Streaming links | anime streamers | Defer unless Detail info-block scope keeps them |
| Episodes list/strip | series episodes, season picker, watched menu | Keep core Stremio videos; richer watched menu Defer |
| Anime franchise/episodes | Kitsu/Jikan/anime mappings | Keep lean episode/id mapping; defer franchise graph UI |
| Credits | directors, writers, producers, etc | Needs decision |
| Cast row | cast cards opening Person | Needs decision |
| Collection row | franchise collection | Needs decision |
| More Like This / You Might Also Like | recommendations/similar rows | Cut for now with recommendations |
| Media gallery | stills/backdrops/videos | Needs decision |
| Awards blocks/corners | award UI | Cut |
| Info block | metadata details | Keep lean version |
| Upcoming CTA | special handling for unreleased titles | Needs decision |

## Play Picker

Sources:

- `src/views/play-picker.tsx`
- `src/views/play-picker/*`
- `src/lib/streams/*`
- `harbor-core/src/*`

Inventory:

| Feature | Harbor behavior | Tankoban 3 status |
|---|---|---|
| Full-screen picker | shell hidden, backdrop, back button | Keep |
| Stream ID builder | generates movie/episode/addon ids | Keep |
| Addon stream queries | installed addons queried by manifest support | Keep |
| Advanced parser/scoring/trust | parse source/resolution/audio/codec/language/size; rank/trust | Keep |
| Quality tiers | 4K/DV/HDR/1080/720/SD/Rough | Keep |
| Stremio layout | list-style picker | Keep |
| Condensed layout/primary card/source drawer | alternate picker UI | Needs decision |
| Source diagnostics | details on raw/filtered streams | Needs decision |
| Language filter | preferred language filtering | Keep or Defer |
| Cached-only filter/debrid state | debrid-specific | Defer until debrid decision |
| Auto-play/instant play | automatic best source attempt | Needs decision; depends on scorer |
| P2P confirm | warn before P2P/torrent | Defer until torrent engine |
| Debrid down/no sources modals | debrid-specific states | Defer |
| Download intent | choose source for downloads | Defer |

## Player

Sources:

- `src/views/player.tsx`
- `src/views/player/*`
- `src/components/player/*`
- `src/lib/player/*`
- `src/lib/player-shells/*`
- `src/lib/skip-intro/*`
- `src/lib/trickplay.ts`

Inventory:

| Feature | Harbor behavior | Tankoban 3 status |
|---|---|---|
| mpv/html5 player bridge | playback engine abstraction | Keep via Agent 4 libmpv |
| Transport controls | play/pause, seek, volume, time, fullscreen | Keep |
| Audio/subtitle menus | track selection, delay, add subtitle | Keep basic; advanced menu Defer |
| Subtitle styling | font, border, opacity, margin, ASS overrides | Needs decision |
| Stream switcher | switch source in player | Keep or Defer after picker |
| Episode nav | prev/next episode | Keep |
| Resume prompt/autosave | continue watching | Keep |
| Auto next episode | prompt/auto advance | Needs decision |
| Skip intro/credits | Aniskip/TheIntroDB/chapters | Needs decision |
| Thumbnail preview/trickplay | seek preview images | Needs decision |
| PiP/fullscreen | playback modes | Keep if Qt/libmpv supports |
| Cast/AirPlay/Chromecast | casting | Defer/Cut from v1 |
| Draw mode | drawing overlay | Cut with Watch Together unless wanted solo |
| Watch Together room/chat/avatars | collaborative viewing | Cut |
| Frame grab/GIF recorder | tools | Defer/Cut from v1 |
| AB loop/sleep timer | tools | Defer/Cut from v1 |
| Stats/engine diagnostics | playback diagnostics | Needs decision |
| Downloads from player | save current stream | Defer |
| Live overlays/DVR | IPTV | Cut |

## Addons

Sources:

- `src/views/addons.tsx`
- `src/views/addons/*`
- `src/lib/addon-store.ts`
- `src/lib/addons.ts`
- `src/lib/addons-store/*`
- `src/lib/providers/stremio-addons.ts`
- `src/lib/providers/stremio-addons-index.ts`

Inventory:

| Feature | Harbor behavior | Tankoban 3 status |
|---|---|---|
| Addons app store | Discover/Browse/Installed app-store shape | Keep |
| Curated addons | curated entries and hero/rails | Keep |
| Community addon index | public addon listing | Keep |
| stremio-addons.net provider | top/new/rising/category data | Keep |
| Addon search | search marketplace | Keep |
| Category filters | streams/catalogs/subtitles/anime/etc | Keep |
| Adult addon gate | adult toggle + age gate | Needs decision |
| Install by URL | paste manifest/stremio link | Keep |
| Install modal/config warning | install/manage/replace | Keep |
| Installed pane | manage installed addons | Keep |
| Reorder/organize/backups | addon order and backup UI | Needs decision |
| Addon detail | description, docs, related/recommended addons | Keep detail; related/recommended Needs decision |
| Deep link install | stremio:// install handling | Needs decision |
| Account/Stremio sync | sync addon collection to Stremio account | Cut with account sync |

## Search

Sources:

- `src/components/search/search-overlay.tsx`
- `src/components/search/*`
- `src/lib/search.ts`
- `src/lib/search-context.tsx`
- `src/lib/search-addons.ts`
- `src/lib/search-addon-index.ts`

Inventory:

| Feature | Harbor behavior | Tankoban 3 status |
|---|---|---|
| Global overlay | Ctrl/Cmd+K, modal search | Keep |
| Top match | prominent movie/show match | Keep |
| Movies/series results | Cinemeta/addon merged | Keep |
| Addon hits/addon groups | addon marketplace/catalog search results | Keep |
| Anime results | Jikan search | Keep |
| Live TV results | IPTV search | Cut |
| People results | TMDB people search | Cut |
| Genre/year intent | query opens filter page | Needs decision |
| Magnet card | magnet input handling | Needs decision; depends torrent engine |
| Recent searches | local recent list | Needs decision |

## Library

Sources:

- `src/views/library.tsx`
- `src/views/library/*`
- `src/lib/watchlist.ts`
- `src/lib/local-watchlist.tsx`
- `src/lib/playback-history.ts`
- `src/lib/local-library.ts`

Inventory:

| Feature | Harbor behavior | Tankoban 3 status |
|---|---|---|
| Watchlist tab | saved items | Keep |
| History tab | playback history | Keep |
| Local files tab | scan folder, parse filenames, play local path | Keep |
| Trakt tab | Trakt library | Cut |
| AniList tab | AniList library | Cut |
| Simkl tab | Simkl library | Cut |
| Manual watched menu | mark episodes watched/unwatched | Needs decision |
| Local watchlist/favorites storage | local storage/provider | Keep as SQLite/QSettings equivalent |

## Downloads

Sources:

- `src/views/downloads.tsx`
- `src/views/downloads/download-dir-bar.tsx`
- `src/lib/download/*`
- `src/views/detail/episode-download-button.tsx`
- `src/views/play-picker/download-started.tsx`

Inventory:

| Feature | Harbor behavior | Tankoban 3 status |
|---|---|---|
| Downloads page | saved/offline items list | Defer; separate owner plan |
| Download directory setting | choose/reveal directory | Defer; separate owner plan |
| Source-to-file download | choose stream then save | Defer; separate owner plan |
| Play downloaded file | opens local path in player | Defer; separate owner plan |
| Delete/cancel/reveal | download management | Defer; separate owner plan |

## Settings

Sources:

- `src/views/settings.tsx`
- `src/views/settings/nav.tsx`
- `src/views/settings/*`
- `src/lib/settings/types.ts`
- `src/lib/settings/defaults.ts`

Inventory:

| Feature | Harbor behavior | Tankoban 3 status |
|---|---|---|
| Settings nav + search | section nav and option search | Keep lean |
| Account | Stremio account/sign-in | Cut |
| Library & metadata keys | OMDb/RPDB/Fanart/TVDB, region, badges, spoilers, content filters | Needs decision by item; TMDB cut; TVDB/Fanart key enrichments not part of Anime batch |
| Streaming sources | debrid keys, addon/source settings, picker layout/sort, stream safety | Keep stream safety/sort; debrid Needs decision |
| Language | preferred audio/subtitle languages | Keep |
| Player & quality | engine, HDR, Anime4K, seek bar, instant play, downloads, local engine, remote server, custom code | Keep core player; many subitems Need decision |
| Player layout | rearrange player controls | Cut with theme/player-layout presets unless wanted |
| Hotkeys | keyboard shortcut editor | Needs decision |
| Theme & appearance | presets, backgrounds, fonts, custom themes/studio | Cut per theme preset cut |
| Webhooks | Discord/Telegram notifications | Cut |
| Bug report | bug report panel | Cut or Defer |
| Advanced | updates, backup/restore, tray, Discord presence, privacy blocklist | Needs decision by item; Discord cut |

## Row/Grid/Filter/Collections/Person

Sources:

- `src/views/grid.tsx`
- `src/views/filter.tsx`
- `src/views/filter/*`
- `src/views/collections.tsx`
- `src/views/collection.tsx`
- `src/views/person.tsx`
- `src/lib/collections-catalog.ts`
- `src/lib/providers/tmdb/tmdb-people.ts`

Inventory:

| Feature | Harbor behavior | Tankoban 3 status |
|---|---|---|
| Grid view | paged "View all" grid | Keep |
| Filter pages | genre/year/runtime/studio/country/language/network pages | Needs decision |
| Collections index | franchise collections browse/search | Needs decision |
| Collection detail | individual franchise row/page | Needs decision |
| Person detail | actor/crew profile, filmography, awards | Current cut if people search remains cut; Needs decision |
| Awards in person/collections | awards laurel strips | Cut |

## Lists

Sources:

- `src/views/lists.tsx`
- `src/views/lists/*`
- `src/lib/lists/*`
- `src/lib/media-list-store.tsx`

Inventory:

| Feature | Harbor behavior | Tankoban 3 status |
|---|---|---|
| Custom lists | add/manage imported lists | Needs decision |
| External list sources | IMDb, Letterboxd, MAL, MDBList, Trakt | Needs decision; TMDB cut; Trakt source cut if account cut |
| List picker | add item to custom list | Needs decision |

## Cut Route Clusters

Sources:

- `src/views/live.tsx`, `src/views/live/*`, `src/lib/iptv/*`
- `src/views/playlist-vod.tsx`, `src/views/playlist-vod/*`
- `src/views/multiview.tsx`, `src/views/multiview/*`
- `src/views/calendar.tsx`, `src/views/calendar/*`, `src/lib/calendar*`
- `src/views/award.tsx`, `src/views/anime-award.tsx`
- `src/lib/together/*`, `src/components/together-*`
- `src/lib/profiles.tsx`, `src/components/profile-picker/*`
- `src/lib/parental.tsx`
- `src/lib/trakt/*`, `src/lib/anilist/*`, `src/lib/simkl/*`
- `src/lib/discord/*`, `src/lib/webhook-engine.ts`

Inventory:

| Feature | Harbor behavior | Tankoban 3 status |
|---|---|---|
| Live TV/IPTV | M3U/Xtream, guide, favorites, EPG, live player overlays | Cut |
| Playlist VOD | VOD from IPTV/playlists | Cut |
| Multiview | multiple live cells | Cut |
| Calendar | releases/episodes notifications and calendar UI | Cut |
| Awards | award browsers/pages | Cut |
| Watch Together | rooms, chat, cursors, relay, host source matching | Cut |
| Profiles | profile picker/avatar/password | Cut |
| Parental controls | hidden tabs/PIN | Cut |
| Trakt/AniList/Simkl | account sync, scrobble, list rows | Cut |
| Discord/webhooks | presence and notifications | Cut |

## Backend and Cross-Cutting Systems

Sources:

- `src/lib/streams/*`
- `harbor-core/src/*`
- `src/lib/debrid/*`
- `src/lib/torrent/*`
- `src/lib/subtitles/*`
- `src/lib/providers/*`
- `src/lib/hotkeys.ts`
- `src/lib/i18n/*`
- `src/lib/backup.ts`
- `src/lib/privacy/blocklist.ts`
- `src/lib/updater/*`

Inventory:

| Feature | Harbor behavior | Tankoban 3 status |
|---|---|---|
| Stream parser | parse resolution/source/audio/codec/HDR/language/size/release group | Keep |
| Stream scoring | score/rank streams by quality/trust/preferences | Keep |
| Stream trust filtering | reject fake/mismatched/new-release stubs | Keep |
| Debrid providers | RD/TorBox/AD/PM/DebridLink, cache checks, playable URLs | Needs decision |
| Torrent/local engine | direct torrent/stremio server/local engine | Needs decision |
| Stream proxy/transcode | proxy headers/transcode paths | Needs decision |
| Subtitle providers | addons, OpenSubtitles, Wyzie, Jimaku | Keep addons/OpenSubtitles/Wyzie; Jimaku Defer |
| Metadata providers | Cinemeta, OMDb, RPDB, Fanart, TVDB, Kitsu/Jikan, MDBList, Wikidata | Cinemeta Keep; Kitsu/Jikan Keep for Anime; TMDB Cut; Fanart/TVDB key enrichments Defer; others Need decision or Cut if awards/account-dependent |
| Hotkeys | global/player hotkeys | Needs decision |
| i18n/Arabic | English/Arabic translation and RTL support | Needs decision |
| Backup/restore | export/import settings | Needs decision |
| Privacy blocklist | tracker blocking | Defer/Cut from v1 |
| Updates | updater UI | Defer/Cut from v1 |
| Deep links | addon install/open meta | Needs decision |

## Immediate Scope Questions

These require explicit product decisions before the roadmap should be considered stable:

1. Should people search and Person pages stay cut, or should they be kept for cast/crew navigation?
2. Should Discover keep genre/language tiles and daily rows while cutting recommendations/critics/awards?
3. Should Detail keep cast/crew, media gallery, collection rows, and upcoming/unreleased-title handling?
4. Downloads/offline saving is deferred to a separate owner plan; local files cover offline playback in this roadmap.
5. Should debrid and torrent/local-engine support be in the first playable milestone, or follow direct URL playback?
6. Kitsu/Jikan are kept for Anime. Which other non-TMDB metadata providers, if any, should be kept beyond Stremio/Cinemeta/addons?
7. Subtitle support now includes embedded subtitles, Stremio subtitle addons, OpenSubtitles V3, and Wyzie; Jimaku is deferred.
8. Should advanced player tools like skip intro and trickplay thumbnails be kept?
9. Should custom lists/collections/filter pages exist in Tankoban 3?
10. Should Arabic/i18n support be kept from Harbor?
