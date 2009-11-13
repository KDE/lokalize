/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2008 by Nick Shaforostoff <shafff@ukr.net>

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

#define KDE_NO_DEBUG_OUTPUT

#include "alttransview.h"
#include <QDragEnterEvent>

#include "diff.h"
#include "catalog.h"
#include "cmd.h"
#include "project.h"
#include "xlifftextedit.h"
#include "tmview.h" //TextBrowser
#include "mergecatalog.h"
#include "prefs_lokalize.h"

#include <klocale.h>
#include <kdebug.h>
#include <kaction.h>

#include <QSignalMapper>
//#include <QTime>
#include <QFileInfo>
#include <QToolTip>


AltTransView::AltTransView(QWidget* parent, Catalog* catalog,const QVector<KAction*>& actions)
    : QDockWidget ( i18nc("@title:window","Alternate Translations"), parent)
    , m_browser(new TM::TextBrowser(this))
    , m_catalog(catalog)
    , m_normTitle(i18nc("@title:window","Alternate Translations"))
    , m_hasInfoTitle(m_normTitle+" [*]")
    , m_hasInfo(false)
    , m_actions(actions)
{
    setObjectName("msgIdDiff");
    setWidget(m_browser);
    hide();

    m_browser->setReadOnly(true);
    m_browser->viewport()->setBackgroundRole(QPalette::Background);
    QTimer::singleShot(0,this,SLOT(initLater()));
}

void AltTransView::initLater()
{
    setAcceptDrops(true);

    KConfig config;
    KConfigGroup group(&config,"AltTransView");
    m_everShown=group.readEntry("EverShown",false);



    QSignalMapper* signalMapper=new QSignalMapper(this);
    int i=m_actions.size();
    while(--i>=0)
    {
        connect(m_actions.at(i),SIGNAL(triggered()),signalMapper,SLOT(map()));
        signalMapper->setMapping(m_actions.at(i), i);
    }
    connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(slotUseSuggestion(int)));

    setToolTip(i18nc("@info:tooltip","<p>Sometimes, if original text is changed, its translation becomes deprecated and is either marked as <emphasis>needing&nbsp;review</emphasis> (i.e. looses approval status), "
    "or (only in case of XLIFF file) moved to the <emphasis>alternate&nbsp;translations</emphasis> section accompanying the unit.</p>"
    "<p>This toolview also shows the difference between new original string and the old one, so that you can easily see which changes should be applied to existing translation.</p>"
    "<p>Double-click any word in this toolview to insert it into translation.</p>"
    "<p>Drop translation file onto this toolview to use it as a source for alternate translations.</p>"
    ));

    connect(m_browser,SIGNAL(textInsertRequested(QString)),this,SIGNAL(textInsertRequested(QString)));
    //connect(m_browser,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(contextMenu(QPoint)));
}

AltTransView::~AltTransView()
{
}

void AltTransView::dragEnterEvent(QDragEnterEvent* event)
{
    if(event->mimeData()->hasUrls() && Catalog::extIsSupported(event->mimeData()->urls().first().path()))
        event->acceptProposedAction();
}

void AltTransView::dropEvent(QDropEvent *event)
{
    event->acceptProposedAction();
    attachAltTransFile(event->mimeData()->urls().first().toLocalFile());

    //update
    m_prevEntry.entry=-1;
    QTimer::singleShot(0,this,SLOT(process()));
}

void AltTransView::attachAltTransFile(const QString& path)
{
    MergeCatalog* altCat=new MergeCatalog(m_catalog, m_catalog, /*saveChanges*/false);
    altCat->loadFromUrl(KUrl::fromLocalFile(path));
    m_catalog->attachAltTransCatalog(altCat);
}

void AltTransView::fileLoaded()
{
    m_prevEntry.entry=-1;
    QString absPath=m_catalog->url().toLocalFile();
    QString relPath=KUrl::relativePath(Project::instance()->projectDir(),absPath);
    
    QFileInfo info(Project::instance()->altTransDir()+'/'+relPath);
    if (info.canonicalFilePath()!=absPath && info.exists())
        attachAltTransFile(info.canonicalFilePath());
}

void AltTransView::slotNewEntryDisplayed(const DocPosition& pos)
{
    m_entry=DocPos(pos);
    QTimer::singleShot(0,this,SLOT(process()));
}

