/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2009 by Nick Shaforostoff <shafff@ukr.net>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of
  the License or (at your option) version 3 or any later version
  accepted by the membership of KDE e.V. (or its successor approved
  by the membership of KDE e.V.), which shall act as a proxy
  defined in Section 14 of version 3 of the license.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

**************************************************************************** */

#include "tmview.h"

#include "jobs.h"
#include "tmscanapi.h"
#include "catalog.h"
#include "cmd.h"
#include "project.h"
#include "prefs_lokalize.h"
#include "dbfilesmodel.h"
#include "diff.h"
#include "xlifftextedit.h"

#include <klocale.h>
#include <kdebug.h>
#include <threadweaver/ThreadWeaver.h>
#include <ktextbrowser.h>
#include <kglobalsettings.h>
#include <kpassivepopup.h>
#include <kaction.h>
#include <kmessagebox.h>

#include <QTime>
#include <QDragEnterEvent>
#include <QFileInfo>
#include <QDir>
#include <QSignalMapper>
#include <QTimer>
#include <QToolTip>
#include <QMenu>

#ifdef NDEBUG
#undef NDEBUG
#endif
#define DEBUG
using namespace TM;


struct DiffInfo
{
    DiffInfo(int reserveSize);

    QString diffClean;
    QString old;
    //Formatting info:
    QByteArray diffIndex;
    //Map old string-->d.diffClean
    QVector<int> old2DiffClean;
};

DiffInfo::DiffInfo(int reserveSize)
{
    diffClean.reserve(reserveSize);
    old.reserve(reserveSize);
    diffIndex.reserve(reserveSize);
    old2DiffClean.reserve(reserveSize);
}



/**
 * 0 - common
 + - add
 - - del
 M - modified

 so the string is like 00000MM00+++---000
 (M appears afterwards)
*/
static DiffInfo getDiffInfo(const QString& diff)
{
    DiffInfo d(diff.size());

    QChar sep('{');
    char state='0';
    //walk through diff string char-by-char
    //calculate old and others
    int pos=-1;
    while (++pos<diff.size())
    {
        if (diff.at(pos)==sep)
        {
            if (diff.indexOf("{KBABELDEL}",pos)==pos)
            {
                state='-';
                pos+=10;
            }
            else if (diff.indexOf("{KBABELADD}",pos)==pos)
            {
                state='+';
                pos+=10;
            }
            else if (diff.indexOf("{/KBABEL",pos)==pos)
            {
                state='0';
                pos+=11;
            }
        }
        else
        {
            if (state!='+')
            {
                d.old.append(diff.at(pos));
                d.old2DiffClean.append(d.diffIndex.count());
            }
            d.diffIndex.append(state);
            d.diffClean.append(diff.at(pos));
        }
    }
    return d;
}


void TextBrowser::mouseDoubleClickEvent(QMouseEvent* event)
{
    KTextBrowser::mouseDoubleClickEvent(event);

    QString sel=textCursor().selectedText();
    if (!(sel.isEmpty()||sel.contains(' ')))
        emit textInsertRequested(sel);
}


TMView::TMView(QWidget* parent, Catalog* catalog, const QVector<KAction*>& actions)
    : QDockWidget ( i18nc("@title:window","Translation Memory"), parent)
    , m_browser(new TextBrowser(this))
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

    m_browser->document()->setDefaultStyleSheet("p.close_match { font-weight:bold; }");
    m_browser->viewport()->setBackgroundRole(QPalette::Background);

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
    setAcceptDrops(true);

    QSignalMapper* signalMapper=new QSignalMapper(this);
    int i=m_actions.size();
    while(--i>=0)
    {
        connect(m_actions.at(i),SIGNAL(triggered()),signalMapper,SLOT(map()));
        signalMapper->setMapping(m_actions.at(i), i);
    }
    connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(slotUseSuggestion(int)));

    setToolTip(i18nc("@info:tooltip","Double-click any word to insert it into translation"));

    DBFilesModel::instance();

    connect(m_browser,SIGNAL(textInsertRequested(QString)),this,SIGNAL(textInsertRequested(QString)));
    connect(m_browser,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(contextMenu(QPoint)));
    //TODO ? kdisplayPaletteChanged
