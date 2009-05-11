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

    editortab.cpp
    editortab_findreplace.cpp

    editorview.cpp
    xlifftextedit.cpp
    syntaxhighlighter.cpp

    phaseswindow.cpp
    noteeditor.cpp
    msgctxtview.cpp

    binunitsview.cpp

    cataloglistview/cataloglistview.cpp
    cataloglistview/catalogmodel.cpp

    common/flowlayout.cpp
    common/termlabel.cpp
    common/languagelistmodel.cpp
    glossary/glossaryview.cpp
    glossary/tbxparser.cpp
    glossary/glossary.cpp
    glossary/glossarywindow.cpp

    mergemode/mergecatalog.cpp
    mergemode/mergeview.cpp
    alttransview.cpp
    common/diff.cpp
    project/project.cpp
    project/projectmodel.cpp
    project/projectwidget.cpp
    project/projecttab.cpp

    prefs/prefs.cpp

    webquery/webqueryview.cpp
    webquery/webquerycontroller.cpp
    webquery/myactioncollectionview.cpp

    tm/tmview.cpp
    tm/tmscanapi.cpp
    tm/jobs.cpp
    tm/dbfilesmodel.cpp
    tm/tmmanager.cpp
    tm/tmtab.cpp

    catalog/phase.cpp

    catalog/cmd.cpp
    catalog/pos.cpp
    catalog/catalog.cpp
    catalog/catalogstring.cpp
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
    prefs/prefs_editor.ui
    prefs/prefs_appearance.ui
    prefs/prefs_tm.ui
    project/prefs_project_advanced.ui
    project/prefs_project_local.ui
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
    project/projectlocal.kcfgc
)

#qt4_add_dbus_interface(lokalize_SRCS org.kde.lokalize.MainWindow.xml lokainterface)
qt4_add_dbus_adaptor(lokalize_SRCS org.kde.lokalize.MainWindow.xml
                     lokalizemainwindow.h LokalizeMainWindow)
qt4_add_dbus_adaptor(lokalize_SRCS org.kde.lokalize.Editor.xml
                     editortab.h EditorTab)
qt4_add_dbus_adaptor(lokalize_SRCS tm/org.kde.lokalize.TranslationMemory.xml
                     tm/tmtab.h TM::TMTab)
qt4_add_dbus_adaptor(lokalize_SRCS project/org.kde.lokalize.Project.xml
                     project/project.h Project)
qt4_add_dbus_adaptor(lokalize_SRCS project/org.kde.lokalize.ProjectOverview.xml
                     project/projecttab.h ProjectTab)



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
install( FILES
        editorui.rc
        lokalizemainwindowui.rc
        scriptsui.rc
        project/projectmanagerui.rc
        tm/translationmemoryrui.rc
        DESTINATION  ${DATA_INSTALL_DIR}/lokalize )
