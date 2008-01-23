PROJECT(lokalize)
INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}/prefs
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/catalog
    ${CMAKE_CURRENT_SOURCE_DIR}/cataloglistview
    ${CMAKE_CURRENT_SOURCE_DIR}/project
    ${CMAKE_CURRENT_SOURCE_DIR}/glossary
    ${CMAKE_CURRENT_SOURCE_DIR}/webquery
    ${CMAKE_CURRENT_SOURCE_DIR}/tm
    ${CMAKE_CURRENT_SOURCE_DIR}/mergemode
    ${KDE4_INCLUDE_DIR}
    ${QT_INCLUDES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(lokalize_SRCS
    main.cpp
    kaider.cpp
    kaider_findreplace.cpp

    kaiderview.cpp
    syntaxhighlighter.cpp

    cataloglistview/cataloglistview.cpp
    cataloglistview/catalogmodel.cpp

    common/flowlayout.cpp
    common/termlabel.cpp
    glossary/glossaryview.cpp
    glossary/tbxparser.cpp
    glossary/glossary.cpp
    glossary/glossarywindow.cpp

    mergemode/mergecatalog.cpp
    mergemode/mergeview.cpp
    msgctxtview.cpp
    msgiddiffview.cpp
    common/diff.cpp
    project/project.cpp
    project/projectmodel.cpp
    project/projectwidget.cpp
    project/projectview.cpp
    project/projectwindow.cpp

    prefs/prefs.cpp

    webquery/webqueryview.cpp
    webquery/webquerycontroller.cpp
    webquery/myactioncollectionview.cpp

    tm/tmview.cpp
    tm/jobs.cpp
    tm/tmwindow.cpp
    tm/dbfilesmodel.cpp
    tm/tmmanager.cpp

    catalog/cmd.cpp
    catalog/pos.cpp
    catalog/catalog.cpp
    catalog/catalogitem.cpp
    catalog/importplugin.cpp
    catalog/exportplugin.cpp
    catalog/gettextimport.cpp
    catalog/gettextexport.cpp
)


kde4_add_ui_files(lokalize_SRCS
    prefs/prefs_identity.ui
    prefs/prefs_font.ui
    prefs/prefs_misc.ui
    project/prefs_projectmain.ui
    project/prefs_regexps.ui
    glossary/termedit.ui
    tm/queryoptions.ui
    tm/managedatabases.ui
    tm/dbparams.ui
    kaider_findextension.ui
    webquery/querycontrol.ui
)

kde4_add_kcfg_files(lokalize_SRCS
    prefs/prefs_kaider.kcfgc
    project/projectbase.kcfgc
)

kde4_add_executable(lokalize ${lokalize_SRCS})

target_link_libraries(lokalize
#     ${KDE4_KFIlE_LIBS}
#     ${KDE4_KROSSCORE_LIBS}
    ${KDE4_KROSSUI_LIBS}
    -lkfile
    ${KDE4_THREADWEAVER_LIBRARY}
    ${QT_QTSQL_LIBRARY}
)

install(TARGETS lokalize DESTINATION ${BIN_INSTALL_DIR} )


########### install files ###############

install( FILES lokalize.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
install( FILES prefs/kaider.kcfg  DESTINATION  ${KCFG_INSTALL_DIR} )
install( FILES kaiderui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kaider )
install( FILES project/projectmanagerui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kaider )