//     connect(KGlobalSettings::self(),,SIGNAL(kdisplayPaletteChanged()),this,SLOT(slotPaletteChanged()));

}

void TMView::dragEnterEvent(QDragEnterEvent* event)
{
    if (dragIsAcceptable(event->mimeData()->urls()))
        event->acceptProposedAction();
}

void TMView::dropEvent(QDropEvent *event)
{
    if (scanRecursive(event->mimeData()->urls(),Project::instance()->projectID()))
        event->acceptProposedAction();
}

void TMView::slotFileLoaded(const KUrl& url)
{
    const QString& pID=Project::instance()->projectID();

    if (Settings::scanToTMOnOpen())
    {
        ScanJob* job=new ScanJob(url,pID);
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
        if (!m_catalog->isEmpty(pos.entry)
           &&m_catalog->isApproved(pos.entry))
            continue;
        SelectJob* j=initSelectJob(m_catalog, pos, pID);
        connect(j,SIGNAL(done(ThreadWeaver::Job*)),this,SLOT(slotCacheSuggestions(ThreadWeaver::Job*)));
        m_jobs.append(j);
    }

    //dummy job for the finish indication
    BatchSelectFinishedJob* m_seq=new BatchSelectFinishedJob(this);
    connect(m_seq,SIGNAL(done(ThreadWeaver::Job*)),m_seq,SLOT(deleteLater()));
    connect(m_seq,SIGNAL(done(ThreadWeaver::Job*)),this,SLOT(slotBatchSelectDone(ThreadWeaver::Job*)));
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
        if (!(m_catalog->isEmpty(pos.entry)
             ||!m_catalog->isApproved(pos.entry))
           )
            continue;
        const QVector<TMEntry>& suggList=m_cache.value(DocPos(pos));
        if (suggList.isEmpty())
            continue;
        const TMEntry& entry=suggList.first();
        if (entry.score<9900)//hacky
            continue;
        {
            bool forceFuzzy=(suggList.size()>1&&suggList.at(1).score>=10000)
                            ||entry.score<10000;
            bool ctxtMatches=entry.score==1001;
            if (!m_catalog->isApproved(pos.entry))
            {
                ///m_catalog->push(new DelTextCmd(m_catalog,pos,m_catalog->msgstr(pos)));
                removeTargetSubstring(m_catalog, pos, 0, m_catalog->targetWithTags(pos).string.size());
                if ( ctxtMatches || !(m_markAsFuzzy||forceFuzzy) )
                    SetStateCmd::push(m_catalog,pos,true);
            }
            else if ((m_markAsFuzzy&&!ctxtMatches)||forceFuzzy)
            {
                SetStateCmd::push(m_catalog,pos,false);
            }
            ///m_catalog->push(new InsTextCmd(m_catalog,pos,entry.target));
            insertCatalogString(m_catalog, pos, entry.target, 0);

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
    if (m_catalog->numberOfEntries()<=pos.entry)
        return;//because of Qt::QueuedConnection

    ThreadWeaver::Weaver::instance()->dequeue(m_currentSelectJob);

    //update DB
    //m_catalog->flushUpdateDBBuffer();
    //this is called via subscribtion

    if (pos.entry!=-1)
        m_pos=pos;
    m_browser->clear();
    if (Settings::prefetchTM()
        &&m_cache.contains(DocPos(m_pos)))
    {
        QTimer::singleShot(0,this,SLOT(displayFromCache()));
    }
    m_currentSelectJob=initSelectJob(m_catalog, m_pos);
    connect(m_currentSelectJob,SIGNAL(done(ThreadWeaver::Job*)),this,SLOT(slotSuggestionsCame(ThreadWeaver::Job*)));
}

void TMView::displayFromCache()
{
    if (m_prevCachePos.entry==m_pos.entry
        &&m_prevCachePos.form==m_pos.form)
        return;
    SelectJob* temp=initSelectJob(m_catalog, m_pos, QString(), 0);
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
    Project* project=Project::instance();
    const QString& projectID=project->projectID();
    //check if this is an additional query, from secondary DBs
    if (job.m_dbName!=projectID)
    {
        job.m_entries+=m_entries;
        qSort(job.m_entries.begin(), job.m_entries.end(), qGreater<TMEntry>());
        int limit=qMin(Settings::suggCount(),job.m_entries.size());
        int i=job.m_entries.size();
        while(--i>=limit)
            job.m_entries.removeLast();
    }
    else if (job.m_entries.isEmpty()||job.m_entries.first().score<8500)
    {
        //be careful, as we switched to QDirModel!
        const DBFilesModel& dbFilesModel=*(DBFilesModel::instance());
        QModelIndex root=dbFilesModel.rootIndex();
        int i=dbFilesModel.rowCount(root);
        //kWarning()<<"query other DBs,"<<i<<"total";
        while (--i>=0)
        {
            const QString& dbName=dbFilesModel.data(dbFilesModel.index(i,0,root)).toString();
            if (projectID!=dbName && dbFilesModel.m_configurations.value(dbName).targetLangCode==catalog.targetLangCode())
            {
                SelectJob* j=initSelectJob(m_catalog, m_pos, dbName);
                connect(j,SIGNAL(done(ThreadWeaver::Job*)),this,SLOT(slotSuggestionsCame(ThreadWeaver::Job*)));
                m_jobs.append(j);
            }
        }
    }
    //END query other DBs handling

    m_entries=job.m_entries;

    int limit=job.m_entries.size();

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
    m_entryPositions.clear();

    //m_entries=job.m_entries;
    //m_browser->insertHtml("<html>");

    int i=0;
    QTextBlockFormat blockFormatBase;
    QTextBlockFormat blockFormatAlternate; blockFormatAlternate.setBackground(QPalette().alternateBase());
    QTextCharFormat noncloseMatchCharFormat;
    QTextCharFormat closeMatchCharFormat;  closeMatchCharFormat.setFontWeight(QFont::Bold);
    forever
    {
        QTextCursor cur=m_browser->textCursor();
        QString html;
        html.reserve(1024);

        const TMEntry& entry=job.m_entries.at(i);
        html+=(entry.score>9500)?"<p class='close_match'>":"<p>";
        //kDebug()<<entry.target.string<<entry.hits;

        html+=QString("/%1%/ ").arg(float(entry.score)/100);

        //int sourceStartPos=cur.position();
        QString result=Qt::escape(entry.diff);
        //result.replace("&","&amp;");
        //result.replace("<","&lt;");
        //result.replace(">","&gt;");
        result.replace("{KBABELADD}","<font style=\"background-color:"%Settings::addColor().name()%";color:black\">");
        result.replace("{/KBABELADD}","</font>");
        result.replace("{KBABELDEL}","<font style=\"background-color:"%Settings::delColor().name()%";color:black\">");
        result.replace("{/KBABELDEL}","</font>");
        result.replace("\\n","\\n<br>");
        result.replace("\\n","\\n<br>");
        html+=result;
#if 0
        cur.insertHtml(result);

        cur.movePosition(QTextCursor::PreviousCharacter,QTextCursor::MoveAnchor,cur.position()-sourceStartPos);
        CatalogString catStr(entry.diff);
        catStr.string.remove("{KBABELDEL}"); catStr.string.remove("{/KBABELDEL}");
        catStr.string.remove("{KBABELADD}"); catStr.string.remove("{/KBABELADD}");
        catStr.tags=entry.source.tags;
        DiffInfo d=getDiffInfo(entry.diff);
        int j=catStr.tags.size();
        while(--j>=0)
        {
            catStr.tags[j].start=d.old2DiffClean.at(catStr.tags.at(j).start);
            catStr.tags[j].end  =d.old2DiffClean.at(catStr.tags.at(j).end);
        }
        insertContent(cur,catStr,job.m_source,false);
#endif

        //str.replace('&',"&amp;"); TODO check
        html+="<br>";
        if (KDE_ISLIKELY( i<m_actions.size() ))
        {
            m_actions.at(i)->setStatusTip(entry.target.string);
            html+=QString("[%1] ").arg(m_actions.at(i)->shortcut().toString());
        }
        else
            html+="[ - ] ";
/*
        QString str(entry.target.string);
        str.replace('<',"&lt;");
        str.replace('>',"&gt;");
        html+=str;
*/
        cur.insertHtml(html); html.clear();
        cur.setCharFormat((entry.score>9500)?closeMatchCharFormat:noncloseMatchCharFormat);
        insertContent(cur,entry.target);
        m_entryPositions.insert(cur.anchor(),i);

        html+=i?"<br></p>":"</p>";
        cur.insertHtml(html);

        if (KDE_ISUNLIKELY( ++i>=limit ))
            break;

        cur.insertBlock(i%2?blockFormatAlternate:blockFormatBase);

    }
    m_browser->insertHtml("</html>");
    setUpdatesEnabled(true);
//    kWarning()<<"ELA "<<time.elapsed()<<"BLOCK COUNT "<<m_browser->document()->blockCount();
}


/*
void TMView::slotPaletteChanged()
{

}*/
bool TMView::event(QEvent *event)
{
    if (event->type()==QEvent::ToolTip)
    {
        QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
        int block1=m_browser->cursorForPosition(m_browser->viewport()->mapFromGlobal(helpEvent->globalPos())).blockNumber();
        int block=*m_entryPositions.lowerBound(m_browser->cursorForPosition(m_browser->viewport()->mapFromGlobal(helpEvent->globalPos())).anchor());
        if (block1!=block)
            kWarning()<<"block numbers don't match";
        if (block<m_entries.size())
        {
            const TMEntry& tmEntry=m_entries.at(block);
            QString file=tmEntry.file;
            if (file==m_catalog->url().toLocalFile())
                file=i18nc("File argument in tooltip, when file is current file", "this");
            QString tooltip=i18nc("@info:tooltip","File: %1<br />Addition date: %2",file, tmEntry.date.toString(Qt::ISODate));
            if (!tmEntry.changeDate.isNull() && tmEntry.changeDate!=tmEntry.date)
                tooltip+=i18nc("@info:tooltip on TM entry continues","<br />Last change date: %1", tmEntry.changeDate.toString(Qt::ISODate));
            if (!tmEntry.changeAuthor.isEmpty())
                tooltip+=i18nc("@info:tooltip on TM entry continues","<br />Last change author: %1", tmEntry.changeAuthor);
            tooltip+=i18nc("@info:tooltip on TM entry continues","<br />TM: %1", tmEntry.dbName);
            if (tmEntry.obsolete)
                tooltip+=i18nc("@info:tooltip on TM entry continues","<br />Is not present in the file anymore");
            QToolTip::showText(helpEvent->globalPos(),tooltip);
            return true;
        }
    }
    return QWidget::event(event);
}

void TMView::contextMenu(const QPoint& pos)
{
    int block=*m_entryPositions.lowerBound(m_browser->cursorForPosition(pos).anchor());
    kWarning()<<block;
    if (block>=m_entries.size())
        return;

    const TMEntry& e=m_entries.at(block);
    enum {Remove, Open};
    QMenu popup;
    popup.addAction(i18nc("@action:inmenu", "Remove this entry"))->setData(Remove);
    if (e.file!= m_catalog->url().toLocalFile() && QFile::exists(e.file))
        popup.addAction(i18nc("@action:inmenu", "Open file containing this entry"))->setData(Open);
    QAction* r=popup.exec(m_browser->mapToGlobal(pos));
    if (!r)
        return;
    if ((r->data().toInt()==Remove) &&
        KMessageBox::Yes==KMessageBox::questionYesNo(this, i18n("<html>Do you really want to remove this entry:<br/><i>%1</i><br/>from translation memory %2?</html>",  Qt::escape(e.target.string), e.dbName),
                                                           i18nc("@title:window","Translation Memory Entry Removal")))
    {
        RemoveJob* job=new RemoveJob(e);
        connect(job,SIGNAL(done(ThreadWeaver::Job*)),job,SLOT(deleteLater()));
        connect(job,SIGNAL(done(ThreadWeaver::Job*)),this,SLOT(slotNewEntryDisplayed()));
        ThreadWeaver::Weaver::instance()->enqueue(job);
    }
    else if (r->data().toInt()==Open)
        emit fileOpenRequested(e.file, e.source.string, e.ctxt);
}

/**
 * helper function:
 * searches to th nearest rxNum or ABBR
 * clears rxNum if ABBR is found before rxNum
 */
static int nextPlacableIn(const QString& old, int start, QString& cap)
{
    static QRegExp rxNum("[\\d\\.\\%]+");
    static QRegExp rxAbbr("\\w+");

    int numPos=rxNum.indexIn(old,start);
//    int abbrPos=rxAbbr.indexIn(old,start);
    int abbrPos=start;
    //kWarning()<<"seeing"<<old.size()<<old;
    while (((abbrPos=rxAbbr.indexIn(old,abbrPos))!=-1))
    {
        QString word=rxAbbr.cap(0);
        //check if tail contains uppoer case characters
        const QChar* c=word.unicode()+1;
        int i=word.size()-1;
        while (--i>=0)
        {
            if ((c++)->isUpper())
                break;
        }
        abbrPos+=rxAbbr.matchedLength();
    }

    int pos=qMin(numPos,abbrPos);
    if (pos==-1)
        pos=qMax(numPos,abbrPos);

//     if (pos==numPos)
//         cap=rxNum.cap(0);
//     else
//         cap=rxAbbr.cap(0);

    cap=(pos==numPos?rxNum:rxAbbr).cap(0);
    //kWarning()<<cap;

    return pos;
}





//TODO thorough testing

/**
 * this tries some black magic
 * naturally, there are many assumptions that might not always be true
 */
CatalogString TM::targetAdapted(const TMEntry& entry, const CatalogString& ref)
{
    kWarning()<<entry.source.string<<entry.target.string<<entry.diff;

    QString diff=entry.diff;
    CatalogString target=entry.target;
    //QString english=entry.english;


    QRegExp rxAdd("<font style=\"background-color:[^>]*" % Settings::addColor().name() % "[^>]*\">([^>]*)</font>");
    QRegExp rxDel("<font style=\"background-color:[^>]*" % Settings::delColor().name() % "[^>]*\">([^>]*)</font>");
    //rxAdd.setMinimal(true);
    //rxDel.setMinimal(true);

    //first things first
    int pos=0;
    while ((pos=rxDel.indexIn(diff,pos))!=-1)
        diff.replace(pos,rxDel.matchedLength(),"\tKBABELDEL\t" % rxDel.cap(1) % "\t/KBABELDEL\t");
    pos=0;
    while ((pos=rxAdd.indexIn(diff,pos))!=-1)
        diff.replace(pos,rxAdd.matchedLength(),"\tKBABELADD\t" % rxAdd.cap(1) % "\t/KBABELADD\t");

    diff.replace("&lt;","<");
    diff.replace("&gt;",">");

    //possible enhancement: search for non-translated words in removedSubstrings...
    //QStringList removedSubstrings;
    //QStringList addedSubstrings;


    /*
      0 - common
      + - add
      - - del
      M - modified

      so the string is like 00000MM00+++---000
    */
    DiffInfo d=getDiffInfo(diff);

    bool sameMarkup=Project::instance()->markup()==entry.markupExpr&&!entry.markupExpr.isEmpty();
    bool tryMarkup=entry.target.tags.size() && sameMarkup;
    //search for changed markup
    if (tryMarkup)
    {
        QRegExp rxMarkup(entry.markupExpr);
        rxMarkup.setMinimal(true);
        pos=0;
        int replacingPos=0;
        while ((pos=rxMarkup.indexIn(d.old,pos))!=-1)
        {
            //kWarning()<<"size"<<oldM.size()<<pos<<pos+rxMarkup.matchedLength();
            QByteArray diffIndexPart(d.diffIndex.mid(d.old2DiffClean.at(pos),
                                           d.old2DiffClean.at(pos+rxMarkup.matchedLength()-1)+1-d.old2DiffClean.at(pos)));
            //kWarning()<<"diffMPart"<<diffMPart;
            if (diffIndexPart.contains('-')
                ||diffIndexPart.contains('+'))
            {
                //form newMarkup
                QString newMarkup;
                newMarkup.reserve(diffIndexPart.size());
                int j=-1;
                while(++j<diffIndexPart.size())
                {
                    if (diffIndexPart.at(j)!='-')
                        newMarkup.append(d.diffClean.at(d.old2DiffClean.at(pos)+j));
                }

                //replace first ocurrence
                int tmp=target.string.indexOf(rxMarkup.cap(0),replacingPos);
                if (tmp!=-1)
                {
                    target.replace(tmp,
                                rxMarkup.cap(0).size(),
                                newMarkup);
                    replacingPos=tmp;
                    //kWarning()<<"d.old"<<rxMarkup.cap(0)<<"new"<<newMarkup;

                    //avoid trying this part again
                    tmp=d.old2DiffClean.at(pos+rxMarkup.matchedLength()-1);
                    while(--tmp>=d.old2DiffClean.at(pos))
                        d.diffIndex[tmp]='M';
                    //kWarning()<<"M"<<diffM;
                }
            }

            pos+=rxMarkup.matchedLength();
        }
    }

    //del, add only markup, punct, num
    //TODO further improvement: spaces, punct marked as 0
//BEGIN BEGIN HANDLING
    QRegExp rxNonTranslatable;
    if (tryMarkup)
        rxNonTranslatable.setPattern("^((" % entry.markupExpr % ")|(\\W|\\d)+)+");
    else
        rxNonTranslatable.setPattern("^(\\W|\\d)+");

    //kWarning()<<"("+entry.markup+"|(\\W|\\d)+";


    //handle the beginning
    int len=d.diffIndex.indexOf('0');
    if (len>0)
    {
        QByteArray diffMPart(d.diffIndex.left(len));
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
                oldMarkup.append(d.diffClean.at(j));
        }

        //kWarning()<<"old"<<oldMarkup;
        rxNonTranslatable.indexIn(oldMarkup);
        oldMarkup=rxNonTranslatable.cap(0);
        if (target.string.startsWith(oldMarkup))
        {

            //form 'newMarkup'
            QString newMarkup;
            newMarkup.reserve(diffMPart.size());
            j=-1;
            while(++j<diffMPart.size())
            {
                if (diffMPart.at(j)!='-')
                    newMarkup.append(d.diffClean.at(j));
            }
            //kWarning()<<"new"<<newMarkup;
            rxNonTranslatable.indexIn(newMarkup);
            newMarkup=rxNonTranslatable.cap(0);

            //replace
            kWarning()<<"BEGIN HANDLING. replacing"<<target.string.left(oldMarkup.size())<<"with"<<newMarkup;
            target.remove(0,oldMarkup.size());
            target.insert(0,newMarkup);

            //avoid trying this part again
            j=diffMPart.size();
            while(--j>=0)
                d.diffIndex[j]='M';
            //kWarning()<<"M"<<diffM;
        }

    }
//END BEGIN HANDLING
//BEGIN END HANDLING
    if (tryMarkup)
        rxNonTranslatable.setPattern("(("% entry.markupExpr %")|(\\W|\\d)+)+$");
    else
        rxNonTranslatable.setPattern("(\\W|\\d)+$");

    //handle the end
    if (!d.diffIndex.endsWith('0'))
    {
        len=d.diffIndex.lastIndexOf('0')+1;
        QByteArray diffMPart(d.diffIndex.mid(len));
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
                oldMarkup.append(d.diffClean.at(len+j));
        }
        //kWarning()<<"old-"<<oldMarkup;
        rxNonTranslatable.indexIn(oldMarkup);
        oldMarkup=rxNonTranslatable.cap(0);
        if (target.string.endsWith(oldMarkup))
        {

            //form newMarkup
            QString newMarkup;
            newMarkup.reserve(diffMPart.size());
            j=-1;
            while(++j<diffMPart.size())
            {
                if (diffMPart.at(j)!='-')
                    newMarkup.append(d.diffClean.at(len+j));
            }
            //kWarning()<<"new"<<newMarkup;
            rxNonTranslatable.indexIn(newMarkup);
            newMarkup=rxNonTranslatable.cap(0);

            //replace
            target.string.chop(oldMarkup.size());
            target.string.append(newMarkup);

            //avoid trying this part again
            j=diffMPart.size();
            while(--j>=0)
                d.diffIndex[len+j]='M';
            //kWarning()<<"M"<<diffM;
        }
    }
