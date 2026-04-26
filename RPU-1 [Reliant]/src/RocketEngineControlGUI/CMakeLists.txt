cmake_minimum_required(VERSION 3.16)

project(RocketEngineControlGUI VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_PREFIX_PATH "~/Qt/6.7.2/gcc_64/lib/cmake")

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets Charts SerialPort)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Charts SerialPort)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
)


qt_add_executable(RocketEngineControlGUI
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}

    components/datachart.h components/datachart.cpp components/datachart.ui

    communications/serialworker.h communications/serialworker.cpp
    communications/sensordata.h
    logging/loghandler.h logging/loghandler.cpp
    components/alertdialog.h components/alertdialog.cpp components/alertdialog.ui


)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(RocketEngineControlGUI PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(RocketEngineControlGUI PRIVATE Qt${QT_VERSION_MAJOR}::Charts)
target_link_libraries(RocketEngineControlGUI PRIVATE Qt${QT_VERSION_MAJOR}::SerialPort)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.RocketEngineControlGUI)
endif()
set_target_properties(RocketEngineControlGUI PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS RocketEngineControlGUI
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(RocketEngineControlGUI)
endif()
