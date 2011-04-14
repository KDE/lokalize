/* ****************************************************************************
  This file is part of Lokalize
  This file contains parts of KBabel code

  Copyright (C) 1999-2000 by Matthias Kiefer <matthias.kiefer@gmx.de>
		2001-2002 by Stanislav Visnovsky <visnovsky@kde.org>
  Copyright (C) 2005,2006 by Nicolas GOUTTE <goutte@kde.org>

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

#include "gettextexport.h"

//#include <resources.h>
#include "gettextstorage.h"
#include "catalogitem.h"

#include <QFile>
#include <QTextCodec>
#include <QList>
#include <QTextStream>
#include <QEventLoop>
#include <QStringBuilder>

#include <ksavefile.h>
#include <kapplication.h>
#include <klocale.h>
#include <kdebug.h>


using namespace GettextCatalog;

GettextExportPlugin::GettextExportPlugin(short wrapWidth, short trailingNewLines)
    : m_wrapWidth(wrapWidth)
    , m_trailingNewLines(trailingNewLines)
{
}

ConversionStatus GettextExportPlugin::save(QIODevice* device,
                                           const GettextStorage* catalog)
{
    QTextStream stream(device);

    //if ( m_wrapWidth == -1 ) m_wrapWidth=80;

#if 0
//legacy
    if (useOldEncoding && catalog->fileCodec())
    {
        stream.setCodec(catalog->fileCodec());
    }
    else

    {
        /*         switch(_saveSettings.encoding)
                 {
                    case ProjectSettingsBase::UTF8:
                       stream.setCodec(QTextCodec::codecForName("utf-8"));
                       break;
                    case ProjectSettingsBase::UTF16:
                       stream.setCodec(QTextCodec::codecForName("utf-16"));
                       break;
                    default:
        	       stream.setCodec(QTextCodec::codecForLocale());
                       break;
                 }*/
#endif
    //NOTE i had a look and even ja team uses utf-8 now
    stream.setCodec(QTextCodec::codecForName("utf-8"));


    // only save header if it is not empty
    const QString& headerComment( catalog->m_header.comment() );
    // ### why is this useful to have a header with an empty msgstr?
    if ( !headerComment.isEmpty() || !catalog->m_header.msgstrPlural().isEmpty() )
    {
        // write header
        writeComment( stream, headerComment );

        const QString& headerMsgid (catalog->m_header.msgid());

        // Gettext PO files should have an empty msgid as header
        if ( !headerMsgid.isEmpty() )
        {
            // ### perhaps it is grave enough for a user message
            kWarning() << "Non-empty msgid for the header, assuming empty msgid!" << endl << headerMsgid << "---";
        }

        // ### FIXME: if it is the header, then the msgid should be empty! (Even if KBabel has made something out of a non-header first entry!)
        stream << "msgid \"\"\n";

        writeKeyword( stream, "msgstr", catalog->m_header.msgstr(), false );
    }


    const QVector<CatalogItem>& catalogEntries=catalog->m_entries;
    int limit=catalog->numberOfEntries();
    QStringList list;
    for (int counter = 0; counter < limit; counter++)
    {
        stream << '\n';

        // write entry
        writeComment( stream, catalogEntries.at(counter).comment() );

        const QString& msgctxt = catalogEntries.at(counter).msgctxt();
        if (! msgctxt.isEmpty() )
            writeKeyword( stream, "msgctxt", msgctxt );

        writeKeyword( stream, "msgid", catalogEntries.at(counter).msgid(), true, catalogEntries.at(counter).prependEmptyForMsgid() );
        if ( catalogEntries.at(counter).isPlural() )
            writeKeyword( stream, "msgid_plural", catalogEntries.at(counter).msgid(1), true, catalogEntries.at(counter).prependEmptyForMsgid() );

        if (!catalogEntries.at(counter).isPlural())
            writeKeyword( stream, "msgstr", catalogEntries.at(counter).msgstr(), true, catalogEntries.at(counter).prependEmptyForMsgstr() );
        else
        {
            kDebug() << "Saving gettext plural form";
            //TODO check len of the actual stringlist??
            const int forms = catalog->numberOfPluralForms();
            for ( int i = 0; i < forms; ++i )
            {
                QString keyword = "msgstr[" % QString::number( i ) % ']';
                writeKeyword( stream, keyword, catalogEntries.at(counter).msgstr(i), true, catalogEntries.at(counter).prependEmptyForMsgstr() );
            }
        }
    }

#if 0
//legacy
    if ( _saveSettings.saveObsolete )
#endif
    {
        QList<QString>::const_iterator oit;
        const QStringList& _obsolete=catalog->m_catalogExtraData;
        oit=_obsolete.constBegin();
        if (oit!=_obsolete.constEnd())
        {
            stream << "\n" << (*oit);
            while((++oit)!=_obsolete.constEnd())
                stream << "\n\n" << (*oit);
        }
    }

    int i=m_trailingNewLines+1;
    while (--i>=0)
        stream << '\n';

    return OK;
}

