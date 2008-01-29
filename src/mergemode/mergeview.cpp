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

#include "mergeview.h"

#include "cmd.h"
#include "mergecatalog.h"
#include "project.h"
#include "diff.h"

#include <klocale.h>
#include <kdebug.h>
#include <kurl.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <ktextedit.h>

#include <QDragEnterEvent>

MergeView::MergeView(QWidget* parent, Catalog* catalog,bool primary)
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
}

MergeView::~MergeView()
{
    delete m_mergeCatalog;
}

void MergeView::dragEnterEvent(QDragEnterEvent* event)
{
    if(event->mimeData()->hasUrls() && event->mimeData()->urls().first().path().endsWith(".po"))
    {
        //kWarning() << " " <<;
        event->acceptProposedAction();
    };
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
    //TODO clear view on 'delete m_mergeCatalog'
    if (!m_mergeCatalog)
        return;

    emit signalPriorChangedAvailable(pos.entry>m_mergeCatalog->firstChangedIndex());
    emit signalNextChangedAvailable(pos.entry<m_mergeCatalog->lastChangedIndex());

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

    QString result(wordDiff(m_baseCatalog->msgstr(pos),
                            m_mergeCatalog->msgstr(pos),
                            Project::instance()->accel(),
                            Project::instance()->markup()
                           ));
    result.replace("\\n","\\n<br>");
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
    //static QPalette::ColorRole roles[2]={/*QPalette::Base*/QPalette::LinkVisited,QPalette::AlternateBase};
    //m_browser->viewport()->setBackgroundRole(roles[m_mergeCatalog->isFuzzy(pos.entry)]);
//     static QPalette::ColorRole roles[2]={/*QPalette::Base*/QPalette::WindowText,QPalette::BrightText};
    //m_browser->viewport()->setForegroundRole(roles[m_mergeCatalog->isFuzzy(pos.entry)]);
    if (m_mergeCatalog->isFuzzy(pos.entry))
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

    emit signalPriorChangedAvailable(false);
    emit signalNextChangedAvailable(false);

    m_browser->clear();
}

void MergeView::mergeOpen(KUrl url)
{
    if (KDE_ISUNLIKELY( !m_baseCatalog->numberOfEntries() ))
        return;

    if (url.isEmpty())
        url=KFileDialog::getOpenUrl(KUrl("kfiledialog:///merge-source") /*m_baseCatalog->url()*/ , "text/x-gettext-translation",this);
    if (url.isEmpty())
        return;

    delete m_mergeCatalog;
    m_mergeCatalog=new MergeCatalog(this,m_baseCatalog,m_primary);
    if (KDE_ISLIKELY( m_mergeCatalog->loadFromUrl(url) ))
    {
        if (m_pos.entry>0)
            emit signalPriorChangedAvailable(m_pos.entry>m_mergeCatalog->firstChangedIndex());

        emit signalNextChangedAvailable(m_pos.entry<m_mergeCatalog->lastChangedIndex());

        //a bit hacky :)
        connect (m_mergeCatalog,SIGNAL(signalEntryChanged(DocPosition)),this,SLOT(slotUpdate(DocPosition)));


        slotNewEntryDisplayed(m_pos);
        show();
    }
    else
    {
        //KMessageBox::error(this, KIO::NetAccess::lastErrorString() );
        cleanup();
        KMessageBox::error(this, i18nc("@info","Error opening the file <filename>%1</filename>",url.pathOrUrl()) );
    }

}



void MergeView::gotoPrevChanged()
{
    if (KDE_ISUNLIKELY( !m_mergeCatalog ))
        return;

    DocPosition pos;

    if(KDE_ISUNLIKELY( (pos.entry=m_mergeCatalog->prevChangedIndex(m_pos.entry)) == -1 ))
        return;

    emit gotoEntry(pos,0);
}

void MergeView::gotoNextChanged()
{
    if (KDE_ISUNLIKELY( !m_mergeCatalog ))
        return;

    DocPosition pos;

    if(KDE_ISUNLIKELY( (pos.entry=m_mergeCatalog->nextChangedIndex(m_pos.entry)) == -1 ))
        return;

    emit gotoEntry(pos,0);
}