//END BEGIN HANDLING

    //search for numbers and stuff
    //QRegExp rxNum("[\\d\\.\\%]+");
    pos=0;
    int replacingPos=0;
    QString cap;
    QString _;
    //while ((pos=rxNum.indexIn(old,pos))!=-1)
    kWarning()<<"string:"<<target.string<<"searching for placeables in"<<d.old;
    while ((pos=nextPlacableIn(d.old,pos,cap))!=-1)
    {
        kWarning()<<"considering placable"<<cap;
        //save these so we can use rxNum in a body
        int endPos1=pos+cap.size()-1;
        int endPos=d.old2DiffClean.at(endPos1);
        int startPos=d.old2DiffClean.at(pos);
        QByteArray diffMPart=d.diffIndex.mid(startPos,
                                       endPos+1-startPos);

        kWarning()<<"starting diffMPart"<<diffMPart;

        //the following loop extends replacement text, e.g. for 1 -> 500 cases
        while ((++endPos<d.diffIndex.size())
                  &&(d.diffIndex.at(endPos)=='+')
                  &&(-1!=nextPlacableIn(QString(d.diffClean.at(endPos)),0,_))
              )
            diffMPart.append('+');

        kWarning()<<"diffMPart extended 1"<<diffMPart;
//         if ((pos-1>=0) && (d.old2DiffClean.at(pos)>=0))
//         {
//             kWarning()<<"d.diffIndex"<<d.diffIndex<<d.old2DiffClean.at(pos)-1;
//             kWarning()<<"(d.diffIndex.at(d.old2DiffClean.at(pos-1))=='+')"<<(d.diffIndex.at(d.old2DiffClean.at(pos-1))=='+');
//             //kWarning()<<(-1!=nextPlacableIn(QString(d.diffClean.at(d.old2DiffClean.at(pos))),0,_));
//         }

        //this is for the case when +'s preceed -'s:
        while ((--startPos>=0)
                  &&(d.diffIndex.at(startPos)=='+')
                  //&&(-1!=nextPlacableIn(QString(d.diffClean.at(d.old2DiffClean.at(pos))),0,_))
              )
            diffMPart.prepend('+');
        ++startPos;

        kWarning()<<"diffMPart extended 2"<<diffMPart;

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
                    newMarkup.append(d.diffClean.at(startPos+j));
            }
            if (newMarkup.endsWith(' ')) newMarkup.chop(1);
            //kWarning()<<"d.old"<<cap<<"new"<<newMarkup;


            //replace first ocurrence
            int tmp=target.string.indexOf(cap,replacingPos);
            if (tmp!=-1)
            {
                kWarning()<<"replacing"<<cap<<"with"<<newMarkup;
                target.replace(tmp, cap.size(), newMarkup);
                replacingPos=tmp;

                //avoid trying this part again
                tmp=d.old2DiffClean.at(endPos1)+1;
                while(--tmp>=d.old2DiffClean.at(pos))
                    d.diffIndex[tmp]='M';
                //kWarning()<<"M"<<diffM;
            }
            else
                kWarning()<<"newMarkup"<<newMarkup<<"wasn't used";
        }
        pos=endPos1+1;
    }
    adaptCatalogString(target, ref);
    return target;
}

