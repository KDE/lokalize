/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2008 by Nick Shaforostoff <shafff@ukr.net>

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
#define KDE_NO_DEBUG_OUTPUT

#include "msgiddiffview.h"

#include "diff.h"
#include "catalog.h"
#include "project.h"

#include <klocale.h>
#include <kdebug.h>
#include <ktextedit.h>

//#include <QTime>

MsgIdDiff::MsgIdDiff(QWidget* parent, Catalog* catalog)
    : QDockWidget ( i18nc("@title:window","Original Diff"), parent)
    , m_browser(new KTextEdit(this))
    , m_catalog(catalog)
    , m_normTitle(i18nc("@title:window","Original Diff"))
    , m_hasInfoTitle(m_normTitle+" [*]")
    , m_hasInfo(false)
{
    setObjectName("msgIdDiff");
    setWidget(m_browser);
    setToolTip(i18nc("@info:tooltip","Sometimes, if original text is changed, its translation becomes <emphasis>fuzzy</emphasis> (i.e. looses approval status). This window shows the difference between new original string and the old one, so that you can easily see which changes should be applied to translation."));

    m_browser->setReadOnly(true);
}

MsgIdDiff::~MsgIdDiff()
{
}

void MsgIdDiff::slotNewEntryDisplayed(const DocPosition& pos)
{
    m_entry=DocPos(pos);
    QTimer::singleShot(0,this,SLOT(process()));
}

void MsgIdDiff::process()
{
    kWarning()<<"1";
    if (m_entry==m_prevEntry)
        return;
    if (m_catalog->numberOfEntries()<=m_entry.entry)
        return;//because of Qt::QueuedConnection

    m_prevEntry=m_entry;

    QString diff=m_catalog->alttrans(m_entry.toDocPosition());

    if (diff.isEmpty())
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

    m_browser->setHtml(diff);
}

#include "msgiddiffview.moc"
