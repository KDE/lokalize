PROJECT(kaider)
INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}/project
    ${CMAKE_CURRENT_SOURCE_DIR}/mergemode
    ${CMAKE_CURRENT_SOURCE_DIR}/prefs
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
    ${KDE4_INCLUDE_DIR}
    ${QT_INCLUDES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(kaider_SRCS
    main.cpp
    kaider.cpp
    kaider_findreplace.cpp
    kaider_prefs.cpp
    kaider_mergemode.cpp

    kaiderview.cpp
    syntaxhighlighter.cpp

    mergemode/mergecatalog.cpp
    mergemode/mergeview.cpp
    msgctxtview.cpp
    msgiddiffview.cpp
    ../common/diff.cpp
    project/projectview.cpp
    project/project.cpp
    project/projectmodel.cpp

    ../common/cmd.cpp
    ../common/catalog.cpp
    ../common/catalogitem.cpp
    ../common/importplugin.cpp
    ../common/exportplugin.cpp
    ../common/gettextimport.cpp
    ../common/gettextexport.cpp
)

kde4_automoc(${kaider_SRCS})

kde4_add_ui_files(kaider_SRCS
    prefs/prefs_identity.ui
    prefs/prefs_font.ui
    project/prefs_projectmain.ui
    findExtension.ui
)

kde4_add_kcfg_files(kaider_SRCS
    prefs/prefs_kaider.kcfgc
    project/projectbase.kcfgc
)

kde4_add_executable(kaider ${kaider_SRCS})

target_link_libraries(kaider ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS} )

install(TARGETS kaider DESTINATION ${BIN_INSTALL_DIR} )


########### install files ###############

install( FILES kaider.desktop  DESTINATION  ${XDG_APPS_DIR} )
install( FILES prefs/kaider.kcfg  DESTINATION  ${KCFG_INSTALL_DIR} )
install( FILES kaiderui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kaider )


