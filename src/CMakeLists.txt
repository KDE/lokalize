project(lokalize)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/prefs
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/catalog
    ${CMAKE_CURRENT_SOURCE_DIR}/catalog/gettext
    ${CMAKE_CURRENT_SOURCE_DIR}/catalog/xliff
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
    lokalizemainwindow.cpp
    actionproxy.cpp

    kaider.cpp
    kaider_findreplace.cpp

    kaiderview.cpp
    syntaxhighlighter.cpp

#    kaiderdockwidget.cpp

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
    catalog/gettextheader.cpp
    catalog/gettext/gettextstorage.cpp
    catalog/gettext/catalogitem.cpp
    catalog/gettext/importplugin.cpp
    catalog/gettext/gettextimport.cpp
    catalog/gettext/gettextexport.cpp

    catalog/xliff/xliffstorage.cpp
)


kde4_add_ui_files(lokalize_SRCS
    prefs/prefs_identity.ui
    prefs/prefs_font.ui
    prefs/prefs_misc.ui
    project/prefs_regexps.ui
    project/prefs_projectmain.ui
    glossary/termedit.ui
    tm/queryoptions.ui
    tm/managedatabases.ui
    tm/dbparams.ui
    kaider_findextension.ui
    webquery/querycontrol.ui
)

kde4_add_kcfg_files(lokalize_SRCS
    prefs/prefs_lokalize.kcfgc
    project/projectbase.kcfgc
)

#qt4_add_dbus_interface(lokalize_SRCS org.kde.lokalize.MainWindow.xml lokainterface)
qt4_add_dbus_adaptor(lokalize_SRCS org.kde.lokalize.MainWindow.xml
                     lokalizemainwindow.h LokalizeMainWindow)

qt4_add_dbus_adaptor(lokalize_SRCS org.kde.lokalize.Editor.xml
                     kaider.h EditorWindow)

qt4_add_dbus_adaptor(lokalize_SRCS tm/org.kde.lokalize.TranslationMemory.xml
                     tm/tmwindow.h TM::TMWindow)

kde4_add_app_icon(lokalize_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/../icons/hi*-app-lokalize.png")

kde4_add_executable(lokalize ${lokalize_SRCS})

target_link_libraries(lokalize
    ${KDE4_KPARTS_LIBS}
    ${KDE4_KROSSCORE_LIBS}
    ${KDE4_KROSSUI_LIBS}
    ${KDE4_KIO_LIBS}
    ${KDE4_KFILE_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${KDE4_THREADWEAVER_LIBRARY}
    ${QT_QTSQL_LIBRARY}
)

install(TARGETS lokalize ${INSTALL_TARGETS_DEFAULT_ARGS} )


########### install files ###############

install( FILES lokalize.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
install( FILES prefs/lokalize.kcfg  DESTINATION  ${KCFG_INSTALL_DIR} )
install( FILES editorui.rc  DESTINATION  ${DATA_INSTALL_DIR}/lokalize )
install( FILES lokalizemainwindowui.rc  DESTINATION  ${DATA_INSTALL_DIR}/lokalize )
install( FILES project/projectmanagerui.rc  DESTINATION  ${DATA_INSTALL_DIR}/lokalize )
install( FILES tm/translationmemoryrui.rc  DESTINATION  ${DATA_INSTALL_DIR}/lokalize )
install( FILES scripts/find-gui-text.sh  DESTINATION  ${DATA_INSTALL_DIR}/lokalize )
install( FILES scripts/widget-text-capture.py  DESTINATION  ${DATA_INSTALL_DIR}/lokalize/scripts )
install( FILES scripts/widget-text-capture.rc  DESTINATION  ${DATA_INSTALL_DIR}/lokalize/scripts )
install( FILES scripts/widget-text-capture.ui  DESTINATION  ${DATA_INSTALL_DIR}/lokalize/scripts )

