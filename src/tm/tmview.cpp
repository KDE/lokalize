/* ****************************************************************************
  This file is part of KAider

  Copyright (C) 2007 by Nick Shaforostoff <shafff@ukr.net>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

  In addition, as a special exception, the copyright holders give
  permission to link the code of this program with any edition of
  the Qt library by Trolltech AS, Norway (or with modified versions
  of Qt that use the same license as Qt), and distribute linked
  combinations including the two.  You must obey the GNU General
  Public License in all respects for all of the code used other than
  Qt. If you modify this file, you may extend this exception to
  your version of the file, but you are not obligated to do so.  If
  you do not wish to do so, delete this exception statement from
  your version.

**************************************************************************** */

#include "tmview.h"

#include "jobs.h"
#include "diff.h"
#include "catalog.h"
#include "project.h"
// #include "prefs_kaider.h"

#include <klocale.h>
#include <kdebug.h>
#include <threadweaver/ThreadWeaver.h>

#include <QTextBrowser>
#include <QTime>
#include <QDragEnterEvent>
#include <QFileInfo>
#include <QDir>
#include <QSignalMapper>
#include <QAction>

TMView::TMView(QWidget* parent, Catalog* catalog, const QVector<QAction*>& actions)
    : QDockWidget ( i18nc("@title:window","Translation Memory"), parent)
    , m_browser(new QTextBrowser(this))
    , m_catalog(catalog)
    , m_normTitle(i18nc("@title:window","Translation Memory"))
    , m_hasInfoTitle(m_normTitle+" [*]")
    , m_hasInfo(false)
    , m_actions(actions)
//     , m_currentSelectJob(0)
{
    setObjectName("TMView");
    setWidget(m_browser);
    setAcceptDrops(true);

    QSignalMapper* signalMapper=new QSignalMapper(this);
    int i=actions.size();
    while(--i>=0)
    {
        connect(actions.at(i),SIGNAL(triggered()),signalMapper,SLOT(map()));
        signalMapper->setMapping(actions.at(i), i);
    }

    connect(signalMapper, SIGNAL(mapped(int)),
             this, SLOT(slotUseSuggestion(int)));
}

TMView::~TMView()
{
    delete m_browser;
}


void TMView::slotUseSuggestion(int i)
{
    emit textReplaceRequested(m_actions.at(i)->statusTip());
}

void TMView::dragEnterEvent(QDragEnterEvent* event)
{
    if(event->mimeData()->hasUrls())
    {
        int i=event->mimeData()->urls().size();
        while(--i>=0)
        {
            if (event->mimeData()->urls().at(i).path().endsWith(".po"))
            {
                event->acceptProposedAction();
                return;
            }
            QFileInfo info(event->mimeData()->urls().at(i).path());
            if (info.exists() && info.isDir())
            {
                event->acceptProposedAction();
                return;
            }
        }
        //kWarning() << " " << <<endl;
    };
}

bool scanRecursive(const QDir& dir)
{
    bool ok=false;
    QStringList subDirs(dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot|QDir::Readable));
    int i=subDirs.size();
    while(--i>=0)
        ok=scanRecursive(QDir(dir.filePath(subDirs.at(i))))||ok;

    QStringList filters("*.po");
    QStringList files(dir.entryList(filters,QDir::Files|QDir::NoDotAndDotDot|QDir::Readable));
    i=files.size();
    while(--i>=0)
    {
        ScanJob* job=new ScanJob(KUrl(dir.filePath(files.at(i))));
        job->connect(job,SIGNAL(failed(ThreadWeaver::Job*)),Project::instance(),SLOT(deleteScanJob(ThreadWeaver::Job*)));
        job->connect(job,SIGNAL(done(ThreadWeaver::Job*)),Project::instance(),SLOT(deleteScanJob(ThreadWeaver::Job*)));
        ThreadWeaver::Weaver::instance()->enqueue(job);
        ok=true;
    }

    return ok;
}

