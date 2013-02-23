cmake_minimum_required(VERSION 2.8)

project(lokalize)

# search packages used by KDE
find_package(KDE4 REQUIRED)
include(KDE4Defaults)
include(MacroLibrary)

add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)

macro_optional_find_package(HUNSPELL)
macro_log_feature(HUNSPELL_FOUND "HUNSPELL" "Library used for stemming" "http://hunspell.sourceforge.net/" FALSE "" "Required to build Lokalize.")

if(HUNSPELL_FOUND OR WIN32)
  include_directories (${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${KDE4_INCLUDES})
  add_subdirectory(doc)
  add_subdirectory(src)
  add_subdirectory(scripts)
  add_subdirectory(icons)
endif()
#add_subdirectory(strigi-analyzer)
