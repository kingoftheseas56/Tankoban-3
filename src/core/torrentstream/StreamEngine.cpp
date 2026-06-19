// Tankoban 3 — native torrent streaming engine implementation. See StreamEngine.h.
// Ported from the proven off-master spike (Desktop/tb3-engine-spike/spike.cpp):
// same session settings, deadline scheduler, async piece-wait, and HTTP Range
// server — now PIMPL'd behind a Qt-clean header. NOMINMAX / WIN32_LEAN_AND_MEAN
// must precede the libtorrent (windows.h) includes; scoped to this TU.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>

#include "StreamEngine.h"

#ifdef HAS_LIBTORRENT
#include <libtorrent/session.hpp>
#include <libtorrent/session_params.hpp>
#include <libtorrent/settings_pack.hpp>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/file_storage.hpp>
#include <libtorrent/torrent_status.hpp>
#include <libtorrent/download_priority.hpp>
#include <libtorrent/error_code.hpp>
namespace lt = libtorrent;
#endif

#include <algorithm>
#include <atomic>
#include <chrono>
#include <climits>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <mutex>
#include <set>
#include <thread>
#include <tuple>

namespace tankoban::tstream {

#ifdef HAS_LIBTORRENT
namespace {

void logln(const std::string& s) {
    std::fprintf(stderr, "[stream-engine] %s\n", s.c_str());
    std::fflush(stderr);
}

const char* kDefaultTrackers[] = {
    "udp://tracker.opentrackr.org:1337/announce",
    "udp://open.demonii.com:1337/announce",
    "udp://tracker.openbittorrent.com:6969/announce",
    "udp://exodus.desync.com:6969/announce",
    "udp://tracker.torrent.eu.org:451/announce",
};

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower); return s;
}
bool isVideo(const std::string& name) {
    auto lo = toLower(name);
    static const char* exts[] = {".mp4",".mkv",".avi",".mov",".webm",".m4v",".ts"};
    for (auto* e : exts) { auto n = std::strlen(e);
        if (lo.size() >= n && lo.compare(lo.size()-n, n, e) == 0) return true; }
    return false;
}
std::string jsonEscape(const std::string& s) {
    std::string o; o.reserve(s.size()+8);
    for (char c : s) switch (c) {
        case '\\': o += "\\\\"; break; case '"': o += "\\\""; break;
        case '\n': o += "\\n"; break;  case '\r': o += "\\r"; break;
        case '\t': o += "\\t"; break;  default: o += c; break; }
    return o;
}
std::string hexHash(const lt::torrent_handle& h) {
    auto raw = h.info_hashes().get_best().to_string();
    static const char* d = "0123456789abcdef";
    std::string s; s.reserve(raw.size()*2);
    for (unsigned char c : raw) { s += d[c>>4]; s += d[c&0xf]; }
    return s;
}
bool sendAll(SOCKET s, const char* buf, int len) {
    int sent = 0;
    while (sent < len) { int n = send(s, buf+sent, len-sent, 0); if (n <= 0) return false; sent += n; }
    return true;
}
bool parseRange(const std::string& req, std::int64_t& start, std::int64_t& end) {
    std::string lo = toLower(req);
    auto p = lo.find("range:"); if (p == std::string::npos) return false;
    auto eq = lo.find("bytes=", p); if (eq == std::string::npos) return false;
    eq += 6; auto eol = lo.find("\r\n", eq);
    std::string spec = req.substr(eq, (eol == std::string::npos ? req.size() : eol) - eq);
    auto dash = spec.find('-'); if (dash == std::string::npos) return false;
    start = std::strtoll(spec.substr(0, dash).c_str(), nullptr, 10);
    std::string e = spec.substr(dash + 1);
    end = e.empty() ? -1 : std::strtoll(e.c_str(), nullptr, 10);
    return true;
}
std::vector<std::string> scanTrackers(const std::string& body) {
    std::vector<std::string> out;
    for (const char* scheme : {"udp://","http://","https://"}) {
        size_t pos = 0;
        while ((pos = body.find(scheme, pos)) != std::string::npos) {
            size_t e = body.find_first_of("\"", pos);
            if (e == std::string::npos) break;
            out.push_back(body.substr(pos, e - pos)); pos = e;
        }
    }
    return out;
}

