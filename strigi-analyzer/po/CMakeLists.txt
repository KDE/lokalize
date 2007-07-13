include_directories(${STRIGI_INCLUDE_DIR} ${QT_INCLUDES})

# build the analyzer as a module
kde4_add_library(po MODULE poendanalyzer.cpp)

# link with the required libraries
target_link_libraries(po ${STRIGI_STREAMANALYZER_LIBRARY} ${QT_QTCORE_LIBRARY})

# adjust the name so the module is recognized as a strigi plugin
set_target_properties(po PROPERTIES
    PREFIX strigila_)

# install the module in the right directory so it is picked up
install(TARGETS po LIBRARY DESTINATION ${LIB_INSTALL_DIR}/strigi)


install(FILES strigi_translation.fieldproperties
	DESTINATION share/strigi/fieldproperties
)
