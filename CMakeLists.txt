cmake_minimum_required(VERSION 3.20)
project(Tankoban3 VERSION 0.1.0 LANGUAGES CXX)

# Mirrors Tankoban 2's known-good toolchain: Qt6 (msvc2022_64), C++20, MSVC 2022.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Step 2 needs the GUI core. More components (Network for the addon engine,
# OpenGL/OpenGLWidgets for the libmpv player, Svg for icons) get added as we reach them.
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_add_executable(Tankoban3
    src/main.cpp
    src/ui/Theme.h
    src/ui/Sidebar.h
    src/ui/Sidebar.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
)

target_include_directories(Tankoban3 PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(Tankoban3 PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)
