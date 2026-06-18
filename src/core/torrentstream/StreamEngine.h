#pragma once
// Tankoban 3 — native torrent streaming engine (in-process).
//
// Reproduces the Stremio :11470 streaming contract over libtorrent:
//   - resolve a torrent stream (infoHash + fileIdx + trackers) to a local HTTP URL
//   - serve GET /{hash}/{idx} with HTTP Range -> 206, deadline-scheduled head pieces
//   - POST /{hash}/create -> file list + guessedFileIdx
//
// Threading: the libtorrent alert pump and the HTTP server each run on their own
// (non-GUI) threads; per-request piece-waits block only a worker thread via a
// condition variable woken by the pump. The GUI thread NEVER blocks on pieces —
// it only calls streamUrl()/start()/stop(), which return immediately.
//
// PIMPL on purpose: this header pulls in NO libtorrent or winsock headers, so the
// rest of the Qt app can include it freely.

#include <memory>
#include <string>
#include <vector>

namespace tankoban::tstream {

class StreamEngine {
public:
    // savePath = directory libtorrent writes pieces into (the stream cache).
    explicit StreamEngine(std::string savePath);
    ~StreamEngine();

    StreamEngine(const StreamEngine&) = delete;
    StreamEngine& operator=(const StreamEngine&) = delete;

    // Start the libtorrent session + alert pump + local HTTP server.
    // Returns true on success (server bound to a loopback port).
    bool start();

    int         port() const;     // bound loopback port, or -1 if not started
    std::string baseUrl() const;  // "http://127.0.0.1:<port>", or "" if not started

    // Resolve a torrent stream to a playable local HTTP URL. Adds the torrent to
    // the session (non-blocking — metadata/pieces are fetched lazily as the
    // player's first Range request drives the engine). fileIdx < 0 => the engine
    // guesses the largest video file at request time.
    // Returns "http://127.0.0.1:<port>/<hash>/<idx>" (or "" if not started).
    std::string streamUrl(const std::string& infoHash, int fileIdx,
                          const std::vector<std::string>& trackers);

private:
    struct Impl;
    std::unique_ptr<Impl> d_;
};

} // namespace tankoban::tstream
