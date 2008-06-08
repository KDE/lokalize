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
// #include "diff.h"
#include "catalog.h"
#include "cmd.h"
#include "project.h"
#include "prefs_lokalize.h"
#include "dbfilesmodel.h"

#include <klocale.h>
#include <kdebug.h>
#include <threadweaver/ThreadWeaver.h>
#include <ktextbrowser.h>
#include <kglobalsettings.h>
#include <kpassivepopup.h>

#include <QTime>
#include <QDragEnterEvent>
#include <QFileInfo>
#include <QDir>
#include <QSignalMapper>
#include <QAction>
#include <QTimer>

#ifdef NDEBUG
#undef NDEBUG
#endif
#define DEBUG


TMView::TMView(QWidget* parent, Catalog* catalog, const QVector<QAction*>& actions)
    : QDockWidget ( i18nc("@title:window","Translation Memory"), parent)
    , m_browser(new KTextBrowser(this))
    , m_catalog(catalog)
    , m_currentSelectJob(0)
    , m_actions(actions)
    , m_normTitle(i18nc("@title:window","Translation Memory"))
    , m_hasInfoTitle(m_normTitle+" [*]")
    , m_hasInfo(false)
    , m_isBatching(false)
    , m_markAsFuzzy(false)
{
    setObjectName("TMView");
    setWidget(m_browser);

    QTimer::singleShot(0,this,SLOT(initLater()));
    connect(m_catalog,SIGNAL(signalFileLoaded(KUrl)),
            this,SLOT(slotFileLoaded(KUrl)));
}

TMView::~TMView()
{
    int i=m_jobs.size();
    while (--i>=0)
        ThreadWeaver::Weaver::instance()->dequeue(m_jobs.takeLast());


}

void TMView::initLater()
{
    QTime time;time.start();

    m_browser->viewport()->setBackgroundRole(QPalette::Background);
    setAcceptDrops(true);

    QSignalMapper* signalMapper=new QSignalMapper(this);
    int i=m_actions.size();
    while(--i>=0)
    {
        connect(m_actions.at(i),SIGNAL(triggered()),signalMapper,SLOT(map()));
        signalMapper->setMapping(m_actions.at(i), i);
    }

    connect(signalMapper, SIGNAL(mapped(int)),
             this, SLOT(slotUseSuggestion(int)));
    connect(m_browser,SIGNAL(selectionChanged()),this,SLOT(slotSelectionChanged()));

    m_browser->setToolTip(i18nc("@info:tooltip","Double-click any word to insert it into translation"));

    DBFilesModel::instance();

    m_browser->document()->setDefaultStyleSheet("p.close_match { font-weight:bold; }");

    //TODO ? kdisplayPaletteChanged
//     connect(KGlobalSettings::self(),,SIGNAL(kdisplayPaletteChanged()),this,SLOT(slotPaletteChanged()));

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
        //kWarning() << " " <<;
    };
}

void TMView::dropEvent(QDropEvent *event)
{
    bool ok=false;
    Project* p=Project::instance();
    const QString& pID=p->projectID();
    int i=event->mimeData()->urls().size();
    while(--i>=0)
    {
        if (event->mimeData()->urls().at(i).isEmpty()
            || event->mimeData()->urls().at(i).path().isEmpty() ) //NOTE is it qt bug?
            continue;
        if (event->mimeData()->urls().at(i).path().endsWith(".po"))
        {
            ScanJob* job=new ScanJob(KUrl(event->mimeData()->urls().at(i)),pID);
            connect(job,SIGNAL(failed(ThreadWeaver::Job*)),p,SLOT(deleteScanJob(ThreadWeaver::Job*)));
            connect(job,SIGNAL(done(ThreadWeaver::Job*)),p,SLOT(deleteScanJob(ThreadWeaver::Job*)));
            ThreadWeaver::Weaver::instance()->enqueue(job);
            ok=true;
        }
        else
        {
            ok=scanRecursive(QDir(event->mimeData()->urls().at(i).path()),
                            pID)||ok;
                //kWarning()<<"dd "<<dir.entryList();
        }
    }
    if (ok)
    {
        //dummy job for the finish indication
        ScanFinishedJob* job=new ScanFinishedJob(this);
        connect(job,SIGNAL(failed(ThreadWeaver::Job*)),p,SLOT(deleteScanJob(ThreadWeaver::Job*)));
        connect(job,SIGNAL(done(ThreadWeaver::Job*)),p,SLOT(deleteScanJob(ThreadWeaver::Job*)));
        ThreadWeaver::Weaver::instance()->enqueue(job);

        event->acceptProposedAction();
    }

}