// ── Faithful port of perpetus enginefs/src/backend/priorities.rs (v1 subset) ──
// Playback intents (mpv direct playback only; HLS/download/probe deferred).
enum class Intent { DirectInitial, DirectSeek, ContainerMetadata };

constexpr std::int64_t MIN_STARTUP_BYTES                   = 16LL * 1024 * 1024;
constexpr std::int64_t MAX_STARTUP_WINDOW_BYTES           = 32LL * 1024 * 1024;
constexpr std::int64_t MAX_SEEK_HOT_WINDOW_BYTES          = 128LL * 1024 * 1024;
constexpr std::int64_t MAX_WARM_WINDOW_BYTES              = 256LL * 1024 * 1024;
constexpr std::int64_t MAX_CONTAINER_METADATA_WINDOW_BYTES = 16LL * 1024 * 1024;
constexpr std::int64_t SMALL_FILE_BYTES                   = 64LL * 1024 * 1024;
constexpr int MAX_STARTUP_PIECES            = 4;
constexpr int MAX_SMALL_FILE_STARTUP_PIECES = 32;
constexpr int MIN_STARTUP_PIECES            = 2;
constexpr int MIN_SEEK_HOT_PIECES           = 24;
constexpr int SEEK_IMMEDIATE_PIECES         = 12;
constexpr int MAX_HOT_PIECES                = 96;
constexpr int MAX_WARM_PIECES               = 192;

