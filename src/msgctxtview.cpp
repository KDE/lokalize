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

#include "msgctxtview.h"

#include "catalog.h"

#include <klocale.h>
#include <kdebug.h>

#include <QTextBrowser>

MsgCtxtView::MsgCtxtView(QWidget* parent, Catalog* catalog)
    : QDockWidget ( i18nc("@title:window","Message Context"), parent)
    , m_browser(new QTextBrowser(this))
    , m_catalog(catalog)
    , m_normTitle(i18nc("@title:window","Message Context"))
    , m_hasInfoTitle(m_normTitle+" [*]")
    , m_hasInfo(false)
{
    setObjectName("msgCtxtView");
    setWidget(m_browser);
}

MsgCtxtView::~MsgCtxtView()
{
    delete m_browser;
}

void MsgCtxtView::slotNewEntryDisplayed(uint index)
{
    m_browser->clear();
    QString comment(m_catalog->comment(index));
    comment.replace('<',"&lt;");
    comment.replace('>',"&gt;");
    int pos=comment.indexOf("#:");
    if (pos!=-1)
    {
        comment.replace(pos,2,i18nc("@info PO comment parsing","<br><i>Place:</i>"));
        comment.replace("#:","<br>");
    }
    pos=comment.indexOf("#. i18n:");
    if (pos!=-1)
    {
        comment.replace(pos,8,i18nc("@info PO comment parsing","<br><i>GUI place:</i>"));
        comment.replace("#. i18n:","<br>");
    }
    comment.replace("#,",i18nc("@info PO comment parsing","<br><i>Misc:</i>"));
    //comment.remove(QRegExp("#\\|[^\n]+\n")); //we're parsing this in another view
    comment.replace("#| msgid",i18nc("@info PO comment parsing","<br><i>Previous string:</i>"));
    comment.replace("#| msgctxt",i18nc("@info PO comment parsing","<br><i>Previous context:</i>"));
    comment.replace("#|","<br>");
    comment.replace("#","<br>");
    m_browser->setHtml(i18nc("@info PO comment parsing","<b>Comment:</b>")+comment);

    if (m_catalog->msgctxt(index).isEmpty())
    {
        if (m_hasInfo)
        {
            setWindowTitle(m_normTitle);
            m_hasInfo=false;
        }
    }
    else
    {
        if (!m_hasInfo)
        {
            setWindowTitle(m_hasInfoTitle);
            m_hasInfo=true;
        }
        QTextCursor t=m_browser->textCursor();
        t.movePosition(QTextCursor::End);
        m_browser->setTextCursor(t);
        m_browser->insertHtml(i18nc("@info PO comment parsing","<br><b>Context:</b><br>")+m_catalog->msgctxt(index));
    }
}

#include "msgctxtview.moc"