void AltTransView::process()
{
    if (m_entry==m_prevEntry) return;
    if (m_catalog->numberOfEntries()<=m_entry.entry)
        return;//because of Qt::QueuedConnection

    m_prevEntry=m_entry;
    m_browser->clear();
    m_entryPositions.clear();

    const QVector<AltTrans>& entries=m_catalog->altTrans(m_entry.toDocPosition());
    m_entries=entries;

    if (entries.isEmpty())
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


    CatalogString source=m_catalog->sourceWithTags(m_entry.toDocPosition());

    QTextBlockFormat blockFormatBase;
    QTextBlockFormat blockFormatAlternate; blockFormatAlternate.setBackground(QPalette().alternateBase());
    QTextCharFormat noncloseMatchCharFormat;
    QTextCharFormat closeMatchCharFormat;  closeMatchCharFormat.setFontWeight(QFont::Bold);
    int i=0;
    int limit=entries.size();
    forever
    {
        const AltTrans& entry=entries.at(i);

        QTextCursor cur=m_browser->textCursor();
        QString html;
        html.reserve(1024);
        if (!entry.source.isEmpty())
        {
            html+="<p>";

            QString result=Qt::escape(userVisibleWordDiff(entry.source.string, source.string,Project::instance()->accel(),Project::instance()->markup()));
            //result.replace("&","&amp;");
            //result.replace("<","&lt;");
            //result.replace(">","&gt;");
            result.replace("{KBABELADD}","<font style=\"background-color:"+Settings::addColor().name()+";color:black\">");
            result.replace("{/KBABELADD}","</font>");
            result.replace("{KBABELDEL}","<font style=\"background-color:"+Settings::delColor().name()+";color:black\">");
            result.replace("{/KBABELDEL}","</font>");
            result.replace("\\n","\\n<br>");
            result.replace("\\n","\\n<br>");

            html+=result;
            html+="<br>";
            cur.insertHtml(html); html.clear();
        }
        if (!entry.target.isEmpty())
        {
            if (KDE_ISLIKELY( i<m_actions.size() ))
            {
                m_actions.at(i)->setStatusTip(entry.target.string);
                html+=QString("[%1] ").arg(m_actions.at(i)->shortcut().toString());
            }
            else
                html+="[ - ] ";

            cur.insertText(html); html.clear();
            insertContent(cur,entry.target);
        }
        m_entryPositions.insert(cur.anchor(),i);

        html+=i?"<br></p>":"</p>";
        cur.insertHtml(html);

        if (KDE_ISUNLIKELY( ++i>=limit ))
            break;

        cur.insertBlock(i%2?blockFormatAlternate:blockFormatBase);
    }


    if (!m_everShown)
    {
        m_everShown=true;
        show();

        KConfig config;
        KConfigGroup group(&config,"AltTransView");
        group.writeEntry("EverShown",true);
    }
}


bool AltTransView::event(QEvent *event)
{
    if (event->type()==QEvent::ToolTip)
    {
        QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
        int block1=m_browser->cursorForPosition(m_browser->viewport()->mapFromGlobal(helpEvent->globalPos())).blockNumber();
        int block=*m_entryPositions.lowerBound(m_browser->cursorForPosition(m_browser->viewport()->mapFromGlobal(helpEvent->globalPos())).anchor());
        if (block1!=block)
            kWarning()<<"block numbers don't match";
        if (block>=m_entries.size())
            return false;
        QString origin=m_entries.at(block).origin;
        if (origin.isEmpty())
            return false;

        QString tooltip=i18nc("@info:tooltip","Origin: %1",origin);
        QToolTip::showText(helpEvent->globalPos(),tooltip);
        return true;
    }
    return QWidget::event(event);
}

void AltTransView::slotUseSuggestion(int i)
{
    if (KDE_ISUNLIKELY( i>=m_entries.size() ))
        return;

    TM::TMEntry tmEntry;
    tmEntry.target=m_entries.at(i).target;
    CatalogString source=m_catalog->sourceWithTags(m_entry.toDocPosition());
    tmEntry.diff=userVisibleWordDiff(m_entries.at(i).source.string, source.string,Project::instance()->accel(),Project::instance()->markup());

    CatalogString target=TM::targetAdapted(tmEntry, source);

    kWarning()<<"0"<<target.string;
    if (KDE_ISUNLIKELY( target.isEmpty() ))
        return;

    m_catalog->beginMacro(i18nc("@item Undo action","Use alternate translation"));

    QString old=m_catalog->targetWithTags(m_entry.toDocPosition()).string;
    if (!old.isEmpty())
    {
        //FIXME test!
        removeTargetSubstring(m_catalog, m_entry.toDocPosition(), 0, old.size());
        //m_catalog->push(new DelTextCmd(m_catalog,m_pos,m_catalog->msgstr(m_pos)));
    }
    kWarning()<<"1"<<target.string;

    //m_catalog->push(new InsTextCmd(m_catalog,m_pos,target)/*,true*/);
    insertCatalogString(m_catalog, m_entry.toDocPosition(), target, 0);

    m_catalog->endMacro();

    emit refreshRequested();
}


#include "alttransview.moc"