inline std::int64_t containerMetadataStart(std::int64_t fileSize) {
    if (fileSize == 0) return 0;
    if (fileSize < SMALL_FILE_BYTES) return fileSize * 95 / 100;
    return std::min<std::int64_t>(fileSize - 10LL * 1024 * 1024, fileSize * 95 / 100);
}
inline bool isContainerMetadata(std::int64_t start, std::int64_t reqLen, std::int64_t fileSize) {
    return start > 0 && fileSize > 0 && reqLen > 0
        && reqLen <= MAX_CONTAINER_METADATA_WINDOW_BYTES
        && start >= containerMetadataStart(fileSize);
}
inline int piecesForBytes(std::int64_t bytes, int pieceLen) {
    if (bytes <= 0 || pieceLen <= 0) return 0;
    long long p = (bytes + pieceLen - 1) / pieceLen;
    if (p < 1) p = 1;
    if (p > INT_MAX) p = INT_MAX;
    return int(p);
}
inline int capPiecesByBytes(int pieces, int pieceLen, std::int64_t maxBytes) {
    if (pieces <= 0 || maxBytes == 0) return 0;
    return std::min(pieces, std::max(1, piecesForBytes(maxBytes, pieceLen)));
}
inline int dynamicHotWindow(std::int64_t dlRate, int peers) {
    int hot;
    if (dlRate > 10LL * 1024 * 1024) hot = 96;
    else if (dlRate > 5LL * 1024 * 1024) hot = 48;
    else if (dlRate > 1LL * 1024 * 1024) hot = MIN_SEEK_HOT_PIECES;
    else hot = 16;
    if (peers < 3) hot = std::min(hot, MIN_SEEK_HOT_PIECES);
    return hot;
}
inline const char* contentTypeForName(const std::string& name) {
    std::string lo = name; std::transform(lo.begin(), lo.end(), lo.begin(), ::tolower);
    auto ends = [&](const char* s){ size_t n = std::strlen(s); return lo.size() >= n && lo.compare(lo.size()-n, n, s) == 0; };
    if (ends(".mp4") || ends(".m4v")) return "video/mp4";
    if (ends(".mkv")) return "video/x-matroska";
    if (ends(".ts"))  return "video/mp2t";
    if (ends(".avi")) return "video/x-msvideo";
    if (ends(".mov")) return "video/quicktime";
    if (ends(".webm")) return "video/webm";
    return "application/octet-stream";
}
// Returns (immediate, hot, warm) piece counts for the read-ahead window.
inline std::tuple<int,int,int> decideWindow(Intent intent, bool firstByteSent, int pieceLen,
        std::int64_t fileSize, std::int64_t dlRate, int peers, int currentPiece, int lastPiece) {
    int immediate = 0, hot = 0, warm = 0;
    if (intent == Intent::DirectInitial && !firstByteSent) {
        std::int64_t targetBytes = (fileSize > 0 && fileSize <= SMALL_FILE_BYTES)
            ? std::min<std::int64_t>(fileSize, MAX_STARTUP_WINDOW_BYTES) : MIN_STARTUP_BYTES;
        targetBytes = std::max<std::int64_t>(targetBytes, pieceLen);
        targetBytes = std::min<std::int64_t>(targetBytes, std::max<std::int64_t>(MAX_STARTUP_WINDOW_BYTES, pieceLen));
        int pieces = int((targetBytes + pieceLen - 1) / pieceLen);
        int maxStartup = (fileSize > 0 && fileSize <= SMALL_FILE_BYTES)
            ? std::clamp(piecesForBytes(std::min<std::int64_t>(fileSize, MAX_STARTUP_WINDOW_BYTES), pieceLen), 1, MAX_SMALL_FILE_STARTUP_PIECES)
            : MAX_STARTUP_PIECES;
        int minStartup = std::max(1, std::min(MIN_STARTUP_PIECES, maxStartup));
        pieces = std::clamp(pieces, minStartup, maxStartup);
        immediate = std::min(pieces, MAX_STARTUP_PIECES);
        hot = pieces; warm = 0;
    } else if (intent == Intent::DirectInitial) {
        hot = dynamicHotWindow(dlRate, peers); immediate = 2; warm = 32;
    } else if (intent == Intent::DirectSeek) {
        hot = std::max(dynamicHotWindow(dlRate, peers), MIN_SEEK_HOT_PIECES);
        immediate = SEEK_IMMEDIATE_PIECES; warm = 32;
    } else { // ContainerMetadata
        immediate = 1; hot = 2; warm = 0;
    }
    const std::int64_t hotCap = (intent == Intent::DirectInitial && !firstByteSent) ? MAX_STARTUP_WINDOW_BYTES
                              : (intent == Intent::ContainerMetadata) ? MAX_CONTAINER_METADATA_WINDOW_BYTES
                              : MAX_SEEK_HOT_WINDOW_BYTES;
    const std::int64_t warmCap = (intent == Intent::ContainerMetadata) ? 0 : MAX_WARM_WINDOW_BYTES;
    hot  = capPiecesByBytes(hot,  pieceLen, hotCap);
    warm = capPiecesByBytes(warm, pieceLen, warmCap);
    const int remaining = lastPiece - currentPiece + 1;
    hot  = std::min(std::clamp(hot, 0, MAX_HOT_PIECES), remaining);
    warm = std::min(std::clamp(warm, 0, MAX_WARM_PIECES), std::max(0, remaining - hot));
    immediate = std::max(0, std::min(immediate, hot));
    return std::make_tuple(immediate, hot, warm);
}

} // namespace
#endif // HAS_LIBTORRENT

// =============================================================================
struct StreamEngine::Impl {
    std::string savePath;
    int port = -1;
    std::atomic<bool> stop{false};

#ifdef HAS_LIBTORRENT
    std::unique_ptr<lt::session> ses;
    std::thread pump;
    SOCKET listenSock = INVALID_SOCKET;
    std::thread acceptThread;
    bool wsaUp = false;

    struct Torrent {
        lt::torrent_handle handle;
        std::shared_ptr<const lt::torrent_info> ti;
        std::mutex mtx;
        std::condition_variable cv;
        std::set<int> havePieces;
        std::atomic<bool> metadataReady{false};
        std::atomic<bool> firstByteSent{false};   // expands the initial window after first byte
        int prioritizedFile = -1;
    };
    std::mutex mapMtx;
    std::map<std::string, std::shared_ptr<Torrent>> torrents;

