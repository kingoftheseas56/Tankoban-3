#pragma once
// Phase 2 (throwaway) link smoke for the native torrent streaming engine.
// Proves libtorrent vendors + links + a session constructs in-process inside
// Tankoban3.exe. Deliberately NO libtorrent includes here, so main.cpp can call
// this without pulling libtorrent headers into the whole app.
namespace tankoban::tstream {

// Constructs + destroys a minimal libtorrent session, logging to stderr.
// No-op stub (logs "engine disabled") when HAS_LIBTORRENT is undefined.
void ltLinkSmoke();

} // namespace tankoban::tstream
