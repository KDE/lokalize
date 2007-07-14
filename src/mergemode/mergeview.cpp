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

#include "diff.h"
#include "mergecatalog.h"

#include <klocale.h>
#include <kdebug.h>
#include <kurl.h>

#include <QTextBrowser>
#include <QDragEnterEvent>

MergeView::MergeView(QWidget* parent, Catalog* catalog)
    : QDockWidget ( i18n("Merge Diff"), parent)
    , m_browser(new QTextBrowser(this))
    , m_baseCatalog(catalog)
    , m_mergeCatalog(0)
    , m_normTitle(i18n("Merge Diff"))
    , m_hasInfoTitle(m_normTitle+" [*]")
    , m_hasInfo(false)

{
    setObjectName("mergeView");
    setWidget(m_browser);

    setAcceptDrops(true);
}

MergeView::~MergeView()
{
    delete m_browser;
}

void MergeView::dragEnterEvent(QDragEnterEvent* event)
{
    if(event->mimeData()->hasUrls() && event->mimeData()->urls().first().path().endsWith(".po"))
    {
        //kWarning() << " " << <<endl;
        event->acceptProposedAction();
    };
}

void MergeView::dropEvent(QDropEvent *event)
{
    emit mergeOpenRequested(KUrl(event->mimeData()->urls().first()));
    event->acceptProposedAction();
}

void MergeView::slotEntryWithMergeDisplayed(bool really, const DocPosition& pos)
{
//     if (!m_mergeCatalog)
//         return;
    //if (!m_mergeCatalog->isValid(pos.entry))
    if (!really)
    {
        if (m_hasInfo)
        {
            m_hasInfo=false;
            setWindowTitle(m_normTitle);
            m_browser->clear();
            m_browser->viewport()->setBackgroundRole(QPalette::Base);
        }
        return;
    }
    if (!m_hasInfo)
    {
        m_hasInfo=true;
        setWindowTitle(m_hasInfoTitle);
    }
    QString newStr(m_mergeCatalog->msgstr(pos));
    QString oldStr(m_baseCatalog->msgstr(pos));


    oldStr.replace('<',"&lt;");
    newStr.replace('<',"&lt;");
    oldStr.replace('>',"&gt;");
    newStr.replace('>',"&gt;");
//     kWarning()<< "OLD "<< oldStr << endl;
//     kWarning()<< "NEW "<< newStr << endl;

//     kWarning()<< "NEW "<< newStr << endl;
/*    oldStr.prepend("aaa ggg aaa ");
    newStr.prepend("ggg aaa bbb aaa ");*/
    oldStr.prepend(' ');
    newStr.prepend(' ');

    QString result(wordDiff(oldStr,newStr));

    result.remove("</KBABELADD><KBABELADD>");
    result.remove("</KBABELDEL><KBABELDEL>");

    result.replace("<KBABELADD>","<font color=\"purple\">");
    result.replace("</KBABELADD>","</font>");
    result.replace("<KBABELDEL>","<font color=\"red\">");
    result.replace("</KBABELDEL>","</font>");

    result.replace("\\n","\\n<br>");
    result.remove(QRegExp("^ "));

    if (m_mergeCatalog->isFuzzy(pos.entry))
        m_browser->viewport()->setBackgroundRole(QPalette::AlternateBase);
    else
        m_browser->viewport()->setBackgroundRole(QPalette::Base);

//     result.replace("&lt;",'<');
//     result.replace("&gt;",'>');

    m_browser->setHtml(result);
//     m_browser->setPlainText(result);
//     kWarning()<<"ELA "<<time.elapsed()<<endl;
    //kWarning()<<" try "<<result<<endl;

//     result.replace("&lt;",'<');
//     result.replace("&gt;",'>');

//     oldStr.replace("\\n","\\n\n");
//     newStr.replace("\\n","\\n\n");

}

void MergeView::cleanup()
{
    m_browser->clear();
}


#include "mergeview.moc"