    static lt::settings_pack makeSettings() {
        lt::settings_pack sp;
        sp.set_str(lt::settings_pack::listen_interfaces, "0.0.0.0:6881,[::]:6881");
        sp.set_str(lt::settings_pack::dht_bootstrap_nodes,
                   "router.bittorrent.com:6881,router.utorrent.com:6881,"
                   "dht.transmissionbt.com:6881,dht.libtorrent.org:25401");
        sp.set_bool(lt::settings_pack::enable_dht, true);
        sp.set_bool(lt::settings_pack::enable_lsd, true);
        sp.set_bool(lt::settings_pack::enable_natpmp, true);
        sp.set_bool(lt::settings_pack::enable_upnp, true);
        sp.set_int(lt::settings_pack::alert_mask,
                   lt::alert_category::status | lt::alert_category::storage
                 | lt::alert_category::error  | lt::alert_category::piece_progress);
        sp.set_int(lt::settings_pack::connections_limit, 400);
        sp.set_int(lt::settings_pack::max_queued_disk_bytes, 32 * 1024 * 1024);
        sp.set_int(lt::settings_pack::request_queue_time, 10);
        sp.set_int(lt::settings_pack::max_out_request_queue, 1500);
        sp.set_bool(lt::settings_pack::announce_to_all_trackers, true);
        sp.set_bool(lt::settings_pack::announce_to_all_tiers, true);
        sp.set_int(lt::settings_pack::in_enc_policy,  lt::settings_pack::pe_enabled);
        sp.set_int(lt::settings_pack::out_enc_policy, lt::settings_pack::pe_enabled);
        return sp;
    }

    std::shared_ptr<Torrent> findByHash(const std::string& h) {
        std::lock_guard<std::mutex> lk(mapMtx);
        auto it = torrents.find(h);
        return it == torrents.end() ? nullptr : it->second;
    }

    std::shared_ptr<Torrent> addOrGet(const std::string& h, const std::vector<std::string>& trackers) {
        std::lock_guard<std::mutex> lk(mapMtx);
        auto it = torrents.find(h);
        if (it != torrents.end()) return it->second;
        lt::error_code ec;
        lt::add_torrent_params atp = lt::parse_magnet_uri("magnet:?xt=urn:btih:" + h, ec);
        if (ec) { logln("parse_magnet_uri: " + ec.message()); return nullptr; }
        for (auto& t : trackers)         atp.trackers.push_back(t);
        for (auto* t : kDefaultTrackers) atp.trackers.push_back(t);
        atp.save_path = savePath;
        lt::torrent_handle hd = ses->add_torrent(std::move(atp), ec);
        if (ec) { logln("add_torrent: " + ec.message()); return nullptr; }
        auto t = std::make_shared<Torrent>();
        t->handle = hd;
        torrents[h] = t;
        logln("added torrent " + h);
        return t;
    }

    bool waitMetadata(const std::shared_ptr<Torrent>& t, int timeoutMs) {
        std::unique_lock<std::mutex> lk(t->mtx);
        return t->cv.wait_for(lk, std::chrono::milliseconds(timeoutMs),
                              [&]{ return t->metadataReady.load() || stop.load(); })
               && t->metadataReady.load();
    }

    // Diagnostic: periodic per-torrent swarm health (peers/seeds/rate/progress).
    void logStats() {
        std::lock_guard<std::mutex> lk(mapMtx);
        for (auto& kv : torrents) {
            auto& t = kv.second;
            if (!t->handle.is_valid()) continue;
            lt::torrent_status st = t->handle.status();
            const char* state = "?";
            switch (st.state) {
                case lt::torrent_status::checking_files:        state = "checking";        break;
                case lt::torrent_status::downloading_metadata:  state = "metadata";        break;
                case lt::torrent_status::downloading:           state = "downloading";     break;
                case lt::torrent_status::finished:              state = "finished";        break;
                case lt::torrent_status::seeding:               state = "seeding";         break;
                case lt::torrent_status::checking_resume_data:  state = "checking_resume"; break;
                default: break;
            }
            logln("stats " + kv.first.substr(0, 8) + " state=" + state
                + " peers=" + std::to_string(st.num_peers)
                + " seeds=" + std::to_string(st.num_seeds)
                + " conns=" + std::to_string(st.num_connections)
                + " dl=" + std::to_string(st.download_rate / 1024) + "KB/s"
                + " done=" + std::to_string((long long)(st.total_done / (1024 * 1024))) + "MB"
                + " prog=" + std::to_string(int(st.progress * 100)) + "%");
        }
    }