#include <unistd.h>
void TMView::slotFileLoaded(const KUrl& url)
{
    Project* p=Project::instance();
    const QString& pID=p->projectID();

    if (Settings::scanToTMOnOpen())
    {
        ScanJob* job=new ScanJob(url,pID);
        connect(job,SIGNAL(failed(ThreadWeaver::Job*)),job,SLOT(deleteLater()));
        connect(job,SIGNAL(done(ThreadWeaver::Job*)),job,SLOT(deleteLater()));
        ThreadWeaver::Weaver::instance()->enqueue(job);
    }

    if (!Settings::prefetchTM()
        &&!m_isBatching)
        return;

    m_cache.clear();
    int i=m_jobs.size();
    while (--i>=0)
        ThreadWeaver::Weaver::instance()->dequeue(m_jobs.takeLast());

    DocPosition pos;
    while(switchNext(m_catalog,pos))
    {
        if (!m_catalog->isUntranslated(pos.entry)
           &&!m_catalog->isFuzzy(pos.entry))
            continue;
        SelectJob* j=new SelectJob(m_catalog->msgid(pos),
                                   m_catalog->msgctxt(pos.entry),
                                   pos,
                                   pID);
        //these two are for cleanup
        connect(j,SIGNAL(failed(ThreadWeaver::Job*)),p,SLOT(deleteScanJob(ThreadWeaver::Job*)));
        connect(j,SIGNAL(done(ThreadWeaver::Job*)),p,SLOT(dispatchSelectJob(ThreadWeaver::Job*)));

        connect(j,SIGNAL(done(ThreadWeaver::Job*)),
                this,SLOT(slotCacheSuggestions(ThreadWeaver::Job*)));

        ThreadWeaver::Weaver::instance()->enqueue(j);
        m_jobs.append(j);
    }

    //dummy job for the finish indication
    BatchSelectFinishedJob* m_seq=new BatchSelectFinishedJob(this);
    connect(m_seq,SIGNAL(failed(ThreadWeaver::Job*)),p,SLOT(deleteScanJob(ThreadWeaver::Job*)));
    connect(m_seq,SIGNAL(done(ThreadWeaver::Job*)),p,SLOT(deleteScanJob(ThreadWeaver::Job*)));
    connect(m_seq,SIGNAL(done(ThreadWeaver::Job*)),
            this,SLOT(slotBatchSelectDone(ThreadWeaver::Job*)));
    ThreadWeaver::Weaver::instance()->enqueue(m_seq);
    m_jobs.append(m_seq);
}

void TMView::slotCacheSuggestions(ThreadWeaver::Job* j)
{
    m_jobs.removeAll(j);
    SelectJob* job=static_cast<SelectJob*>(j);
    kDebug()<<job->m_pos.entry;
    if (job->m_pos.entry==m_pos.entry)
        slotSuggestionsCame(j);

    m_cache[DocPos(job->m_pos)]=job->m_entries.toVector();
}

