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

#include "mergeview.h"

#include "cmd.h"
#include "mergecatalog.h"
#include "project.h"
#include "diff.h"
#include "projectmodel.h"

#include <klocale.h>
#include <kdebug.h>
#include <kurl.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <knotification.h>
#include <ktextedit.h>
#include <kaction.h>
#include <threadweaver/ThreadWeaver.h>

#include <QDragEnterEvent>
#include <QFile>
#include <QToolTip>

MergeView::MergeView(QWidget* parent, Catalog* catalog, bool primary)
    : QDockWidget ( primary?i18nc("@title:window that displays difference between current file and 'merge source'","Primary Sync"):i18nc("@title:window that displays difference between current file and 'merge source'","Secondary Sync"), parent)
    , m_browser(new KTextEdit(this))
    , m_baseCatalog(catalog)
    , m_mergeCatalog(0)
    , m_normTitle(primary?
                          i18nc("@title:window that displays difference between current file and 'merge source'","Primary Sync"):
                          i18nc("@title:window that displays difference between current file and 'merge source'","Secondary Sync"))
    , m_hasInfoTitle(m_normTitle+" [*]")
    , m_hasInfo(false)
    , m_primary(primary)

{
    setObjectName(primary?"mergeView-primary":"mergeView-secondary");
    setWidget(m_browser);
    setToolTip(i18nc("@info:tooltip","Drop file to be merged into / synced with the current one here"));

    hide();

    setAcceptDrops(true);
    m_browser->setReadOnly(true);
    m_browser->setContextMenuPolicy(Qt::NoContextMenu);
    setContextMenuPolicy(Qt::ActionsContextMenu);
}

MergeView::~MergeView()
{
    delete m_mergeCatalog;
}

KUrl MergeView::url()
{
    if (m_mergeCatalog)
        return m_mergeCatalog->url();
    return KUrl();
}

void MergeView::dragEnterEvent(QDragEnterEvent* event)
{
    if(event->mimeData()->hasUrls() && Catalog::extIsSupported(event->mimeData()->urls().first().path()))
        event->acceptProposedAction();
}

void MergeView::dropEvent(QDropEvent *event)
{
    mergeOpen(KUrl(event->mimeData()->urls().first()));
    event->acceptProposedAction();
}

void MergeView::slotUpdate(const DocPosition& pos)
{
    if (pos.entry==m_pos.entry)
        slotNewEntryDisplayed(pos);
}

void MergeView::slotNewEntryDisplayed(const DocPosition& pos)
{
    m_pos=pos;

    if (!m_mergeCatalog)
        return;

    emit signalPriorChangedAvailable((pos.entry>m_mergeCatalog->firstChangedIndex())
                                    ||(pluralFormsAvailableBackward()!=-1));
    emit signalNextChangedAvailable((pos.entry<m_mergeCatalog->lastChangedIndex())
                                    ||(pluralFormsAvailableForward()!=-1));

    if (!m_mergeCatalog->isPresent(pos.entry))
    {
        //i.e. no corresponding entry, whether changed or not
        if (m_hasInfo)
        {
            m_hasInfo=false;
            setWindowTitle(m_normTitle);
            m_browser->clear();
//             m_browser->viewport()->setBackgroundRole(QPalette::Base);
        }
        emit signalEntryWithMergeDisplayed(false);

        /// no editing at all!  ////////////
        return;
    }
    if (!m_hasInfo)
    {
        m_hasInfo=true;
        setWindowTitle(m_hasInfoTitle);
    }

    emit signalEntryWithMergeDisplayed(m_mergeCatalog->isChanged(pos.entry));

    QString result=userVisibleWordDiff(m_baseCatalog->msgstr(pos),
                                       m_mergeCatalog->msgstr(pos),
                                       Project::instance()->accel(),
                                       Project::instance()->markup(),
                                       Html);
#if 0
    int i=-1;
    bool inTag=false;
    while(++i<result.size())//dynamic
    {
        if (!inTag)
        {
            if (result.at(i)=='<')
                inTag=true;
            else if (result.at(i)==' ')
                result.replace(i,1,"&sp;");
        }
        else if (result.at(i)=='>')
            inTag=false;
    }
#endif

    if (!m_mergeCatalog->isApproved(pos.entry))
    {
        result.prepend("<i>");
        result.append("</i>");
    }

    m_browser->setHtml(result);
//     kWarning()<<"ELA "<<time.elapsed();
}

void MergeView::cleanup()
{
    delete m_mergeCatalog;m_mergeCatalog=0;
    m_pos=DocPosition();

    emit signalPriorChangedAvailable(false);
    emit signalNextChangedAvailable(false);

    m_browser->clear();
}

