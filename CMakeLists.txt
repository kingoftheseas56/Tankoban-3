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
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Svg)

qt_add_executable(Tankoban3
    src/main.cpp

    src/core/MetaItem.h
    src/core/AddonClient.h
    src/core/AddonClient.cpp

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
)