void TMView::slotBatchSelectDone(ThreadWeaver::Job* /*j*/)
{
    m_jobs.clear();
    if (!m_isBatching)
        return;

    bool insHappened=false;
    DocPosition pos;
    while(switchNext(m_catalog,pos))
    {
        if (!(m_catalog->isUntranslated(pos.entry)
             ||m_catalog->isFuzzy(pos.entry))
           )
            continue;
        const QVector<TMEntry>& suggList=m_cache.value(DocPos(pos));
        if (suggList.isEmpty())
            continue;
        const TMEntry& entry=suggList.first();
        if (entry.score<9900)//kinda hack
            continue;
        {
            bool forceFuzzy=(suggList.size()>1&&suggList.at(1).score>=10000)
                            ||entry.score<10000;
            bool ctxtMatches=entry.score==1001;
            if (m_catalog->isFuzzy(pos.entry))
            {
                m_catalog->push(new DelTextCmd(m_catalog,pos,m_catalog->msgstr(pos)));
                if ( ctxtMatches || !(m_markAsFuzzy||forceFuzzy) )
                    m_catalog->push(new ToggleFuzzyCmd(m_catalog,pos.entry,false));
            }
            else if ((m_markAsFuzzy&&!ctxtMatches)||forceFuzzy)
            {
                m_catalog->push(new ToggleFuzzyCmd(m_catalog,pos.entry,true));
            }
            m_catalog->push(new InsTextCmd(m_catalog,pos,entry.target));

            if (KDE_ISUNLIKELY( m_pos.entry==pos.entry&&pos.form==m_pos.form ))
                emit refreshRequested();

        }
        if (!insHappened)
        {
            insHappened=true;
            m_catalog->beginMacro(i18nc("@item Undo action","Batch translation memory filling"));
        }


    }
    QString msg=i18nc("@info","Batch translation has been completed.");
    if (insHappened)
        m_catalog->endMacro();
    else
    {
        // xgettext: no-c-format
        msg+=' ';
        msg+=i18nc("@info","No suggestions with exact matches were found.");
    }

    KPassivePopup::message(KPassivePopup::Balloon,
                           i18nc("@title","Batch translation complete"),
                           msg,
                           this);
}

void TMView::slotBatchTranslate()
{
    m_isBatching=true;
    m_markAsFuzzy=false;
    if (!Settings::prefetchTM())
        slotFileLoaded(m_catalog->url());
    else if (m_jobs.isEmpty())
        return slotBatchSelectDone(0);
    KPassivePopup::message(KPassivePopup::Balloon,
                           i18nc("@title","Batch translation"),
                           i18nc("@info","Batch translation has been scheduled."),
                           this);

}

void TMView::slotBatchTranslateFuzzy()
{
    m_isBatching=true;
    m_markAsFuzzy=true;
    if (!Settings::prefetchTM())
        slotFileLoaded(m_catalog->url());
    else if (m_jobs.isEmpty())
        slotBatchSelectDone(0);
    KPassivePopup::message(KPassivePopup::Balloon,
                           i18nc("@title","Batch translation"),
                           i18nc("@info","Batch translation has been scheduled."),
                           this);

}

void TMView::slotNewEntryDisplayed(const DocPosition& pos)
{
    QTime time;time.start();

    if (m_catalog->numberOfEntries()<=pos.entry)
        return;//because of Qt::QueuedConnection


    ThreadWeaver::Weaver::instance()->dequeue(m_currentSelectJob);

    //update DB
    m_catalog->flushUpdateDBBuffer();

    m_pos=pos;
    m_browser->clear();
    if (Settings::prefetchTM()
        &&m_cache.contains(DocPos(pos)))
    {
        QTimer::singleShot(0,this,SLOT(displayFromCache()));
    }
    m_currentSelectJob=new SelectJob(m_catalog->msgid(pos),
                                     m_catalog->msgctxt(pos.entry),
                                     pos,
                                     Project::instance()->projectID());
    //these two are for cleanup
    connect(m_currentSelectJob,SIGNAL(failed(ThreadWeaver::Job*)),Project::instance(),SLOT(deleteScanJob(ThreadWeaver::Job*)));
    connect(m_currentSelectJob,SIGNAL(done(ThreadWeaver::Job*)),Project::instance(),SLOT(dispatchSelectJob(ThreadWeaver::Job*)));

    connect(m_currentSelectJob,SIGNAL(done(ThreadWeaver::Job*)),
            this,SLOT(slotSuggestionsCame(ThreadWeaver::Job*)));

    ThreadWeaver::Weaver::instance()->enqueue(m_currentSelectJob);
    kWarning()<<"ELA2 "<<time.elapsed();

}

