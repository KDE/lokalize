#-------------------------------------------------
#
# Project created by QtCreator 2012-06-06T01:26:21
#
#-------------------------------------------------

QT       += core widgets xml sql

TARGET = lokalize
TEMPLATE = app


SOURCES += main.cpp\
##    lokalizemainwindow.cpp
#    actionproxy.cpp
    editortab.cpp\
#    editortab_findreplace.cpp
    editorview.cpp\
    xlifftextedit.cpp\
##    syntaxhighlighter.cpp
##    completionstorage.cpp
#    phaseswindow.cpp
    noteeditor.cpp\
    msgctxtview.cpp\
    binunitsview.cpp\
    cataloglistview/cataloglistview.cpp\
    cataloglistview/catalogmodel.cpp\
    common/domroutines.cpp\
    common/fastsizehintitemdelegate.cpp\
    common/flowlayout.cpp\
    common/termlabel.cpp\
##    common/languagelistmodel.cpp
##    common/stemming.cpp
    glossary/glossaryview.cpp\
    glossary/glossary.cpp\
    glossary/glossarywindow.cpp\
    mergemode/mergecatalog.cpp\
    mergemode/mergeview.cpp\
    alttransview.cpp\
    common/diff.cpp\
    project/project.cpp\
#    project/projectmodel.cpp
#    project/projectwidget.cpp
#    project/projecttab.cpp
#    project/poextractor.cpp
##    prefs/prefs.cpp
#    webquery/webqueryview.cpp
#    webquery/webquerycontroller.cpp
#    webquery/myactioncollectionview.cpp
#    tools/widgettextcaptureconfig.cpp
##    filesearch/filesearchtab.cpp
    tm/tmview.cpp\
    tm/tmscanapi.cpp\
    tm/jobs.cpp\
    tm/dbfilesmodel.cpp\
    tm/tmmanager.cpp\
    tm/tmtab.cpp\
    tm/qaview.cpp\
    tm/qamodel.cpp\
    catalog/phase.cpp\
    catalog/cmd.cpp\
    catalog/pos.cpp\
    catalog/catalog.cpp\
    catalog/catalogstring.cpp\
    catalog/gettextheader.cpp\
    catalog/gettext/gettextstorage.cpp\
    catalog/gettext/catalogitem.cpp\
    catalog/gettext/importplugin.cpp\
    catalog/gettext/gettextimport.cpp\
    catalog/gettext/gettextexport.cpp\
    catalog/xliff/xliffstorage.cpp\
    catalog/ts/tsstorage.cpp

HEADERS  += editortab.h\
        tm/jobs.h

FORMS    +=    glossary/termedit.ui\
    tm/queryoptions.ui\
    tm/managedatabases.ui\
    tm/dbparams.ui

INCLUDEPATH += catalog cataloglistview mergemode glossary tm project nokde-stubs


mac: INCLUDEPATH += ../taglib ../taglib/taglib ../taglib/taglib/toolkit ../taglib/taglib/mpeg/id3v2 ../taglib/build
mac: LIBS += -L../taglib/build/taglib

unix: LIBS += -lhunspell
unix: DEFINES += NOKDE

win32: LIBS += ../taglib/build/taglib/tag.lib
win32: INCLUDEPATH += ../taglib ../taglib/taglib ../taglib/taglib/mpeg/id3v2 ../taglib/taglib/toolkit ../taglib/build
#win32: INCLUDEPATH += ../taglib/include ../taglib/build


 