void TMView::dropEvent(QDropEvent *event)
{
//     emit mergeOpenRequested(KUrl(event->mimeData()->urls().first()));
    bool ok=false;
    int i=event->mimeData()->urls().size();
    while(--i>=0)
    {
        if (event->mimeData()->urls().at(i).path().endsWith(".po"))
        {
            ScanJob* job=new ScanJob(KUrl(event->mimeData()->urls().at(i)));
            connect(job,SIGNAL(failed(ThreadWeaver::Job*)),Project::instance(),SLOT(deleteScanJob(ThreadWeaver::Job*)));
            connect(job,SIGNAL(done(ThreadWeaver::Job*)),Project::instance(),SLOT(deleteScanJob(ThreadWeaver::Job*)));
            ThreadWeaver::Weaver::instance()->enqueue(job);
            ok=true;
        }
        else
        {
            ok=scanRecursive(QDir(event->mimeData()->urls().at(i).path()))||ok;
                //kWarning()<<"dd "<<dir.entryList()<<endl;
        }
    }
    if (ok)
        event->acceptProposedAction();

}


void TMView::slotNewEntryDisplayed(const DocPosition& pos)
{
    m_browser->clear();
    m_pos=pos;
    SelectJob* m_currentSelectJob=new SelectJob(m_catalog->msgid(pos),this,pos);
    connect(m_currentSelectJob,SIGNAL(failed(ThreadWeaver::Job*)),Project::instance(),SLOT(deleteScanJob(ThreadWeaver::Job*)));
    //connecting job to singleton project because this window may be closed by the time suggestions arrive and we dont wanna leak
    connect(m_currentSelectJob,SIGNAL(done(ThreadWeaver::Job*)),Project::instance(),SLOT(dispatchSelectJob(ThreadWeaver::Job*)));
    ThreadWeaver::Weaver::instance()->enqueue(m_currentSelectJob);

}

void TMView::slotSuggestionsCame(SelectJob* job)
{
    QTime time;
    time.start();

    if (job->m_pos.entry!=m_pos.entry)
        return;

    int limit=job->m_entries.size();
    int i=0;

    if (!limit)
    {
        if (m_hasInfo)
        {
            m_hasInfo=false;
            setWindowTitle(m_normTitle);
            m_browser->clear();
        }
        return;
    }
    if (!m_hasInfo)
    {
        m_hasInfo=true;
        setWindowTitle(m_hasInfoTitle);
    }
    //m_entries=job->m_entries;
    m_browser->insertHtml("<html>");

    while (i<limit)
    {

//         kWarning()<<"res: "<<m_pos.entry<<endl;
        m_browser->insertHtml(QString("[%1%] ").arg(float(job->m_entries.at(i).score)/100));

        QString oldStr(job->m_entries.at(i).english);
        QString newStr(m_catalog->msgid(m_pos));


        kWarning()<<"res: "<<oldStr<<endl;
//         kWarning()<<"res: "<<newStr<<endl;
        QString result(wordDiff(oldStr,newStr));
        result.replace("\\n","\\n<br>");

//         kWarning()<<"res: "<<result<<endl;

        m_browser->insertHtml(result);



        QString str(job->m_entries.at(i).target);
        str.replace('<',"&lt;");
        str.replace('>',"&gt;");
        //str.replace('&',"&amp;"); TODO check

        //str.remove(QRegExp("<br> *$"));
        //m_browser->insertHtml(QString("<br><p style=\"margin-left:10px\">%1</p>").arg(str));
        m_browser->insertHtml("<br>");
        if (i<m_actions.size())
        {
            m_actions.at(i)->setStatusTip(job->m_entries.at(i).target);
            m_browser->insertHtml(QString("[%1] ").arg(m_actions.at(i)->shortcut().toString()));
        }
        else
            m_browser->insertHtml("[ - ] ");

        m_browser->insertHtml(QString("%1<br><br>").arg(str));
        ++i;
/*        if (i<limit)
            m_browser->insertHtml("<hr width=80%>");*/
    }
    m_browser->insertHtml("</html>");
//     kWarning()<<endl;
    kWarning()<<"ELA "<<time.elapsed()<<endl;
}