void TMView::displayFromCache()
{
    if (m_prevCachePos.entry==m_pos.entry
        &&m_prevCachePos.form==m_pos.form)
        return;
    SelectJob* temp=new SelectJob(m_catalog->msgid(m_pos),
                                  m_catalog->msgctxt(m_pos.entry),
                                  m_pos,
                                  Project::instance()->projectID());
    temp->m_entries=m_cache.value(DocPos(m_pos)).toList();
    slotSuggestionsCame(temp);
    temp->deleteLater();
    m_prevCachePos=m_pos;
}

void TMView::slotSuggestionsCame(ThreadWeaver::Job* j)
{
    QTime time;time.start();

    SelectJob& job=*(static_cast<SelectJob*>(j));
    if (job.m_pos.entry!=m_pos.entry)
        return;

    Catalog& catalog=*m_catalog;
    if (catalog.numberOfEntries()<=m_pos.entry)
        return;//because of Qt::QueuedConnection


    //BEGIN query other DBs handling
    Project* p=Project::instance();
    const QString& pID=p->projectID();
    //check if this is an additional query, from secondary DBs
    if (job.m_dbName!=pID)
    {
        job.m_entries+=m_entries;
        qSort(job.m_entries);
        int limit=qMin(Settings::suggCount(),job.m_entries.size());
        int i=job.m_entries.size();
        while(--i>=limit)
            job.m_entries.removeLast();
    }
    else if (job.m_entries.isEmpty()||job.m_entries.first().score<8500)
    {
        DBFilesModel& model=*(DBFilesModel::instance());
        int i=model.rowCount();
        kWarning()<<"query other DBs"<<i;
        while (--i>=0)
        {
            const QString& db=model.data(model.index(i,0)).toString();
            if (pID!=db)
            {
                SelectJob* j=new SelectJob(catalog.msgid(m_pos),
                                           catalog.msgctxt(m_pos.entry),
                                           m_pos,db);
                //these two are for cleanup
                connect(j,SIGNAL(failed(ThreadWeaver::Job*)),p,SLOT(deleteScanJob(ThreadWeaver::Job*)));
                connect(j,SIGNAL(done(ThreadWeaver::Job*)),p,SLOT(dispatchSelectJob(ThreadWeaver::Job*)));

                connect(j,SIGNAL(done(ThreadWeaver::Job*)),
                    this,SLOT(slotSuggestionsCame(ThreadWeaver::Job*)));

                ThreadWeaver::Weaver::instance()->enqueue(j);
                m_jobs.append(j);
            }
        }
    }
    //END query other DBs handling

    m_entries=job.m_entries;

    int limit=job.m_entries.size();
    int i=0;

    if (!limit)
    {
        if (m_hasInfo)
        {
            m_hasInfo=false;
            setWindowTitle(m_normTitle);
        }
        return;
    }
    if (!m_hasInfo)
    {
        m_hasInfo=true;
        setWindowTitle(m_hasInfoTitle);
    }

    setUpdatesEnabled(false);
    m_browser->clear();

    //m_entries=job.m_entries;
    m_browser->insertHtml("<html>");

    QTextBlockFormat blockFormatBase;
    QTextBlockFormat blockFormatAlternate;
    //QPalette defaultPalette;
    blockFormatAlternate.setBackground(QPalette().alternateBase());
    //while (i<limit)
    forever
    {
        QTextCursor cur=m_browser->textCursor();
        QString html;
        html.reserve(1024);

        html+=(job.m_entries.at(i).score>9500)?"<p class='close_match'>":"<p>";

        html+=QString("/%1%/ ").arg(float(job.m_entries.at(i).score)/100);

        QString result(job.m_entries.at(i).diff);
        result.replace("\\n","\\n<br>");
        html+=result;

        QString str(job.m_entries.at(i).target);
        str.replace('<',"&lt;");
        str.replace('>',"&gt;");
        //str.replace('&',"&amp;"); TODO check
        html+="<br>";
        if (KDE_ISLIKELY( i<m_actions.size() ))
        {
            m_actions.at(i)->setStatusTip(job.m_entries.at(i).target);
            html+=QString("[%1] ").arg(m_actions.at(i)->shortcut().toString());
        }
        else
            html+="[ - ] ";
        html+=str;

        html+=i?"<br></p>":"</p>";
        cur.insertHtml(html);

        if (KDE_ISUNLIKELY( ++i>=limit ))
            break;

        cur.insertBlock(i%2?blockFormatAlternate:blockFormatBase);

    }
    m_browser->insertHtml("</html>");
    setUpdatesEnabled(true);
    kWarning()<<"ELA "<<time.elapsed()<<"BLOCK COUNT "<<m_browser->document()->blockCount();
}


