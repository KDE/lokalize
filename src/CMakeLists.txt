project(lokalize)

if(NOT WIN32)
    find_package(HUNSPELL REQUIRED)
else()
    find_package(HUNSPELL)
endif()

if(HUNSPELL_FOUND)
    add_definitions(-DHAVE_HUNSPELL)
    include_directories(
    ${HUNSPELL_INCLUDE_DIRS}
    )
endif()

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/prefs
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/catalog
    ${CMAKE_CURRENT_SOURCE_DIR}/catalog/gettext
    ${CMAKE_CURRENT_SOURCE_DIR}/catalog/xliff
    ${CMAKE_CURRENT_SOURCE_DIR}/catalog/ts
    ${CMAKE_CURRENT_SOURCE_DIR}/cataloglistview
    ${CMAKE_CURRENT_SOURCE_DIR}/project
    ${CMAKE_CURRENT_SOURCE_DIR}/glossary
    ${CMAKE_CURRENT_SOURCE_DIR}/tm
    ${CMAKE_CURRENT_SOURCE_DIR}/filesearch
    ${CMAKE_CURRENT_SOURCE_DIR}/mergemode
    ${CMAKE_CURRENT_SOURCE_DIR}/languagetool
)

configure_file(version.h.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/version.h)

set(liblokalize_SRCS
    main.cpp
    lokalizemainwindow.cpp
    lokalizetabpagebase.cpp
    statusbar.cpp

    editortab.cpp
    editortab_findreplace.cpp
    resizewatcher.cpp

    editorview.cpp
    xlifftextedit.cpp
    syntaxhighlighter.cpp

    completionstorage.cpp

    phaseswindow.cpp
    noteeditor.cpp
    msgctxtview.cpp

    binunitsview.cpp

    cataloglistview/cataloglistview.cpp
    cataloglistview/catalogmodel.cpp

    common/headerviewmenu.cpp
    common/domroutines.cpp
    common/htmlhelpers.cpp
    common/fastsizehintitemdelegate.cpp
    common/flowlayout.cpp
    common/termlabel.cpp
    common/languagelistmodel.cpp
    common/stemming.cpp
    glossary/glossaryview.cpp
    glossary/glossary.cpp
    glossary/glossarywindow.cpp

    metadata/filemetadata.cpp

    mergemode/mergecatalog.cpp
    mergemode/mergeview.cpp
    alttransview.cpp
    common/diff.cpp
    project/project.cpp
    project/projectmodel.cpp
    project/projectwidget.cpp
    project/projecttab.cpp
    project/updatestatsjob.cpp
    metadata/poextractor.cpp
    metadata/xliffextractor.cpp

    prefs/prefs.cpp

    languagetool/languagetoolresultjob.cpp
    languagetool/languagetoolmanager.cpp
    languagetool/languagetoolparser.cpp
    languagetool/languagetoolgrammarerror.cpp

    tools/widgettextcaptureconfig.cpp

    filesearch/filesearchtab.cpp

    tm/tmview.cpp
    tm/tmscanapi.cpp
    tm/jobs.cpp
    tm/dbfilesmodel.cpp
    tm/tmmanager.cpp
    tm/tmtab.cpp
    tm/qaview.cpp
    tm/qamodel.cpp

    catalog/phase.cpp

    catalog/cmd.cpp
    catalog/pos.cpp
    catalog/catalog.cpp
    catalog/catalogstring.cpp
    catalog/gettextheader.cpp
    catalog/gettextheaderparser.cpp
    catalog/gettext/gettextstorage.cpp
    catalog/gettext/catalogitem.cpp
    catalog/gettext/importplugin.cpp
    catalog/gettext/gettextimport.cpp
    catalog/gettext/gettextexport.cpp
    catalog/xliff/xliffstorage.cpp
    catalog/ts/tsstorage.cpp

    lokalizemainwindow.h
    statusbar.h

    editortab.h
    resizewatcher.h

    editorview.h
    xlifftextedit.h
    syntaxhighlighter.h

    completionstorage.h

    phaseswindow.h
    noteeditor.h
    msgctxtview.h

    binunitsview.h

    cataloglistview/cataloglistview.h
    cataloglistview/catalogmodel.h

    common/headerviewmenu.h
    common/domroutines.h
    common/fastsizehintitemdelegate.h
    common/flowlayout.h
    common/termlabel.h
    common/languagelistmodel.h
    common/stemming.h
    glossary/glossaryview.h
    glossary/glossary.h
    glossary/glossarywindow.h

    metadata/filemetadata.h

    mergemode/mergecatalog.h
    mergemode/mergeview.h
    alttransview.h
    common/diff.h
    project/project.h
    project/projectmodel.h
    project/projectwidget.h
    project/projecttab.h
    project/updatestatsjob.h
    metadata/poextractor.h
    metadata/xliffextractor.h

    prefs/prefs.h

    languagetool/languagetoolresultjob.h
    languagetool/languagetoolmanager.h
    languagetool/languagetoolparser.h
    languagetool/languagetoolgrammarerror.h

    tools/widgettextcaptureconfig.h

    filesearch/filesearchtab.h

    tm/tmview.h
    tm/tmscanapi.h
    tm/jobs.h
    tm/dbfilesmodel.h
    tm/tmmanager.h
    tm/tmtab.h
    tm/qaview.h
    tm/qamodel.h

    catalog/phase.h

    catalog/cmd.h
    catalog/pos.h
    catalog/catalog.h
    catalog/catalogstring.h
    catalog/gettextheader.h
    catalog/gettextheaderparser.h
    catalog/gettext/gettextstorage.h
    catalog/gettext/catalogitem.h
    catalog/gettext/gettextimport.h
    catalog/gettext/gettextexport.h
    catalog/xliff/xliffstorage.h
    catalog/ts/tsstorage.h
)

