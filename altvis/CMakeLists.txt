cmake_minimum_required(VERSION 3.16)
project(rootVis VERSION 0.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(ROOT REQUIRED COMPONENTS Core Hist Graf Gpad Gui)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network)

include(${ROOT_USE_FILE})

add_executable(rootVis
    main.cpp
    rootcontrol.cpp
    rootcontrol.h
    digitizerprotocol.h
    waveformserver.cpp
    waveformserver.h
)

target_link_libraries(rootVis PRIVATE
    ${ROOT_LIBRARIES}
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
)

target_include_directories(rootVis PRIVATE ${ROOT_INCLUDE_DIRS})