/*
void TMView::slotPaletteChanged()
{
    
}*/



void TMView::slotSelectionChanged()
{
    //NOTE works fine only for dbl-click word selection
    //(actually, quick word insertion is exactly the purpose of this slot:)
    QString sel(m_browser->textCursor().selectedText());
    if (!(sel.isEmpty()||sel.contains(' ')))
        emit textInsertRequested(sel);
}

//TODO thorough testing
void TMView::slotUseSuggestion(int i)
{
    if (KDE_ISUNLIKELY( i>=m_entries.size() ))
        return;
    //this tries some black magic
    //naturally, there are many assumptions that might not always be true

    QString diff(m_entries.at(i).diff);
    QString target(m_entries.at(i).target);
    QString english(m_entries.at(i).english);

    QRegExp rxAdd("<font style=\"background-color:"+Settings::addColor().name()+"\">(.*)</font>");
    QRegExp rxDel("<font style=\"background-color:"+Settings::delColor().name()+"\">(.*)</font>");
    rxAdd.setMinimal(true);
    rxDel.setMinimal(true);

    //first things first
    int pos=0;
    while ((pos=rxDel.indexIn(diff,pos))!=-1)
        diff.replace(pos,rxDel.matchedLength(),"\tKAIDERDEL\t"+rxDel.cap(1)+"\t/KAIDERDEL\t");
    pos=0;
    while ((pos=rxAdd.indexIn(diff,pos))!=-1)
        diff.replace(pos,rxAdd.matchedLength(),"\tKAIDERADD\t"+rxAdd.cap(1)+"\t/KAIDERADD\t");

    diff.replace("&lt;","<");
    diff.replace("&gt;",">");

    //kWarning()<<diff;

    QString diffClean;diffClean.reserve(diff.size());
    QString old;old.reserve(diff.size());
    QByteArray diffM;diffM.reserve(diff.size());//kinda formatting info
    QVector<int> oldM;oldM.reserve(diff.size());//map old string-->diffClean

    /*
      0 - common
      + - add
      - - del
      M - modified

      so the string is like 00000MM00+++---000
    */
    QChar sep('\t');
    char state='0';
    pos=-1;
    while (++pos<diff.size())
    {
        if (diff.at(pos)==sep)
        {
            if (diff.indexOf("\tKAIDERDEL\t",pos)==pos)
            {
                state='-';
                pos+=10;
            }
            else if (diff.indexOf("\tKAIDERADD\t",pos)==pos)
            {
                state='+';
                pos+=10;
            }
            else if (diff.indexOf("\t/KAIDER",pos)==pos)
            {
                state='0';
                pos+=11;
            }
        }
        else
        {
            if (state!='+')
            {
                old.append(diff.at(pos));
                oldM.append(diffM.count());
            }
            diffM.append(state);
            diffClean.append(diff.at(pos));
        }
    }

    //kWarning()<<diff;
    //kWarning()<<diffM;
    //kWarning()<<old;



    bool sameMarkup=Project::instance()->markup()==m_entries.at(i).markup&&!m_entries.at(i).markup.isEmpty();
    //search for changed markup
    if (sameMarkup)
    {
        QRegExp rxMarkup(m_entries.at(i).markup);
        rxMarkup.setMinimal(true);
        pos=0;
        int replacingPos=0;
        while ((pos=rxMarkup.indexIn(old,pos))!=-1)
        {
            //kWarning()<<"size"<<oldM.size()<<pos<<pos+rxMarkup.matchedLength();
            QByteArray diffMPart(diffM.mid(oldM.at(pos),
                                           oldM.at(pos+rxMarkup.matchedLength()-1)+1-oldM.at(pos)));
            //kWarning()<<"diffMPart"<<diffMPart;
            if (diffMPart.contains('-')
                ||diffMPart.contains('+'))
            {
                //form newMarkup
                QString newMarkup;
                newMarkup.reserve(diffMPart.size());
                int j=-1;
                while(++j<diffMPart.size())
                {
                    if (diffMPart.at(j)!='-')
                        newMarkup.append(diffClean.at(oldM.at(pos)+j));
                }

                //replace first ocurrence
                int tmp=target.indexOf(rxMarkup.cap(0),replacingPos);
                if (tmp!=-1)
                {
                    target.replace(tmp,
                                rxMarkup.cap(0).size(),
                                newMarkup);
                    replacingPos=tmp;
                    //kWarning()<<"old"<<rxMarkup.cap(0)<<"new"<<newMarkup;

                    //avoid trying this part again
                    tmp=oldM.at(pos+rxMarkup.matchedLength()-1);
                    while(--tmp>=oldM.at(pos))
                        diffM[tmp]='M';
                    //kWarning()<<"M"<<diffM;
                }
            }

            pos+=rxMarkup.matchedLength();
        }
    }

    //del, add only markup, punct, num
    //TODO further improvement: spaces, punct marked as 0
    QRegExp rxNonTranslatable;
    if (sameMarkup)
        rxNonTranslatable.setPattern("^(("+m_entries.at(i).markup+")|(\\W|\\d)+)+");
    else
        rxNonTranslatable.setPattern("^(\\W|\\d)+");

    //kWarning()<<"("+m_entries.at(i).markup+"|(\\W|\\d)+";


    //handle the beginning
    int len=diffM.indexOf('0');
    if (len>0)
    {
        QByteArray diffMPart(diffM.left(len));
        int m=diffMPart.indexOf('M');
        if (m!=-1)
            diffMPart.truncate(m);

#if 0
nono
        //first goes del, then add. so stop on second del sequence
        bool seenAdd=false;
        int j=-1;
        while(++j<diffMPart.size())
        {
            if (diffMPart.at(j)=='+')
                seenAdd=true;
            else if (seenAdd && diffMPart.at(j)=='-')
            {
                diffMPart.truncate(j);
                break;
            }
        }
#endif
        //form 'oldMarkup'
        QString oldMarkup;
        oldMarkup.reserve(diffMPart.size());
        int j=-1;
        while(++j<diffMPart.size())
        {
            if (diffMPart.at(j)!='+')
                oldMarkup.append(diffClean.at(j));
        }

        //kWarning()<<"old"<<oldMarkup;
        rxNonTranslatable.indexIn(oldMarkup);
        oldMarkup=rxNonTranslatable.cap(0);
        if (target.startsWith(oldMarkup))
        {

            //form 'newMarkup'
            QString newMarkup;
            newMarkup.reserve(diffMPart.size());
            j=-1;
            while(++j<diffMPart.size())
            {
                if (diffMPart.at(j)!='-')
                    newMarkup.append(diffClean.at(j));
            }
            //kWarning()<<"new"<<newMarkup;
            rxNonTranslatable.indexIn(newMarkup);
            newMarkup=rxNonTranslatable.cap(0);

            //replace
            target.remove(0,oldMarkup.size());
            target.prepend(newMarkup);

            //avoid trying this part again
            j=diffMPart.size();
            while(--j>=0)
                diffM[j]='M';
            //kWarning()<<"M"<<diffM;
        }

    }

    if (sameMarkup)
        rxNonTranslatable.setPattern("(("+m_entries.at(i).markup+")|(\\W|\\d)+)+$");
    else
        rxNonTranslatable.setPattern("(\\W|\\d)+$");

    //handle the end
    if (!diffM.endsWith('0'))
    {
        len=diffM.lastIndexOf('0')+1;
        QByteArray diffMPart(diffM.mid(len));
        int m=diffMPart.lastIndexOf('M');
        if (m!=-1)
        {
            len=m+1;
            diffMPart=diffMPart.mid(len);
        }

        //form 'oldMarkup'
        QString oldMarkup;
        oldMarkup.reserve(diffMPart.size());
        int j=-1;
        while(++j<diffMPart.size())
        {
            if (diffMPart.at(j)!='+')
                oldMarkup.append(diffClean.at(len+j));
        }
        //kWarning()<<"old-"<<oldMarkup;
        rxNonTranslatable.indexIn(oldMarkup);
        oldMarkup=rxNonTranslatable.cap(0);
        if (target.endsWith(oldMarkup))
        {

            //form newMarkup
            QString newMarkup;
            newMarkup.reserve(diffMPart.size());
            j=-1;
            while(++j<diffMPart.size())
            {
                if (diffMPart.at(j)!='-')
                    newMarkup.append(diffClean.at(len+j));
            }
            //kWarning()<<"new"<<newMarkup;
            rxNonTranslatable.indexIn(newMarkup);
            newMarkup=rxNonTranslatable.cap(0);

            //replace
            target.chop(oldMarkup.size());
            target.append(newMarkup);

            //avoid trying this part again
            j=diffMPart.size();
            while(--j>=0)
                diffM[len+j]='M';
            //kWarning()<<"M"<<diffM;
        }
    }

    //search for numbers and stuff
    QRegExp rxNum("[\\d\\.\\%]+");
    pos=0;
    int replacingPos=0;
    while ((pos=rxNum.indexIn(old,pos))!=-1)
    {
        //save these so we can use rxNum in a body
        QString cap(rxNum.cap(0));
        int endPos1=pos+rxNum.matchedLength()-1;
        int endPos=oldM.at(endPos1);
        QByteArray diffMPart(diffM.mid(oldM.at(pos),
                                       endPos+1-oldM.at(pos)));
        while ((++endPos<diffM.size())
                  &&(diffM.at(endPos)=='+')
                  &&QString(diffClean.at(endPos)).contains(rxNum)
              )
            diffMPart.append('+');

        if ((diffMPart.contains('-')
            ||diffMPart.contains('+'))
            &&(!diffMPart.contains('M')))
        {
            //form newMarkup
            QString newMarkup;
            newMarkup.reserve(diffMPart.size());
            int j=-1;
            while(++j<diffMPart.size())
            {
                if (diffMPart.at(j)!='-')
                    newMarkup.append(diffClean.at(oldM.at(pos)+j));
            }
            //kWarning()<<"old"<<cap<<"new"<<newMarkup;

            //replace first ocurrence
            int tmp=target.indexOf(cap,replacingPos);
            if (tmp!=-1)
            {
                target.replace(tmp,
                            cap.size(),
                            newMarkup);
                replacingPos=tmp;

                //avoid trying this part again
                tmp=oldM.at(endPos1)+1;
                while(--tmp>=oldM.at(pos))
                    diffM[tmp]='M';
                //kWarning()<<"M"<<diffM;
            }
        }

        pos=endPos1+1;
    }









    if (KDE_ISUNLIKELY( target==m_catalog->msgstr(m_pos) || target.isEmpty() ))
        return;

    m_catalog->beginMacro(i18nc("@item Undo action","Use translation memory suggestion"));

    if (!m_catalog->msgstr(m_pos).isEmpty())
    {
        m_pos.offset=0;
        m_catalog->push(new DelTextCmd(m_catalog,m_pos,m_catalog->msgstr(m_pos)));
    }

    m_catalog->push(new InsTextCmd(m_catalog,m_pos,target),true);

//     emit textReplaceRequested(target/*m_actions.at(i)->statusTip()*/);
    m_catalog->endMacro();

    emit refreshRequested();
}


