cmake_minimum_required(VERSION 2.8.12)

project(lokalize)

set (QT_MIN_VERSION "5.2.0")


# search packages used by KDE
find_package(ECM 0.0.10 REQUIRED CONFIG)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED Core DBus Widgets Script Sql)

find_package(KF5 REQUIRED I18n KIO XmlGui Notifications Config CoreAddons DocTools Kross Sonnet 
)

add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)

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
