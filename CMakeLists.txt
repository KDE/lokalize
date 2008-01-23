project(kaider)

macro_optional_find_package(Strigi)


add_subdirectory(src)
add_subdirectory(icons)
if(STRIGI_FOUND)
   add_subdirectory(strigi-analyzer)
endif(STRIGI_FOUND)