    static int guessFileIdx(const lt::torrent_info& ti) {
        const auto& fs = ti.files();
        int best = -1; std::int64_t bestSz = -1;
        for (int i = 0; i < fs.num_files(); ++i) {
            auto fi = lt::file_index_t{i};
            if (isVideo(fs.file_path(fi)) && fs.file_size(fi) > bestSz) { bestSz = fs.file_size(fi); best = i; }
        }
        return best;
    }

    // Faithful port of PlaybackPriorityPolicy::decide + assignment_for: set the
    // deadline + priority bands (immediate prio7, hot prio4, warm prio2) across the
    // read-ahead window anchored at the playback head. Re-aimed as the head advances.
    void applyWindow(const std::shared_ptr<Torrent>& t, Intent intent, int currentPiece,
                     bool firstByteSent, int pieceLen, std::int64_t fileSize, int lastPiece) {
        lt::torrent_status st = t->handle.status();
        const auto win = decideWindow(intent, firstByteSent, pieceLen, fileSize,
                                      st.download_rate, st.num_peers, currentPiece, lastPiece);
        const int immediate = std::get<0>(win), hot = std::get<1>(win), warm = std::get<2>(win);
        const int window = hot + warm;
        for (int d = 0; d < window; ++d) {
            const int p = currentPiece + d;
            if (p > lastPiece) break;
            int prio, deadline;
            if (intent == Intent::ContainerMetadata) { prio = 4; deadline = 150 + d * 50; }
            else if (d < immediate)                  { prio = 7; deadline = d * 25; }
            else if (d < hot)                        { prio = 4; deadline = 1500 + d * 150; }
            else                                     { prio = 2; deadline = 10000 + d * 250; }
            t->handle.set_piece_deadline(lt::piece_index_t{p}, deadline);
            t->handle.piece_priority(lt::piece_index_t{p}, lt::download_priority_t(prio));
        }
    }

    // Patient wait: block (this worker thread only) until [head..tail] are present,
    // or timeout. cv woken by piece_finished_alert. Does NOT touch deadlines.
    bool waitPieces(const std::shared_ptr<Torrent>& t, int head, int tail, int timeoutMs) {
        std::unique_lock<std::mutex> lk(t->mtx);
        return t->cv.wait_for(lk, std::chrono::milliseconds(timeoutMs), [&]{
            if (stop.load()) return true;
            for (int p = head; p <= tail; ++p)
                if (t->havePieces.find(p) == t->havePieces.end()) return false;
            return true;
        });
    }

    bool readFileBytes(const std::shared_ptr<Torrent>& t, const std::string& path,
                       std::int64_t pos, char* buf, int n) {
        for (int attempt = 0; attempt < 6; ++attempt) {
            std::ifstream f(path, std::ios::binary);
            if (f) { f.seekg(pos); f.read(buf, n); if (int(f.gcount()) == n) return true; }
            t->handle.flush_cache();
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
        }
        return false;
    }

    void pumpLoop() {
        using namespace std::chrono;
        auto lastStats = steady_clock::now();
        while (!stop.load()) {
            lt::alert* a = ses->wait_for_alert(milliseconds(25));
            auto now = steady_clock::now();
            if (now - lastStats >= seconds(2)) { lastStats = now; logStats(); }
            if (!a) continue;
            std::vector<lt::alert*> alerts;
            ses->pop_alerts(&alerts);
            for (lt::alert* al : alerts) {
                if (auto* mra = lt::alert_cast<lt::metadata_received_alert>(al)) {
                    auto t = findByHash(hexHash(mra->handle));
                    if (!t) continue;
                    { std::lock_guard<std::mutex> lk(t->mtx); t->ti = mra->handle.torrent_file(); }
                    t->metadataReady.store(true);
                    t->cv.notify_all();
                } else if (auto* pfa = lt::alert_cast<lt::piece_finished_alert>(al)) {
                    auto t = findByHash(hexHash(pfa->handle));
                    if (!t) continue;
                    { std::lock_guard<std::mutex> lk(t->mtx); t->havePieces.insert(int(pfa->piece_index)); }
                    t->cv.notify_all();
                } else if (auto* tea = lt::alert_cast<lt::torrent_error_alert>(al)) {
                    logln(std::string("torrent_error: ") + tea->message());
                }
            }
        }
    }

