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

#include "msgiddiffview.h"

#include "diff.h"
#include "catalog.h"
#include "prefs_kaider.h"

#include <klocale.h>
#include <kdebug.h>

#include <QTextBrowser>
//#include <QTime>

MsgIdDiff::MsgIdDiff(QWidget* parent, Catalog* catalog)
    : QDockWidget ( i18nc("@title:window","Original Diff"), parent)
    , m_browser(new QTextBrowser(this))
    , m_catalog(catalog)
    , m_normTitle(i18nc("@title:window","Original Diff"))
    , m_hasInfoTitle(m_normTitle+" [*]")
    , m_hasInfo(false)
{
    setObjectName("msgIdDiff");
    setWidget(m_browser);
}

MsgIdDiff::~MsgIdDiff()
{
    delete m_browser;
}

void MsgIdDiff::slotNewEntryDisplayed(uint index)
{
    QString oldStr(m_catalog->comment(index));
    if (!oldStr.contains("#|"))
    {
        ////kWarning()<< "___ returning... ";
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
    QString newStr(m_catalog->msgid(index));

    oldStr.replace("#| msgid_plural \"","#| \"");
    newStr.replace("#| msgid_plural \"","#| \"");


//     QTime time;
//     time.start();

    //get rid of other info (eg fuzzy marks)
    oldStr.remove(QRegExp("\\#[^\\|][^\n]*\n"));
    oldStr.remove(QRegExp("\\#[^\\|].*$"));

    if (oldStr.contains("#| msgid \"\""))
    {
        oldStr.remove("#| \"");
        oldStr.remove(QRegExp("\"\n"));
        oldStr.remove(QRegExp("\"$"));
            //kWarning() << "BEGIN " << oldStr << " END";

        newStr.remove("\n");
        oldStr.replace("\\n"," \\n ");
        newStr.replace("\\n"," \\n ");
        oldStr.remove("#| msgid \"");
    }
    else
    {
        oldStr.remove("#| msgid \"");
        oldStr.remove(QRegExp("\"$"));
        oldStr.remove("\"\n");
    }

    QString result(wordDiff(oldStr,newStr));
    result.replace("\\n","\\n<br>");

    m_browser->setHtml(result);
//     m_browser->setPlainText(result);
//     kWarning()<<" "<<result;

//     oldStr.replace("\\n","\\n\n");
//     newStr.replace("\\n","\\n\n");

    //kWarning()<<"ELA "<<time.elapsed();
}

#include "msgiddiffview.moc"
