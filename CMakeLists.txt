# Lokalize project
cmake_minimum_required(VERSION 3.0)

# KDE Application Version, managed by release script
set (KDE_APPLICATIONS_VERSION_MAJOR "19")
set (KDE_APPLICATIONS_VERSION_MINOR "08")
set (KDE_APPLICATIONS_VERSION_MICRO "1")
set (KDE_APPLICATIONS_VERSION "${KDE_APPLICATIONS_VERSION_MAJOR}.${KDE_APPLICATIONS_VERSION_MINOR}.${KDE_APPLICATIONS_VERSION_MICRO}")

project(lokalize VERSION ${KDE_APPLICATIONS_VERSION})

set(QT_MIN_VERSION "5.5.0")
set(KF5_MIN_VERSION "5.14.0")

# search packages used
find_package(ECM ${KF5_MIN_VERSION} REQUIRED CONFIG)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMAddAppIcon)
include(ECMQtDeclareLoggingCategory)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED Core DBus Widgets Script Sql)

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED I18n KIO XmlGui Notifications Config CoreAddons DocTools Kross Sonnet DBusAddons Crash)

add_definitions(-DQT_USE_QSTRINGBUILDER)
add_definitions(-DQT_NO_CAST_TO_ASCII)

find_package(HUNSPELL)
set_package_properties(
    HUNSPELL PROPERTIES
    DESCRIPTION "Library used for stemming"
    URL         "http://hunspell.sourceforge.net/"
    TYPE        REQUIRED 
    PURPOSE     "Required to build Lokalize.")

include_directories (${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}) 



add_subdirectory(doc)
add_subdirectory(src)
add_subdirectory(scripts)
add_subdirectory(icons)
#add_subdirectory(strigi-analyzer)

install(FILES org.kde.lokalize.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
