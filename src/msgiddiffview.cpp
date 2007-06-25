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

#include "diff.h"
#include "catalog.h"
#include "msgiddiffview.h"

#include <klocale.h>
#include <kdebug.h>

#include <QTextBrowser>
#include <QTime>

MsgIdDiff::MsgIdDiff(QWidget* parent)
    : QDockWidget ( i18n("Original Diff"), parent)
    , m_browser(new QTextBrowser(this))
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
    QString oldStr(Catalog::instance()->comment(index));
    if (!oldStr.contains("#|"))
    {
        //kWarning()<< "___ returning... "<< endl;
        m_browser->clear();
        return;
    }
    QString newStr(Catalog::instance()->msgid(index));

    oldStr.replace("#| msgid_plural \"","#| \"");
    newStr.replace("#| msgid_plural \"","#| \"");


//     QTime time;
//     time.start();

    //get rid of other info (eg fuzzy marks)
    oldStr.remove(QRegExp("\\#[^\\|][^\n]*\n"));
    oldStr.remove(QRegExp("\\#[^\\|].*$"));

    oldStr.replace('<',"&lt;");
    newStr.replace('<',"&lt;");
    oldStr.replace('>',"&gt;");
    newStr.replace('>',"&gt;");
//     kWarning()<< "OLD "<< oldStr << endl;
//     kWarning()<< "NEW "<< newStr << endl;

    if (oldStr.contains("#| msgid \"\""))
    {
        oldStr.remove("#| \"");
        oldStr.remove(QRegExp("\"\n"));
        oldStr.remove(QRegExp("\"$"));
            //kWarning() << "BEGIN " << oldStr << " END" << endl;
    
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
//     kWarning()<< "NEW "<< newStr << endl;
/*    oldStr.prepend("aaa ggg aaa ");
    newStr.prepend("ggg aaa bbb aaa ");*/
    oldStr.prepend(' ');
    newStr.prepend(' ');
    QString result(wordDiff(oldStr,newStr));
    result.replace("<KBABELADD>","<font color=\"purple\">");
    result.replace("</KBABELADD>","</font>");
    result.replace("<KBABELDEL>","<font color=\"red\">");
    result.replace("</KBABELDEL>","</font>");

    result.replace("\\n","\\n<br>");
    result.remove(QRegExp("^ "));

    m_browser->setHtml(result);
//     m_browser->setPlainText(result);
//     kWarning()<<"ELA "<<time.elapsed()<<endl;
//     kWarning()<<" "<<result<<endl;

    oldStr.replace("\\n","\\n\n");
    newStr.replace("\\n","\\n\n");

}

#include "msgiddiffview.moc"