if(WIN32)
    list (APPEND liblokalize_SRCS common/winhelpers.cpp)
else()
    list (APPEND liblokalize_SRCS common/unixhelpers.cpp)
endif()

ecm_qt_declare_logging_category(liblokalize_SRCS
    HEADER lokalize_debug.h
    IDENTIFIER LOKALIZE_LOG
    CATEGORY_NAME org.kde.lokalize
    DEFAULT_SEVERITY Warning
    DESCRIPTION "Lokalize (lokalize)"
    EXPORT LOKALIZE
)

ki18n_wrap_ui(liblokalize_SRCS
    prefs/prefs_identity.ui
    prefs/prefs_general.ui
    prefs/prefs_editor.ui
    prefs/prefs_appearance.ui
    prefs/prefs_tm.ui
    prefs/prefs_pology.ui
    prefs/prefs_languagetool.ui
    project/prefs_project_advanced.ui
    project/prefs_project_local.ui
    project/prefs_projectmain.ui
    glossary/termedit.ui
    filesearch/filesearchoptions.ui
    filesearch/massreplaceoptions.ui
    tm/queryoptions.ui
    tm/managedatabases.ui
    tm/dbparams.ui
    kaider_findextension.ui
    tools/widgettextcaptureconfig.ui
)

kconfig_add_kcfg_files(liblokalize_SRCS
    prefs/prefs_lokalize.kcfgc
    project/projectbase.kcfgc
    project/projectlocal.kcfgc
)

if(USE_DBUS)
    qt_add_dbus_adaptor(liblokalize_SRCS org.kde.lokalize.MainWindow.xml
                        lokalizemainwindow.h LokalizeMainWindow)
    qt_add_dbus_adaptor(liblokalize_SRCS org.kde.lokalize.Editor.xml
                        editortab.h EditorTab)
    qt_add_dbus_adaptor(liblokalize_SRCS filesearch/org.kde.lokalize.FileSearch.xml
                        filesearch/filesearchtab.h FileSearchTab)
    qt_add_dbus_adaptor(liblokalize_SRCS tm/org.kde.lokalize.TranslationMemory.xml
                        tm/tmtab.h TM::TMTab)
    qt_add_dbus_adaptor(liblokalize_SRCS project/org.kde.lokalize.Project.xml
                        project/project.h Project)
    qt_add_dbus_adaptor(liblokalize_SRCS project/org.kde.lokalize.ProjectOverview.xml
                        project/projecttab.h ProjectTab)
endif()

### Build intermediate library (will be used by unit tests) ###
add_library(liblokalize STATIC ${liblokalize_SRCS})
target_link_libraries(liblokalize
    KF6::I18n
    KF6::TextWidgets
    KF6::KIOFileWidgets
    KF6::ItemViews
    KF6::Notifications
    KF6::SonnetCore
    KF6::SonnetUi
    KF6::Crash
    KF6::XmlGui
    KF6::IconThemes
    KF6::ConfigWidgets
    KF6::ColorScheme
    Qt::Sql
)
if(USE_DBUS)
target_link_libraries(liblokalize
    KF6::DBusAddons
)
endif()
if(HUNSPELL_FOUND)
    target_link_libraries(liblokalize ${HUNSPELL_LIBRARIES})
endif()


### Build Lokalize executable ###
set(lokalize_SRCS
    main.cpp

    lokalize.qrc
)

file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/../icons/global/*-apps-lokalize.png")
ecm_add_app_icon(lokalize_SRCS ICONS ${ICONS_SRCS})

add_executable(lokalize ${lokalize_SRCS})
target_link_libraries(lokalize liblokalize)

install(TARGETS lokalize ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

if(APPLE)
    set_target_properties(lokalize PROPERTIES
        MACOSX_BUNDLE_DISPLAY_NAME "Lokalize"
        MACOSX_BUNDLE_BUNDLE_NAME "Lokalize"
        MACOSX_BUNDLE_LONG_VERSION_STRING "Lokalize ${RELEASE_SERVICE_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${RELEASE_SERVICE_VERSION}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.lokalize"
        MACOSX_BUNDLE_COPYRIGHT "1999-2025 The Lokalize Developers")
endif()

########### install files ###############

install( PROGRAMS org.kde.lokalize.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )
install( FILES prefs/lokalize.kcfg  DESTINATION  ${KDE_INSTALL_KCFGDIR} )
install( FILES lokalize.notifyrc DESTINATION  ${KDE_INSTALL_KNOTIFYRCDIR} )

ecm_qt_install_logging_categories(
        EXPORT LOKALIZE
        FILE lokalize.categories
        DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
        )

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
