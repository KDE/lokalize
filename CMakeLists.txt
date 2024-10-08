# Lokalize project
cmake_minimum_required(VERSION 3.16)

# KDE Application Version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "24")
set (RELEASE_SERVICE_VERSION_MINOR "11")
set (RELEASE_SERVICE_VERSION_MICRO "70")
set (RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

project(lokalize VERSION ${RELEASE_SERVICE_VERSION})

set(QT_MIN_VERSION "6.6.0")
set(KF_MIN_VERSION "6.0.0")
set(KDE_COMPILERSETTINGS_LEVEL "5.85")

# search packages used
find_package(ECM ${KF_MIN_VERSION} REQUIRED CONFIG)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMAddAppIcon)
include(ECMQtDeclareLoggingCategory)
include(ECMAddTests)

find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} CONFIG REQUIRED Core DBus Widgets Sql Test)

find_package(KF6 ${KF_MIN_VERSION} REQUIRED I18n TextWidgets KIO XmlGui Notifications Config CoreAddons DocTools Sonnet DBusAddons Crash)

find_package(HUNSPELL)
set_package_properties(
    HUNSPELL PROPERTIES
    DESCRIPTION "Library used for stemming"
    URL         "https://hunspell.github.io/"
    TYPE        REQUIRED
    PURPOSE     "Required to build Lokalize.")

add_subdirectory(doc)
add_subdirectory(src)
add_subdirectory(scripts)
add_subdirectory(icons)
add_subdirectory(autotests)

install(FILES org.kde.lokalize.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})

ki18n_install(po)
kdoctools_install(po)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
