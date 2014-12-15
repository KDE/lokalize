#include "lokalizesubwindowbase.h"
#include "project.h"
#include <QKeySequence>

KActionCollection::KActionCollection(QMainWindow* w)
    : m_mainWindow(w)
    , file(m_mainWindow->menuBar()->addMenu(QApplication::translate("QMenuBar", "File")))
    , edit(m_mainWindow->menuBar()->addMenu(QApplication::translate("QMenuBar", "Edit")))
    , sync(m_mainWindow->menuBar()->addMenu(QApplication::translate("QMenuBar", "Sync")))
    , tm(m_mainWindow->menuBar()->addMenu(QApplication::translate("QMenuBar", "Translation Memory")))
{
    QAction* a=file->addAction(QApplication::translate("QMenuBar", "Open..."), Project::instance(),SLOT(fileOpen()));
    a->setShortcut(QKeySequence::Open);

    a=file->addAction(QApplication::translate("QMenuBar", "Close"), m_mainWindow,SLOT(close()));
    a->setShortcut(QKeySequence::Close);

    file->addSeparator();
}

QAction* KActionCollection::addAction(const QString& name, QAction* a)
{
    if (name.startsWith("file_")) file->addAction(a);
    if (name.startsWith("edit_")) edit->addAction(a);
    if (name.startsWith("merge_")) sync->addAction(a);
    if (name.startsWith("tmquery_")) tm->addAction(a);
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
        case KStandardAction::SaveAs: name=QApplication::translate("QMenuBar","Save As..."); m=c->file; k-QKeySequence::SaveAs; break;
        default:;
    }
    if (m)
    {
        QAction* a=m->addAction(name, rcv, slot);
        if ((int)k) a->setShortcut(k);
        return a;
    }

    QAction* a=new QAction(name, rcv);
    QObject::connect(a, SIGNAL(triggered(bool)), rcv, slot);
    return a;
}
