cmake_minimum_required(VERSION 3.16)

project(rootVis VERSION 0.3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)

find_package(ROOT REQUIRED COMPONENTS Core Hist Graf Gpad Gui RIO)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Network)

include(${ROOT_USE_FILE})

set(PROJECT_SOURCES
    main.cpp

    rootcontrol.cpp
    rootcontrol.h

    waveformserver.cpp
    waveformserver.h

    digitizerprotocol.h
)

add_executable(rootVis
    ${PROJECT_SOURCES}
)

target_include_directories(rootVis PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ROOT_INCLUDE_DIRS}
)

target_link_libraries(rootVis PRIVATE
    ${ROOT_LIBRARIES}
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Network
)