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
    , m_entry(-1)
    , m_prevEntry(-1)
{
    setObjectName("msgIdDiff");
    setWidget(m_browser);
    setToolTip(i18nc("@info:tooltip","Sometimes, if original is changed, its translation becomes <emphasis>fuzzy</emphasis>. This window shows the difference between new original string and the old one, so that you can see what changes should be applied to translation."));

    m_browser->setReadOnly(true);
}

MsgIdDiff::~MsgIdDiff()
{
    delete m_browser;
}

void MsgIdDiff::slotNewEntryDisplayed(uint index)
{
    m_entry=index;
    QTimer::singleShot(0,this,SLOT(process()));
}

void MsgIdDiff::process()
{
    if (m_entry==m_prevEntry)
        return;
    if (m_catalog->numberOfEntries()<=m_entry)
        return;//because of Qt::QueuedConnection

    m_prevEntry=m_entry;
    QString oldStr(m_catalog->comment(m_entry));
    if (!oldStr.contains("#|"))
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
    QString newStr(m_catalog->msgid(m_entry));

    oldStr.replace("#| msgid_plural \"","#| \"");
    newStr.replace("#| msgid_plural \"","#| \"");


//     QTime time;
//     time.start();

    //get rid of other info (eg fuzzy marks)
    oldStr.remove(QRegExp("\\#[^\\|][^\n]*\n"));
    oldStr.remove(QRegExp("\\#[^\\|].*$"));
    QRegExp rmCtxt("\\#\\| msgctxt\\b.*(?=\n#\\|\\s*[^\"]|$)");
    rmCtxt.setMinimal(true);
    oldStr.remove(rmCtxt);


    if (oldStr.contains("#| msgid \"\"")) //multiline
    {
        oldStr.remove("#| \"");
        oldStr.remove(QRegExp("\"\n"));
        oldStr.remove(QRegExp("\"$"));

        newStr.remove('\n');
        oldStr.replace("\\n"," \\n ");
        newStr.replace("\\n"," \\n ");
        oldStr.remove("#| msgid \"");
    }
    else //signle line
    {
        oldStr.remove("#| msgid \"");
        oldStr.remove(QRegExp("\"$"));
        oldStr.remove("\"\n");
    }

    QString result(wordDiff(oldStr,
                            newStr,
                            Project::instance()->accel(),
                            Project::instance()->markup()
                           ));
    result.replace("\\n","\\n<br>");

    m_browser->setHtml(result);
//     m_browser->setPlainText(result);

//     oldStr.replace("\\n","\\n\n");
//     newStr.replace("\\n","\\n\n");

//     kDebug()<<"ELA "<<time.elapsed();
}

#include "msgiddiffview.moc"
