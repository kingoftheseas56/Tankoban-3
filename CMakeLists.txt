cmake_minimum_required(VERSION 3.20)
project(Tankoban3 VERSION 0.1.0 LANGUAGES CXX)

# Mirrors Tankoban 2's known-good toolchain: Qt6 (msvc2022_64), C++20, MSVC 2022.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Network: the addon engine (Stremio catalog/meta/stream over HTTP).
# Svg: sidebar nav icons. OpenGL/OpenGLWidgets (libmpv player) get added later.
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Svg OpenGL OpenGLWidgets)

qt_add_executable(Tankoban3
    src/main.cpp

    src/engine/PlayerSnapshot.h
    src/engine/MpvController.h
    src/engine/MpvController.cpp
    src/engine/MpvGlWidget.h
    src/engine/MpvGlWidget.cpp
    src/util/TimeFormat.h
    src/util/TimeFormat.cpp
    src/engine/PlaybackClock.h
    src/engine/PlaybackClock.cpp
    src/player/PlayerView.h
    src/player/PlayerView.cpp
    src/player/PlayerLoadingOverlay.h
    src/player/PlayerLoadingOverlay.cpp
    src/player/HotkeyDispatcher.h
    src/player/HotkeyDispatcher.cpp
    src/util/Theme.h
    src/util/VolumeCurve.h
    src/util/VolumeCurve.cpp
    src/chrome/SeekBar.h
    src/chrome/SeekBar.cpp
    src/chrome/VolumeControl.h
    src/chrome/VolumeControl.cpp
    src/chrome/AudioMenu.h
    src/chrome/AudioMenu.cpp
    src/chrome/TimeLabel.h
    src/chrome/TimeLabel.cpp
    src/chrome/TitleInfo.h
    src/chrome/TitleInfo.cpp
    src/chrome/TransportBar.h
    src/chrome/TransportBar.cpp
    src/chrome/SubtitleMenu.h
    src/chrome/SubtitleMenu.cpp
    src/chrome/SpeedMenu.h
    src/chrome/SpeedMenu.cpp

    src/core/MetaItem.h
    src/core/MetaDetail.h
    src/core/AddonClient.h
    src/core/AddonClient.cpp
    src/core/MetaService.h
    src/core/MetaService.cpp
    src/core/AddonModels.h
    src/core/AddonRegistry.h
    src/core/AddonRegistry.cpp
    src/core/StreamModels.h
    src/core/StreamParser.h
    src/core/StreamParser.cpp
    src/core/StreamScorer.h
    src/core/StreamScorer.cpp
    src/core/StreamService.h
    src/core/StreamService.cpp
    src/core/JikanClient.h
    src/core/JikanClient.cpp
    src/core/AddonStoreModels.h
    src/core/StremioAddonsProvider.h
    src/core/StremioAddonsProvider.cpp

    src/ui/Theme.h
    src/ui/Icons.h
    src/ui/Sidebar.h
    src/ui/Sidebar.cpp
    src/ui/BackdropLayer.h
    src/ui/BackdropLayer.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/ui/PickerHeader.h
    src/ui/PickerHeader.cpp
    src/ui/PlayPickerPage.h
    src/ui/PlayPickerPage.cpp
    src/ui/StreamRowPaint.h
    src/ui/StreamRowPaint.cpp
    src/ui/StremioRow.h
    src/ui/StremioRow.cpp
    src/ui/StreamList.h
    src/ui/StreamList.cpp
    src/ui/VideoPlayerPage.h
    src/ui/VideoPlayerPage.cpp
    src/ui/PosterCard.h
    src/ui/PosterCard.cpp
    src/ui/CatalogRow.h
    src/ui/CatalogRow.cpp
    src/ui/BackToTop.h
    src/ui/BackToTop.cpp
    src/ui/FeaturedHero.h
    src/ui/FeaturedHero.cpp
    src/ui/DetailHero.h
    src/ui/DetailHero.cpp
    src/ui/EpisodeList.h
    src/ui/EpisodeList.cpp
    src/ui/DetailPage.h
    src/ui/DetailPage.cpp
    src/ui/ToggleSwitch.h
    src/ui/ToggleSwitch.cpp
    src/ui/AddonCard.h
    src/ui/AddonCard.cpp
    src/ui/BrowsePane.h
    src/ui/BrowsePane.cpp
    src/ui/AddonsPage.h
    src/ui/AddonsPage.cpp
    src/ui/HomePage.h
    src/ui/HomePage.cpp
    src/ui/DiscoverPage.h
    src/ui/DiscoverPage.cpp
    src/ui/RowPage.h
    src/ui/RowPage.cpp
    src/ui/CinemaHero.h
    src/ui/CinemaHero.cpp
    src/ui/AnimeHero.h
    src/ui/AnimeHero.cpp
    src/ui/AnimePage.h
    src/ui/AnimePage.cpp
    src/ui/FlowLayout.h
    src/ui/FlowLayout.cpp
    src/ui/GridPage.h
    src/ui/GridPage.cpp
    src/ui/PeekHero.h
    src/ui/PeekHero.cpp
    src/ui/ShowsPage.h
    src/ui/ShowsPage.cpp
)

