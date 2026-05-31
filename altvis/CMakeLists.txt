cmake_minimum_required(VERSION 3.16)

project(rootVis VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Поиск ROOT
find_package(ROOT REQUIRED COMPONENTS Core Hist Graf Gpad Tree)

if (ROOT_FOUND)
    message(STATUS "ROOT found at: ${ROOT_DIR}")
    message(STATUS "ROOT version: ${ROOT_VERSION}")
else()
    message(FATAL_ERROR "ROOT not found! Please install ROOT from https://root.cern/install/")
endif()

# Включаем файл ROOT
include(${ROOT_USE_FILE})

# Добавляем определения для GUI
add_definitions(-DROOTGUI)

set(PROJECT_SOURCES
    main.cpp
    rootcontrol.cpp
    rootcontrol.h
)

add_executable(rootVis
    ${PROJECT_SOURCES}
)

target_link_libraries(rootVis PRIVATE
    ${ROOT_LIBRARIES}
    ${ROOT_GUI_LIBRARIES}  # Добавляем GUI библиотеки
    #Threads::Threads        # Для многопоточности
)

target_include_directories(rootVis PRIVATE
    ${ROOT_INCLUDE_DIR}
)

# Устанавливаем свойства для macOS/Linux
if(APPLE)
    set_target_properties(rootVis PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
    )
endif()