void GettextExportPlugin::writeComment( QTextStream& stream, const QString& comment ) const
{
    if( !comment.isEmpty() )
    {
        // We must check that each comment line really starts with a #, to avoid syntax errors
        int pos = 0;
        for(;;)
        {
            const int newpos = comment.indexOf( '\n', pos, Qt::CaseInsensitive );
            if ( newpos == pos )
            {
                ++pos;
                stream << '\n';
                continue;
            }
            const QString& span ((newpos==-1 ) ? comment.mid(pos) : comment.mid(pos, newpos-pos) );

            const int len = span.length();
            QString spaces; // Stored leading spaces
            for ( int i = 0 ; i < len ; ++i )
            {
                const QChar& ch = span[ i ];
                if ( ch == '#' )
                {
                    stream << spaces << span.mid( i );
                    break;
                }
                else if ( ch == ' ' || ch == '\t' )
                {
                    // We have a leading white space character, so store it temporary
                    spaces += ch;
                }
                else
                {
                    // Not leading white space and not a # character. so consider that the # character was missing at first position.
                    stream << "# " << spaces << span.mid( i );
                    break;
                }
            }
            stream << '\n';

            if ( newpos == -1 )
                break;
            else
                pos = newpos + 1;
        }
    }
}

void GettextExportPlugin::writeKeyword( QTextStream& stream, const QString& keyword, QString text, bool containsHtml, bool startedWithEmptyLine ) const
{
    if ( text.isEmpty() )
    {
        // Whatever the wrapping mode, an empty line is an empty line
        stream << keyword << " \"\"\n";
        return;
    }

    //TODO remove this for KDE 4.4
    //NOTE not?
    int pos=0;
    while ((pos=text.indexOf("\\\"",pos))!=-1)
    {
        if (pos==0 || text.at(pos-1)!='\\')
            text.replace(pos,2,'"');
        else
            pos++;
    }
    text.replace('"',"\\\"");
#if 0
    if ( m_wrapWidth == -1 )
    {
        // Traditional KBabel wrapping
        QStringList list = text.split( '\n', QString::SkipEmptyParts );

        if ( text.startsWith( '\n' ) )
            list.prepend( QString() );

        if(list.isEmpty())
            list.append( QString() );

        if( list.count() > 1 )
            list.prepend( QString() );

        stream << keyword << ' ';

        QStringList::const_iterator it;
        for( it = list.constBegin(); it != list.constEnd(); ++it )
            stream << '\"' << (*it) << "\"\n";
        return;
    }
#endif

    if ( m_wrapWidth == 0 ) // Unknown special wrapping, so assume "no wrap" instead
    {
        // No wrapping (like Gettext's --no.wrap or -w0 )
        // we need to remove the \n characters, as they are extra characters
        QString realText( text );
        realText.remove( '\n' );
        stream << keyword << " \"" << realText << "\"\n";
        return;
    }
    else if ( m_wrapWidth < 0 )
    {
        // No change in wrapping
        QStringList list = text.split( '\n');
        if (list.count()>1 || startedWithEmptyLine /* || keyword.length()+3+text.length()>=80*/)
            list.prepend(QString());

        stream << keyword << " ";
        QStringList::const_iterator it;
        for( it = list.constBegin(); it != list.constEnd(); ++it )
            stream << "\"" << (*it) << "\"\n";

        return;
    }

    // lazy wrapping
    QStringList list = text.split( '\n', QString::SkipEmptyParts );

    if ( text.startsWith( '\n' ) )
        list.prepend( QString() );

    if(list.isEmpty())
        list.append( QString() );

    static QRegExp breakStopReForHtml("[ >.%]", Qt::CaseSensitive, QRegExp::Wildcard);
    QRegExp breakStopRe=containsHtml?breakStopReForHtml:QRegExp("[ .%]", Qt::CaseSensitive, QRegExp::Wildcard);

    int max=m_wrapWidth-2;
    bool prependedEmptyLine=false;
    QStringList::iterator itm;
    for( itm = list.begin(); itm != list.end(); ++itm )
    {
        if (list.count()==1 && keyword.length()+1+itm->length()>=max)
        {
            prependedEmptyLine=true;
            itm=list.insert(itm,QString());
        }

        if (itm->length()>max)
        {
            int pos = itm->lastIndexOf(breakStopRe,max-1);
            if (pos>0)
            {
                int pos2 = itm->indexOf('<',pos);
                if (pos2>0&&pos2<max-1)
                    pos=itm->indexOf('<',pos);
                ++pos;
            }
            else
                pos=max;
            //itm=list.insert(itm,itm->left(pos));
            QString t=*itm;
            itm=list.insert(itm,t);
            itm++;
            if (itm != list.end())
            {
                (*itm)=itm->remove(0,pos);
                itm--;
                if (itm != list.end())
                    itm->truncate(pos);
            }
        }
    }

    if( !prependedEmptyLine && list.count() > 1 )
        list.prepend( QString() );

    stream << keyword << " ";

    QStringList::const_iterator it;
    for( it = list.constBegin(); it != list.constEnd(); ++it )
        stream << "\"" << (*it) << "\"\n";
}
