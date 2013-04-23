cmake_minimum_required(VERSION 2.8)

project(lokalize)

# search packages used by KDE
find_package(KDE4 REQUIRED)
include(KDE4Defaults)
include(MacroLibrary)

add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)

find_package(HUNSPELL)
set_package_properties(
    HUNSPELL PROPERTIES
    DESCRIPTION "Library used for stemming"
    URL         "http://hunspell.sourceforge.net/"
    TYPE        REQUIRED 
    PURPOSE     "Required to build Lokalize.")

include_directories (${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${KDE4_INCLUDES})
add_subdirectory(doc)
add_subdirectory(src)
add_subdirectory(scripts)
add_subdirectory(icons)
#add_subdirectory(strigi-analyzer)
