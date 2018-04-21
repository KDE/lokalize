cmake_minimum_required(VERSION 2.8.12)

project(lokalize)

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