    void handleCreate(SOCKET c, const std::string& reqHeaders, const std::string& hash) {
        std::string body;
        auto h = reqHeaders.find("\r\n\r\n");
        if (h != std::string::npos) body = reqHeaders.substr(h + 4);
        auto trackers = scanTrackers(body);
        logln("CREATE " + hash + " (" + std::to_string(trackers.size()) + " trackers)");
        auto t = addOrGet(hash, trackers);
        if (!t || !waitMetadata(t, 60000)) {
            const char* r = "HTTP/1.1 504 Gateway Timeout\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
            sendAll(c, r, int(std::strlen(r))); closesocket(c); return;
        }
        auto ti = t->ti; const auto& fs = ti->files();
        int guessed = guessFileIdx(*ti);
        std::string j = "{\"name\":\"" + jsonEscape(std::string(ti->name())) + "\",\"infoHash\":\"" + hash + "\",\"files\":[";
        for (int i = 0; i < fs.num_files(); ++i) {
            auto fi = lt::file_index_t{i};
            if (i) j += ",";
            j += "{\"idx\":" + std::to_string(i)
               + ",\"name\":\"" + jsonEscape(std::string(fs.file_name(fi))) + "\""
               + ",\"path\":\"" + jsonEscape(std::string(fs.file_path(fi))) + "\""
               + ",\"length\":" + std::to_string((long long)fs.file_size(fi))
               + ",\"offset\":" + std::to_string((long long)fs.file_offset(fi)) + "}";
        }
        j += "],\"guessedFileIdx\":" + std::to_string(guessed) + "}";
        std::string hdr = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: "
                        + std::to_string(j.size()) + "\r\nConnection: close\r\n\r\n";
        sendAll(c, hdr.data(), int(hdr.size()));
        sendAll(c, j.data(), int(j.size()));
        closesocket(c);
        logln("CREATE -> " + std::to_string(fs.num_files()) + " files, guessedFileIdx=" + std::to_string(guessed));
    }