void TMView::slotUseSuggestion(int i)
{
    if (KDE_ISUNLIKELY( i>=m_entries.size() ))
        return;

    CatalogString target=targetAdapted(m_entries.at(i), m_catalog->sourceWithTags(m_pos));

#if 0
    QString tmp=target.string;
    tmp.replace(TAGRANGE_IMAGE_SYMBOL, '*');
    kWarning()<<"targetAdapted"<<tmp;

    foreach (InlineTag tag, target.tags)
        kWarning()<<"tag"<<tag.start<<tag.end;
#endif
    if (KDE_ISUNLIKELY( target.isEmpty() ))
        return;

    m_catalog->beginMacro(i18nc("@item Undo action","Use translation memory suggestion"));

    QString old=m_catalog->targetWithTags(m_pos).string;
    if (!old.isEmpty())
    {
        m_pos.offset=0;
        //FIXME test!
        removeTargetSubstring(m_catalog, m_pos, 0, old.size());
        //m_catalog->push(new DelTextCmd(m_catalog,m_pos,m_catalog->msgstr(m_pos)));
    }
    kWarning()<<"1"<<target.string;

    //m_catalog->push(new InsTextCmd(m_catalog,m_pos,target)/*,true*/);
    insertCatalogString(m_catalog, m_pos, target, 0);

    if (m_entries.at(i).score>9900 && !m_catalog->isApproved(m_pos.entry))
        SetStateCmd::push(m_catalog,m_pos,true);

    m_catalog->endMacro();

    emit refreshRequested();
}


#include "tmview.moc"
