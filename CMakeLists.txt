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
    src/player/HotkeyDispatcher.h
    src/player/HotkeyDispatcher.cpp
    src/util/Theme.h
    src/chrome/SeekBar.h
    src/chrome/SeekBar.cpp

    src/core/MetaItem.h
    src/core/MetaDetail.h
    src/core/AddonClient.h
    src/core/AddonClient.cpp
    src/core/MetaService.h
    src/core/MetaService.cpp
    src/core/AddonModels.h
    src/core/AddonRegistry.h
    src/core/AddonRegistry.cpp
    src/core/AddonStoreModels.h
    src/core/StremioAddonsProvider.h
    src/core/StremioAddonsProvider.cpp

    src/ui/Theme.h
    src/ui/Icons.h
    src/ui/Sidebar.h
    src/ui/Sidebar.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/ui/PosterCard.h
    src/ui/PosterCard.cpp
    src/ui/CatalogRow.h
    src/ui/CatalogRow.cpp
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
add_executable(Tankoban3SelfTest src/selftest_main.cpp src/util/TimeFormat.cpp)
target_link_libraries(Tankoban3SelfTest PRIVATE Qt6::Core)
target_include_directories(Tankoban3SelfTest PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME selftest COMMAND Tankoban3SelfTest)

