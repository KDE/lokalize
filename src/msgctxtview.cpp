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

#include "catalog.h"
#include "msgctxtview.h"

#include <klocale.h>
#include <kdebug.h>

#include <QTextBrowser>

MsgCtxtView::MsgCtxtView(QWidget* parent, Catalog* catalog)
    : QDockWidget ( i18n("Message Context"), parent)
    , m_browser(new QTextBrowser(this))
    , m_catalog(catalog)
    , m_normTitle(i18n("Message Context"))
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
//     if (m_catalog->msgctxt(index).isEmpty())
//     {
//         m_browser->clear();
//         return;
//     }
    if (m_catalog->msgctxt(index).isEmpty())
    {
        if (m_hasInfo)
        {
            m_browser->clear();
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
        m_browser->setText(m_catalog->msgctxt(index));
    }
}

#include "msgctxtview.moc"