void MergeView::mergeOpen(KUrl url)
{
    if (KDE_ISUNLIKELY( !m_baseCatalog->numberOfEntries() ))
        return;

    if (url==m_baseCatalog->url())
    {
        //(we are likely to be _mergeViewSecondary)
        //special handling: open corresponding file in the branch
        //for AutoSync

        QString path=QFileInfo(url.toLocalFile()).canonicalFilePath(); //bug 245546 regarding symlinks
        QString oldPath=path;
        path.replace(Project::instance()->poDir(),Project::instance()->branchDir());

        if (oldPath==path) //if file doesn't exist both are empty
        {
            cleanup();
            return;
        }

        url=KUrl(path);
    }

    if (url.isEmpty())
    {
        Project::instance()->model()->weaver()->suspend();
        url=KFileDialog::getOpenUrl(KUrl("kfiledialog:///merge-source") /*m_baseCatalog->url()*/ , "text/x-gettext-translation",this);
        Project::instance()->model()->weaver()->resume();
    }
    if (url.isEmpty())
        return;

    delete m_mergeCatalog; m_mergeCatalog=new MergeCatalog(this,m_baseCatalog);
    int errorLine=m_mergeCatalog->loadFromUrl(url);
    if (KDE_ISLIKELY( errorLine==0 ))
    {
        if (m_pos.entry>0)
            emit signalPriorChangedAvailable(m_pos.entry>m_mergeCatalog->firstChangedIndex());

        emit signalNextChangedAvailable(m_pos.entry<m_mergeCatalog->lastChangedIndex());

        //a bit hacky :)
        connect (m_mergeCatalog,SIGNAL(signalEntryModified(DocPosition)),this,SLOT(slotUpdate(DocPosition)));

        if (m_pos.entry!=-1)
            slotNewEntryDisplayed(m_pos);
        show();
    }
    else
    {
        //KMessageBox::error(this, KIO::NetAccess::lastErrorString() );
        cleanup();
        if (errorLine>0)
            KMessageBox::error(this, i18nc("@info","Error opening the file <filename>%1</filename> for synchronization, error line: %2",url.pathOrUrl(),errorLine) );
        else
        {
            /* disable this as requested by bug 272587
            KNotification* notification=new KNotification("MergeFilesOpenError", this);
            notification->setText( i18nc("@info %1 is full filename","Error opening the file <filename>%1</filename> for synchronization",url.pathOrUrl()) );
            notification->sendEvent();
            */
        }
        //i18nc("@info %1 is w/o path","No branch counterpart for <filename>%1</filename>",url.fileName()),
    }

}

int MergeView::pluralFormsAvailableForward()
{
    if(KDE_ISLIKELY( m_pos.entry==-1 || !m_mergeCatalog->isPlural(m_pos.entry) ))
        return -1;

    int formLimit=qMin(m_baseCatalog->numberOfPluralForms(),m_mergeCatalog->numberOfPluralForms());//just sanity check
    DocPosition pos=m_pos;
    while (++(pos.form)<formLimit)
    {
        if (m_baseCatalog->msgstr(pos)!=m_mergeCatalog->msgstr(pos))
            return pos.form;
    }
    return -1;
}

int MergeView::pluralFormsAvailableBackward()
{
    if(KDE_ISLIKELY( m_pos.entry==-1 || !m_mergeCatalog->isPlural(m_pos.entry) ))
        return -1;

    DocPosition pos=m_pos;
    while (--(pos.form)>=0)
    {
        if (m_baseCatalog->msgstr(pos)!=m_mergeCatalog->msgstr(pos))
            return pos.form;
    }
    return -1;
}

void MergeView::gotoPrevChanged()
{
    if (KDE_ISUNLIKELY( !m_mergeCatalog ))
        return;

    DocPosition pos;

    //first, check if there any plural forms waiting to be synced
    int form=pluralFormsAvailableBackward();
    if (KDE_ISUNLIKELY( form!=-1 ))
    {
        pos=m_pos;
        pos.form=form;
    }
    else if(KDE_ISUNLIKELY( (pos.entry=m_mergeCatalog->prevChangedIndex(m_pos.entry)) == -1 ))
        return;

    if (KDE_ISUNLIKELY( m_mergeCatalog->isPlural(pos.entry)&&form==-1 ))
        pos.form=qMin(m_baseCatalog->numberOfPluralForms(),m_mergeCatalog->numberOfPluralForms())-1;

    emit gotoEntry(pos,0);
}

void MergeView::gotoNextChanged()
{
    if (KDE_ISUNLIKELY( !m_mergeCatalog ))
        return;

    DocPosition pos;

    //first, check if there any plural forms waiting to be synced
    int form=pluralFormsAvailableForward();
    if (KDE_ISUNLIKELY( form!=-1 ))
    {
        pos=m_pos;
        pos.form=form;
    }
    else if(KDE_ISUNLIKELY( (pos.entry=m_mergeCatalog->nextChangedIndex(m_pos.entry)) == -1 ))
        return;

    emit gotoEntry(pos,0);
}

void MergeView::mergeBack()
{
    if(m_pos.entry==-1 ||!m_mergeCatalog ||m_baseCatalog->msgstr(m_pos).isEmpty())
        return;

    m_mergeCatalog->copyFromBaseCatalog(m_pos);
}

void MergeView::mergeAccept()
{
    if(m_pos.entry==-1
       ||!m_mergeCatalog
     //||m_baseCatalog->msgstr(m_pos)==m_mergeCatalog->msgstr(m_pos)
       ||m_mergeCatalog->msgstr(m_pos).isEmpty())
        return;

    m_mergeCatalog->copyToBaseCatalog(m_pos);

    emit gotoEntry(m_pos,0);
}

void MergeView::mergeAcceptAllForEmpty()
{
    if(KDE_ISUNLIKELY(!m_mergeCatalog)) return;

    bool update=m_mergeCatalog->changedEntries().contains(m_pos.entry);

    m_mergeCatalog->copyToBaseCatalog(/*MergeCatalog::EmptyOnly*/MergeCatalog::HigherOnly);

    if (update!=m_mergeCatalog->changedEntries().contains(m_pos.entry))
        emit gotoEntry(m_pos,0);
}


bool MergeView::event(QEvent *event)
{
    if (event->type()==QEvent::ToolTip && m_mergeCatalog)
    {
        QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
        QString text=i18nc("@info:tooltip","Different entries: %1\nUnmatched entries: %2",
                m_mergeCatalog->changedEntries().count(),m_mergeCatalog->unmatchedCount());
        text.replace('\n',"<br />");
        QToolTip::showText(helpEvent->globalPos(),text);
        return true;
    }
    return QWidget::event(event);
}

#include "mergeview.moc"
