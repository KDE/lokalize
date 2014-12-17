#include "lokalizesubwindowbase.h"
#include "project.h"
#include "kaboutdata.h"
#include <QKeySequence>
#include <QApplication>

KActionCollection::KActionCollection(QMainWindow* w)
    : m_mainWindow(w)
    , file(m_mainWindow->menuBar()->addMenu(QApplication::translate("QMenuBar", "File")))
    , edit(m_mainWindow->menuBar()->addMenu(QApplication::translate("QMenuBar", "Edit")))
    , view(m_mainWindow->menuBar()->addMenu(QApplication::translate("QMenuBar", "View")))
    , sync(m_mainWindow->menuBar()->addMenu(QApplication::translate("QMenuBar", "Sync")))
    , tm  (m_mainWindow->menuBar()->addMenu(QApplication::translate("QMenuBar", "Translation Memory")))
{
    QAction* a=file->addAction(QApplication::translate("QMenuBar", "Open..."), Project::instance(),SLOT(fileOpen()));
    a->setShortcut(QKeySequence::Open);

    a=file->addAction(QApplication::translate("QMenuBar", "Close"), m_mainWindow,SLOT(close()));
    a->setShortcut(QKeySequence::Close);

    QMenu* help=m_mainWindow->menuBar()->addMenu(QApplication::translate("QMenuBar", "Help"));
    a=help->addAction(QApplication::translate("QMenuBar", "About Lokalize"), KAboutData::instance,SLOT(doAbout()));
    a->setMenuRole(QAction::AboutRole);
    a=help->addAction(QApplication::translate("QMenuBar", "About Qt"), qApp,SLOT(aboutQt()));
    a->setMenuRole(QAction::AboutQtRole);
}

QAction* KActionCollection::addAction(const QString& name, QAction* a)
{
    if (name.startsWith("file_")) file->addAction(a);
    if (name.startsWith("edit_")) edit->addAction(a);
    if (name.startsWith("merge_")) sync->addAction(a);
    if (name.startsWith("tmquery_")) tm->addAction(a);
    if (name.startsWith("show")) view->addAction(a);
    return a;
}


QAction* KActionCategory::addAction(KStandardAction::StandardAction t, QObject* rcv, const char* slot)
{
    QString name=QStringLiteral("std");
    QMenu* m = 0;
    QKeySequence::StandardKey k=QKeySequence::UnknownKey;
    switch(t)
    {
        case KStandardAction::Save: name=QApplication::translate("QMenuBar","Save"); m=c->file; k=QKeySequence::Save; break;
        case KStandardAction::SaveAs: name=QApplication::translate("QMenuBar","Save As...");m=c->file;k-QKeySequence::SaveAs;break;
        default:;
    }
    if (m)
    {
        QAction* a=m->addAction(name, rcv, slot);
        if ((int)k) a->setShortcut(k);
        if (t==KStandardAction::SaveAs)
            c->file->addSeparator();
        return a;
    }

    QAction* a=new QAction(name, rcv);
    QObject::connect(a, SIGNAL(triggered(bool)), rcv, slot);
    return a;
}