    void handleStream(SOCKET c, const std::string& req, const std::string& hash, int idx) {
        auto t = findByHash(hash);
        if (!t) t = addOrGet(hash, {});
        if (!t || !waitMetadata(t, 60000)) {
            const char* r = "HTTP/1.1 504 Gateway Timeout\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
            sendAll(c, r, int(std::strlen(r))); closesocket(c); return;
        }
        auto ti = t->ti; const auto& fs = ti->files();
        if (idx < 0 || idx >= fs.num_files()) idx = guessFileIdx(*ti);
        auto fi = lt::file_index_t{idx};
        const std::int64_t fileAbsOffset = fs.file_offset(fi);
        const std::int64_t fileSize      = fs.file_size(fi);
        const int pieceLen   = ti->piece_length();
        const int lastPiece  = int((fileAbsOffset + fileSize - 1) / pieceLen);
        const std::string filePath = savePath + "/" + fs.file_path(fi);

        if (t->prioritizedFile != idx) {
            std::vector<lt::download_priority_t> prio(fs.num_files(), lt::download_priority_t{1});
            prio[idx] = lt::download_priority_t{7};
            t->handle.prioritize_files(prio);
            t->prioritizedFile = idx;
        }

        const std::string fileName = std::string(fs.file_name(fi));

        std::int64_t start = 0, end = fileSize - 1;
        bool hasRange = parseRange(req, start, end);
        if (end < 0 || end >= fileSize) end = fileSize - 1;
        if (start < 0 || start >= fileSize) start = 0;
        const std::int64_t reqLen = end - start + 1;

        // Classify the request the way perpetus/Stremio do — by offset, not heuristics.
        Intent intent;
        if (start == 0)                                        intent = Intent::DirectInitial;
        else if (isContainerMetadata(start, reqLen, fileSize)) intent = Intent::ContainerMetadata;
        else                                                   intent = Intent::DirectSeek;
        const int curPiece = int((fileAbsOffset + start) / pieceLen);
        logln(std::string(hasRange ? "GET Range " : "GET full ") + hash.substr(0, 8) + "/"
              + std::to_string(idx) + " " + std::to_string(start) + "-" + std::to_string(end)
              + " intent=" + (intent == Intent::DirectInitial ? "initial"
                            : intent == Intent::DirectSeek ? "seek" : "meta"));

        // Only a genuine seek refocuses the swarm — clear stale deadlines then.
        // Initial / container-metadata / reconnects do NOT clear (clearing every
        // request thrashes the scheduler — that was a root cause of the stall).
        if (intent == Intent::DirectSeek) t->handle.clear_piece_deadlines();
        applyWindow(t, intent, curPiece, t->firstByteSent.load(), pieceLen, fileSize, lastPiece);

        // Short, NON-FATAL readiness wait before headers (perpetus pattern): give the
        // head a moment, then send headers regardless. The real waiting is the
        // patient body below — which never closes on a stall (closing early was what
        // triggered mpv's reconnect storm).
        waitPieces(t, curPiece, curPiece, intent == Intent::DirectInitial ? 2000 : 750);

        char hdr[640];
        int hlen = std::snprintf(hdr, sizeof hdr,
            "HTTP/1.1 %s\r\nContent-Type: %s\r\nAccept-Ranges: bytes\r\n%s%s%s"
            "Content-Length: %lld\r\nConnection: close\r\n\r\n",
            hasRange ? "206 Partial Content" : "200 OK",
            contentTypeForName(fileName),
            hasRange ? "Content-Range: bytes " : "",
            hasRange ? (std::to_string(start)+"-"+std::to_string(end)+"/"+std::to_string(fileSize)).c_str() : "",
            hasRange ? "\r\n" : "",
            (long long)reqLen);
        if (!sendAll(c, hdr, hlen)) { closesocket(c); return; }

        // Patient body: stream chunk-by-chunk; wait for each chunk's pieces with a
        // generous cap and NEVER close on a short stall. Re-aim the window at the
        // advancing head each chunk (read-ahead follows playback).
        const int CHUNK = 256 * 1024;
        std::vector<char> body(CHUNK);
        std::int64_t pos = start;
        while (pos <= end && !stop.load()) {
            const std::int64_t chunkEnd = std::min<std::int64_t>(pos + CHUNK - 1, end);
            const int n = int(chunkEnd - pos + 1);
            const int head = int((fileAbsOffset + pos)      / pieceLen);
            const int tail = int((fileAbsOffset + chunkEnd) / pieceLen);
            applyWindow(t, intent, head, t->firstByteSent.load(), pieceLen, fileSize, lastPiece);
            if (!waitPieces(t, head, tail, 120000)) { logln("piece wait TIMEOUT (120s) — giving up"); break; }
            if (!readFileBytes(t, filePath, pos, body.data(), n)) { logln("disk read FAILED"); break; }
            if (!sendAll(c, body.data(), n)) break;   // client closed (e.g. seek) — fine
            t->firstByteSent.store(true);
            pos += n;
        }
        closesocket(c);
    }