target_include_directories(Tankoban3 PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(Tankoban3 PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Svg
    Qt6::OpenGL
    Qt6::OpenGLWidgets
)


# ── libmpv (vendored, in-process; client + render API) ───────────────────────
set(LIBMPV_DIR ${CMAKE_SOURCE_DIR}/third_party/libmpv)
add_library(libmpv SHARED IMPORTED)
set_target_properties(libmpv PROPERTIES
    IMPORTED_IMPLIB   ${LIBMPV_DIR}/lib/mpv.lib
    IMPORTED_LOCATION ${LIBMPV_DIR}/bin/libmpv-2.dll
    INTERFACE_INCLUDE_DIRECTORIES ${LIBMPV_DIR}/include)
target_link_libraries(Tankoban3 PRIVATE libmpv)

# Copy the libmpv runtime dll next to the exe after build.
add_custom_command(TARGET Tankoban3 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${LIBMPV_DIR}/bin/libmpv-2.dll "$<TARGET_FILE_DIR:Tankoban3>"
    COMMENT "Deploying libmpv-2.dll")

# ── libtorrent (Phase 2: native torrent streaming engine — vendored STATIC) ───
# Mirrors Tankoban 2's PROVEN static recipe for this machine: static
# torrent-rasterbar.lib + header-only Boost + static OpenSSL at C:/tools. STATIC
# => nothing to deploy. Gated: if absent, HAS_LIBTORRENT stays undefined, the
# engine code compiles to a no-op stub, and the app still builds + runs.
target_sources(Tankoban3 PRIVATE
    src/core/torrentstream/LtLinkSmoke.h
    src/core/torrentstream/LtLinkSmoke.cpp
    src/core/torrentstream/StreamEngine.h
    src/core/torrentstream/StreamEngine.cpp)

set(LIBTORRENT_ROOT   "C:/tools/libtorrent-2.0-msvc" CACHE PATH "libtorrent MSVC install")
set(BOOST_ROOT        "C:/tools/boost-1.84.0"        CACHE PATH "Boost headers")
set(OPENSSL_MSVC_ROOT "C:/tools/openssl-msvc"        CACHE PATH "OpenSSL MSVC install")
find_path(LIBTORRENT_INCLUDE_DIR libtorrent/session.hpp HINTS "${LIBTORRENT_ROOT}/include")
find_library(LIBTORRENT_LIBRARY torrent-rasterbar HINTS "${LIBTORRENT_ROOT}/lib")
find_path(BOOST_INCLUDE_DIR boost/version.hpp HINTS "${BOOST_ROOT}")
if(LIBTORRENT_INCLUDE_DIR AND LIBTORRENT_LIBRARY AND BOOST_INCLUDE_DIR)
    message(STATUS "TB3: libtorrent found -> ${LIBTORRENT_LIBRARY}")
    target_include_directories(Tankoban3 PRIVATE
        "${LIBTORRENT_INCLUDE_DIR}" "${BOOST_INCLUDE_DIR}" "${OPENSSL_MSVC_ROOT}/include")
    target_compile_definitions(Tankoban3 PRIVATE
        HAS_LIBTORRENT=1 TORRENT_USE_OPENSSL BOOST_ALL_NO_LIB _WIN32_WINNT=0x0A00)
    target_link_libraries(Tankoban3 PRIVATE
        "${LIBTORRENT_LIBRARY}"
        "${OPENSSL_MSVC_ROOT}/lib/libssl.lib"
        "${OPENSSL_MSVC_ROOT}/lib/libcrypto.lib"
        ws2_32 mswsock crypt32 iphlpapi bcrypt)
else()
    message(WARNING "TB3: libtorrent NOT found at C:/tools — torrent streaming engine disabled.")
endif()

# Deploy the Qt WebP image-format plugin — this Qt build ships none, and metahub
# serves many posters as WebP (they silently fail to decode without it). The
# self-contained DLL lives in resources/imageformats (committed).
add_custom_command(TARGET Tankoban3 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:Tankoban3>/imageformats"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/resources/imageformats/qwebp.dll"
            "$<TARGET_FILE_DIR:Tankoban3>/imageformats/qwebp.dll"
    COMMENT "Deploying qwebp.dll (WebP poster decoder)")

# ── Pure-logic self-test (CTest; no external deps) ───────────────────────────
enable_testing()
add_executable(Tankoban3SelfTest src/selftest_main.cpp src/util/TimeFormat.cpp src/util/VolumeCurve.cpp src/ui/StreamRowPaint.cpp)
target_link_libraries(Tankoban3SelfTest PRIVATE Qt6::Core Qt6::Gui)
target_include_directories(Tankoban3SelfTest PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME selftest COMMAND Tankoban3SelfTest)

