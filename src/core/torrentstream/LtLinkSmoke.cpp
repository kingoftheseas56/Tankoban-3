// Phase 2 (throwaway) link smoke — see LtLinkSmoke.h.
// NOMINMAX / WIN32_LEAN_AND_MEAN must precede the libtorrent (windows.h) includes
// so std::min/std::max and winsock aren't clobbered. Scoped to THIS TU only.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "LtLinkSmoke.h"

#include <cstdio>

#ifdef HAS_LIBTORRENT
#include <libtorrent/session.hpp>
#include <libtorrent/session_params.hpp>
#include <libtorrent/settings_pack.hpp>
#include <libtorrent/version.hpp>
namespace lt = libtorrent;
#endif

namespace tankoban::tstream {

void ltLinkSmoke() {
#ifdef HAS_LIBTORRENT
    lt::settings_pack sp;
    sp.set_int(lt::settings_pack::alert_mask, lt::alert_category::error);
    sp.set_bool(lt::settings_pack::enable_dht, false);
    sp.set_bool(lt::settings_pack::enable_lsd, false);
    sp.set_bool(lt::settings_pack::enable_upnp, false);
    sp.set_bool(lt::settings_pack::enable_natpmp, false);
    sp.set_str(lt::settings_pack::listen_interfaces, "127.0.0.1:0");
    {
        lt::session_params params(sp);   // named var: avoids the most-vexing-parse
        lt::session ses(params);
        std::fprintf(stderr,
            "[TB3 lt-smoke] libtorrent " LIBTORRENT_VERSION
            " session constructed + destroyed OK (in-process)\n");
        std::fflush(stderr);
    }
#else
    std::fprintf(stderr, "[TB3 lt-smoke] HAS_LIBTORRENT undefined — engine disabled\n");
    std::fflush(stderr);
#endif
}

} // namespace tankoban::tstream