    void handleConn(SOCKET c) {
        std::string buf; char tmp[4096];
        while (buf.find("\r\n\r\n") == std::string::npos) {
            int n = recv(c, tmp, sizeof tmp, 0);
            if (n <= 0) { closesocket(c); return; }
            buf.append(tmp, n);
            if (buf.size() > (1u<<20)) break;
        }
        { // drain request body (Content-Length) so closesocket() doesn't RST
            size_t he = buf.find("\r\n\r\n");
            size_t headerEnd = (he == std::string::npos) ? buf.size() : he + 4;
            long contentLen = 0;
            std::string lo = toLower(buf.substr(0, headerEnd));
            auto cl = lo.find("content-length:");
            if (cl != std::string::npos) contentLen = std::strtol(buf.c_str() + cl + 15, nullptr, 10);
            long haveBody = long(buf.size() - headerEnd);
            while (haveBody < contentLen) {
                int n = recv(c, tmp, sizeof tmp, 0);
                if (n <= 0) break;
                buf.append(tmp, n); haveBody += n;
            }
        }
        auto sp1 = buf.find(' ');
        auto sp2 = buf.find(' ', sp1 + 1);
        if (sp1 == std::string::npos || sp2 == std::string::npos) { closesocket(c); return; }
        std::string path = buf.substr(sp1 + 1, sp2 - sp1 - 1);
        std::vector<std::string> seg;
        { size_t i = 0; while (i < path.size()) {
            if (path[i] == '/') { ++i; continue; }
            size_t j = path.find('/', i); if (j == std::string::npos) j = path.size();
            seg.push_back(path.substr(i, j - i)); i = j; } }
        if (seg.size() >= 2 && seg[1] == "create") handleCreate(c, buf, toLower(seg[0]));
        else if (seg.size() >= 2)                  handleStream(c, buf, toLower(seg[0]), std::atoi(seg[1].c_str()));
        else {
            const char* r = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
            sendAll(c, r, int(std::strlen(r))); closesocket(c);
        }
    }
#endif // HAS_LIBTORRENT

    ~Impl() {
        stop.store(true);
#ifdef HAS_LIBTORRENT
        if (listenSock != INVALID_SOCKET) { closesocket(listenSock); listenSock = INVALID_SOCKET; }
        if (acceptThread.joinable()) acceptThread.join();
        { std::lock_guard<std::mutex> lk(mapMtx);
          for (auto& kv : torrents) kv.second->cv.notify_all(); }
        if (pump.joinable()) pump.join();
        ses.reset();
        if (wsaUp) { WSACleanup(); wsaUp = false; }
#endif
    }
};

// ---- public API (delegates to Impl) ----------------------------------------
StreamEngine::StreamEngine(std::string savePath) : d_(new Impl()) {
    d_->savePath = std::move(savePath);
}
StreamEngine::~StreamEngine() = default;

bool StreamEngine::start() {
#ifdef HAS_LIBTORRENT
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) return false;
    d_->wsaUp = true;

    lt::settings_pack sp = Impl::makeSettings();
    lt::session_params params(sp);
    d_->ses.reset(new lt::session(params));
    d_->pump = std::thread([this]{ d_->pumpLoop(); });

    SOCKET srv = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{}; addr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr); addr.sin_port = 0;
    if (bind(srv, (sockaddr*)&addr, sizeof addr) != 0) { closesocket(srv); return false; }
    if (::listen(srv, 32) != 0) { closesocket(srv); return false; }
    sockaddr_in bound{}; int blen = sizeof bound;
    getsockname(srv, (sockaddr*)&bound, &blen);
    d_->port = ntohs(bound.sin_port);
    d_->listenSock = srv;
    d_->acceptThread = std::thread([this, srv]{
        for (;;) {
            SOCKET c = accept(srv, nullptr, nullptr);
            if (c == INVALID_SOCKET) break;          // listen socket closed on stop
            std::thread([this, c]{ d_->handleConn(c); }).detach();
        }
    });
    return true;
#else
    return false;
#endif
}

int StreamEngine::port() const { return d_->port; }

std::string StreamEngine::baseUrl() const {
    return d_->port < 0 ? std::string() : "http://127.0.0.1:" + std::to_string(d_->port);
}

std::string StreamEngine::streamUrl(const std::string& infoHash, int fileIdx,
                                    const std::vector<std::string>& trackers) {
#ifdef HAS_LIBTORRENT
    if (d_->port < 0) return {};
    std::string hash = infoHash;
    std::transform(hash.begin(), hash.end(), hash.begin(), ::tolower);
    d_->addOrGet(hash, trackers);   // kick the fetch now; first Range drives the wait
    return baseUrl() + "/" + hash + "/" + std::to_string(fileIdx);
#else
    (void)infoHash; (void)fileIdx; (void)trackers; return {};
#endif
}

} // namespace tankoban::tstream
