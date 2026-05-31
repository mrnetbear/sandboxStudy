cmake_minimum_required(VERSION 3.5)

project(sandboxStudy VERSION 0.2 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CAEN_DIGITIZER_INCLUDE_DIR "/usr/include/")
set(CAEN_DIGITIZER_LIB "/usr/lib/libCAENDigitizer.so")

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(sandboxStudy
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        dialogconnect.cpp dialogconnect.h dialogconnect.ui dialogprotocol.cpp dialogprotocol.h dialogprotocol.ui dialogsettingvispath.cpp dialogsettingvispath.h dialogsettingvispath.ui digitizeroperation.cpp digitizeroperation.h
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET sandboxStudy APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(sandboxStudy SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(sandboxStudy
            ${PROJECT_SOURCES}
            dialogconnect.cpp dialogconnect.h dialogconnect.ui dialogprotocol.cpp dialogprotocol.h dialogprotocol.ui dialogsettingvispath.cpp dialogsettingvispath.h dialogsettingvispath.ui main.cpp mainwindow.cpp mainwindow.h mainwindow.ui
            digitizeroperation.h digitizeroperation.cpp
            logger.h logger.cpp
        )
    endif()
endif()

target_link_libraries(sandboxStudy PRIVATE Qt${QT_VERSION_MAJOR}::Widgets ${CAEN_DIGITIZER_LIB}) #CAENDigitizer)


# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.sandboxStudy)
endif()
set_target_properties(sandboxStudy PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS sandboxStudy
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(sandboxStudy)
endif()
