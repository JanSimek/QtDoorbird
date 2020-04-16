cmake_minimum_required(VERSION 3.5)

project(QtDoorbird LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package(PkgConfig REQUIRED)
find_package(Qt5 COMPONENTS Widgets Multimedia MultimediaWidgets LinguistTools Network REQUIRED)

find_package(Sodium REQUIRED)
include_directories(${SODIUM_INCLUDE_DIR})

set(TS_FILES localization/QtDoorbird_cs_CZ.ts)

add_executable(QtDoorbird
    src/main.cpp

    src/doorbirdclient.cpp
    src/doorbirdclient.h

    src/mainwindow.cpp
    src/mainwindow.h
    src/mainwindow.ui

    src/credentialsdialog.cpp
    src/credentialsdialog.h
    src/credentialsdialog.ui

    src/settingsdialog.cpp
    src/settingsdialog.h
    src/settingsdialog.ui

    ${TS_FILES}
    resources/resource.qrc
)

target_link_libraries(QtDoorbird PRIVATE Qt5::Widgets Qt5::Network Qt5::Multimedia Qt5::MultimediaWidgets ${sodium_LIBRARY_RELEASE})


qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
