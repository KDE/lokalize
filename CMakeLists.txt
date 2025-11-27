# Lokalize project
cmake_minimum_required(VERSION 3.16)

# KDE Application Version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "25")
set (RELEASE_SERVICE_VERSION_MINOR "11")
set (RELEASE_SERVICE_VERSION_MICRO "90")
set (RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

project(lokalize VERSION ${RELEASE_SERVICE_VERSION})

set(QT_MIN_VERSION "6.6.0")
set(KF_MIN_VERSION "6.3.0")
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
include(KDEClangFormat)
include(KDEGitCommitHooks)

option(BUILD_DOC "Whether to build the documentation" ON)

find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} CONFIG REQUIRED Core Widgets Sql Test)

find_package(KF6 ${KF_MIN_VERSION} REQUIRED I18n TextWidgets KIO XmlGui Notifications Config CoreAddons Sonnet Crash IconThemes ColorScheme)

set(USE_DBUS_DEFAULT OFF)
if(UNIX AND NOT APPLE AND NOT ANDROID AND NOT HAIKU)
    set(USE_DBUS_DEFAULT ON)
endif()
option(USE_DBUS "Build components using DBus" ${USE_DBUS_DEFAULT})
if(USE_DBUS)
    find_package(Qt6DBus REQUIRED)
    find_package(KF6DBusAddons REQUIRED)
    set(HAVE_DBUS 1)
endif()


if(BUILD_DOC)
    find_package(KF6DocTools ${KF_MIN_VERSION})
    set_package_properties(KF6DocTools PROPERTIES
        DESCRIPTION "Tools to generate documentation"
        TYPE REQUIRED
    )
endif()

find_package(HUNSPELL)
set_package_properties(
    HUNSPELL PROPERTIES
    DESCRIPTION "Library used for stemming"
    URL         "https://hunspell.github.io/"
    TYPE        REQUIRED
    PURPOSE     "Required to build Lokalize.")

add_subdirectory(src)
add_subdirectory(icons)
add_subdirectory(autotests)

file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h *.hpp *.c)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)

install(FILES org.kde.lokalize.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})

if(BUILD_DOC)
    add_subdirectory(doc)
    kdoctools_install(po)
endif()

ki18n_install(po)

configure_file(
   ${CMAKE_CURRENT_SOURCE_DIR}/config-lokalize.h.cmake
   ${CMAKE_CURRENT_BINARY_DIR}/src/config-lokalize.h
)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
