cmake_minimum_required(VERSION 3.16)

project(rootVis VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(ROOT)
if (ROOT_FOUND)
    message(STATUS "ROOT found: visualization enabled")
else()
    message(STATUS "ROOT not found: visualization disabled")
    find_package(/usr/local/root/build/ROOT)
endif()

include(${ROOT_USE_FILE})

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
)

target_include_directories(rootVis PRIVATE
    ${ROOT_INCLUDE_DIR}
)