void MergeView::mergeAccept()
{
    if(m_pos.entry==-1
       ||!m_mergeCatalog
       ||m_baseCatalog->msgstr(m_pos)==m_mergeCatalog->msgstr(m_pos)
       ||m_mergeCatalog->msgstr(m_pos).isEmpty())
        return;

    m_baseCatalog->beginMacro(i18nc("@item Undo action item","Accept change in translation"));

    m_pos.offset=0;
    if (!m_baseCatalog->msgstr(m_pos).isEmpty())
        m_baseCatalog->push(new DelTextCmd(m_baseCatalog,m_pos,m_baseCatalog->msgstr(m_pos)));

    if ( m_baseCatalog->isFuzzy(m_pos.entry) && !m_mergeCatalog->isFuzzy(m_pos.entry)       )
        m_baseCatalog->push(new ToggleFuzzyCmd(m_baseCatalog,m_pos.entry,false));
    else if ( !m_baseCatalog->isFuzzy(m_pos.entry) && m_mergeCatalog->isFuzzy(m_pos.entry) )
        m_baseCatalog->push(new ToggleFuzzyCmd(m_baseCatalog,m_pos.entry,true));

    m_baseCatalog->push(new InsTextCmd(m_baseCatalog,m_pos,m_mergeCatalog->msgstr(m_pos)));

    ////////this is NOT done automatically by BaseCatalogEntryChanged slot
    bool remove=true;
    if (m_mergeCatalog->isPlural(m_pos.entry))
    {
        DocPosition pos=m_pos;
        pos.form=qMax(m_baseCatalog->numberOfPluralForms(),1);
        while ((--(pos.form))>=0)
        {
            kWarning()<<pos.form;
            if (m_baseCatalog->msgstr(pos)!=m_mergeCatalog->msgstr(pos))
            {
                remove=false;
                break;
            }
        }

    }
    if (remove)
        m_mergeCatalog->removeFromDiffIndex(m_pos.entry);


    m_baseCatalog->endMacro();

    emit gotoEntry(m_pos,0);
}


void MergeView::mergeAcceptAllForEmpty()
{
    if(!m_mergeCatalog)
        return;

    MergeCatalog& mergeCatalog=*m_mergeCatalog;

    DocPosition pos;
    pos.entry=mergeCatalog.firstChangedIndex();
    pos.offset=0;
    int end=mergeCatalog.lastChangedIndex();
    if (KDE_ISUNLIKELY( end==-1 ))
        return;

    bool insHappened=false;
    bool update=false;
    do
    {
        if (m_baseCatalog->isUntranslated(pos.entry))
        {
            int formsCount=(m_baseCatalog->isPlural(pos.entry))?m_baseCatalog->numberOfPluralForms():1;
            pos.form=0;
            while (pos.form<formsCount)
            {
                //m_baseCatalog->push(new DelTextCmd(m_baseCatalog,pos,m_baseCatalog->msgstr(pos.entry,0)));
                //some forms may still contain translation...
                m_baseCatalog->isUntranslated(pos);
                if (m_baseCatalog->isUntranslated(pos))
                {
                    if (!insHappened)
                    {
                        insHappened=true;
                        m_baseCatalog->beginMacro(i18nc("@item Undo action item","Accept all new translations"));
                    }

                    //mergeAccept(m_pos,true);
                    /// ///
                    m_baseCatalog->push(new InsTextCmd(m_baseCatalog,pos,mergeCatalog.msgstr(pos)));
                    /// ///
                    if(m_pos.entry==pos.entry)
                        update=true;
                }
                ++(pos.form);
            }
            /// ///
            mergeCatalog.removeFromDiffIndex(m_pos.entry);
            /// ///
        }
        if (KDE_ISUNLIKELY( pos.entry==end ))
            break;
        pos.entry=mergeCatalog.nextChangedIndex(pos.entry);
    } while (pos.entry!=-1);

    if (insHappened)
        m_baseCatalog->endMacro();

    if (update)
        emit gotoEntry(m_pos,0);
}


