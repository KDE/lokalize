PROJECT(kaider)
INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}/prefs
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
    ${CMAKE_CURRENT_SOURCE_DIR}/catalog
    ${CMAKE_CURRENT_SOURCE_DIR}/cataloglistview
    ${CMAKE_CURRENT_SOURCE_DIR}/project
    ${CMAKE_CURRENT_SOURCE_DIR}/glossary
    ${CMAKE_CURRENT_SOURCE_DIR}/mergemode
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

    cataloglistview/cataloglistview.cpp
    cataloglistview/catalogmodel.cpp

    ../common/flowlayout.cpp
    ../common/termlabel.cpp
    glossary/glossaryview.cpp
    glossary/tbxparser.cpp

    mergemode/mergecatalog.cpp
    mergemode/mergeview.cpp
    msgctxtview.cpp
    msgiddiffview.cpp
    ../common/diff.cpp
    project/projectview.cpp
    project/project.cpp
    project/projectmodel.cpp

    catalog/cmd.cpp
    catalog/catalog.cpp
    catalog/catalogitem.cpp
    catalog/importplugin.cpp
    catalog/exportplugin.cpp
    catalog/gettextimport.cpp
    catalog/gettextexport.cpp
)

kde4_automoc(${kaider_SRCS})

kde4_add_ui_files(kaider_SRCS
    prefs/prefs_identity.ui
    prefs/prefs_font.ui
    project/prefs_projectmain.ui
    glossary/termdialog.ui
    kaider_findextension.ui
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


