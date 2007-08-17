/* ****************************************************************************
  This file is part of KAider
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
#include "catalog.h"
#include "catalog_private.h"
#include "catalogitem.h"

#include <QFile>
#include <QTextCodec>
#include <QList>
#include <QTextStream>
#include <QEventLoop>

#include <ksavefile.h>
#include <kapplication.h>
#include <klocale.h>
#include <kdebug.h>
//#include <kgenericfactory.h>

//K_EXPORT_COMPONENT_FACTORY( kbabel_gettextexport, KGenericFactory<GettextExportPlugin> ( "kbabelgettextexportfilter" ) )

//using namespace KBabel;

GettextExportPlugin::GettextExportPlugin(QObject* parent/*, const QString&List &*/) :
    CatalogExportPlugin(parent,"GettextExportPlugin"), m_wrapWidth( -1 )
{
}

ConversionStatus GettextExportPlugin::save(const QString& localFile,
                                           const QString& mimetype,
                                           const Catalog* catalog)
{
    // check, whether we know how to handle the extra data
    if ( catalog->importPluginID() != "GNU gettext")
        return UNSUPPORTED_TYPE;

    // we support on the text/x-gettext-translation MIME type
    if ( mimetype != "text/x-gettext-translation")
        return UNSUPPORTED_TYPE;

    //KSaveFile file(localFile);
    QFile file(localFile);

    if (!file.open(QIODevice::WriteOnly))
    {
        //emit signalError(i18n("Wasn't able to open file %1",filename.ascii()));
        return OS_ERROR;
    }
    //kWarning() << "SAVE NAME "<<localFile;

//      int progressRatio = qMax(100/ qMax(catalog->numberOfEntries(),uint(1)), uint(1));
//       emit signalResetProgressBar(i18n("saving file"),100);

    QTextStream stream(&file);



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
    const QString& headerComment( catalog->header().comment() );
    // ### TODO: why is this useful to have a header with an empty msgstr?
    if ( !headerComment.isEmpty() || !catalog->header().msgstrPlural().isEmpty() )
    {
        // write header
        writeComment( stream, headerComment );

        const QString& headerMsgid (catalog->header().msgid());

        // Gettext PO files should have an empty msgid as header
        if ( !headerMsgid.isEmpty() )
        {
            // ### TODO: perhaps it is grave enough for a user message
            kWarning() << "Non-empty msgid for the header, assuming empty msgid!" << endl << headerMsgid << "---";
        }

        // ### FIXME: if it is the header, then the msgid should be empty! (Even if KBabel has made something out of a non-header first entry!)
        stream << "msgid \"\"\n";

        writeKeyword( stream, "msgstr", catalog->header().msgstr() );

        stream << '\n';
    }

    QStringList list;
    for (int counter = 0; counter < catalog->numberOfEntries(); counter++)
    {
        /*          if(counter%10==0) {
                     emit signalProgress(counter/progressRatio);
        	  }*/

        // write entry
        writeComment( stream, catalog->comment(counter) );

        const QString& msgctxt = catalog->msgctxt(counter);
        if (! msgctxt.isEmpty() )
        {
            writeKeyword( stream, "msgctxt", msgctxt );
        }

        writeKeyword( stream, "msgid", catalog->msgid( counter ) );
        if ( catalog->pluralFormType( counter ) == Gettext )
            writeKeyword( stream, "msgid_plural", catalog->msgid( counter, 1 ) );

        if ( catalog->pluralFormType(counter) != Gettext)
        {
            writeKeyword( stream, "msgstr", catalog->msgstr( counter ) );
        }
        else
        {
            kDebug() << "Saving gettext plural form";
            //TODO check len of the actual stringlist??
            const int forms = catalog->numberOfPluralForms();
            for ( int i = 0; i < forms; ++i )
            {
                QString keyword ( "msgstr[" );
                keyword += QString::number( i );
                keyword += ']';

                writeKeyword( stream, keyword, ( catalog->msgstr( counter, i ) ) );
            }
        }

        stream << '\n';

//do we need this?
//         kapp->processEvents(QEventLoop::AllEvents,10);
//         if ( isStopped() )
//             return STOPPED;
    }

#if 0
//legacy
    if ( _saveSettings.saveObsolete )
#endif
    {
        QList<QString>::const_iterator oit;

        const QStringList& _obsolete (catalog->catalogExtraData());

        for (oit=_obsolete.constBegin();oit!=_obsolete.constEnd();++oit)
        {
            stream << (*oit) << "\n\n";

//TODO do we need this?
//             kapp->processEvents( QEventLoop::AllEvents, 10 );
//             if ( isStopped() )
//             {
//                 return STOPPED;
//             }
        }
    }

//       emit signalProgress(100);
    //file.finalize();
    file.close();

//       emit signalClearProgressBar();

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
            const QString& span ( ( newpos == -1 ) ? comment.mid( pos ) : comment.mid( pos, newpos-pos ) );

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

void GettextExportPlugin::writeKeyword( QTextStream& stream, const QString& keyword, const QString& text ) const
{
    if ( text.isEmpty() )
    {
        // Whatever the wrapping mode, an empty line is an empty line
        stream << keyword << " \"\"\n";
        return; 
    }

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
        {
            stream << '\"' << (*it) << "\"\n";
        }
        return;
    }

    if ( m_wrapWidth <= 0 ) // Unknown special wrapping, so assume "no wrap" instead
    {
        // No wrapping (like Gettext's --no.wrap or -w0 )
        // we need to remove the \n characters, as they are extra characters
        QString realText( text );
        realText.remove( '\n' );
        stream << keyword << " \"" << realText << "\"\n";
        return;
    }

    // lazy wrapping
    QStringList list = text.split( '\n', QString::SkipEmptyParts );

    if ( text.startsWith( '\n' ) )
        list.prepend( QString() );

    if(list.isEmpty())
        list.append( QString() );

    int max=m_wrapWidth-2;
    bool prependedEmptyLine=false;
    QStringList::iterator itm;
    for( itm = list.begin(); itm != list.end(); ++itm )
    {
        if (list.count()==1 && itm->length()>max-keyword.length()-1)
        {
            prependedEmptyLine=true;
            itm=list.insert(itm,"");
        }

        if (itm->length()>max)
        {
            int pos = itm->lastIndexOf(QRegExp("[ >.]"),max-1);
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
            (*itm)=itm->remove(0,pos);
            itm--;
            itm->truncate(pos);
        }
    }

    if( !prependedEmptyLine && list.count() > 1 )
        list.prepend( QString() );

    stream << keyword << " ";

    QStringList::const_iterator it;
    for( it = list.constBegin(); it != list.constEnd(); ++it )
    {
        stream << "\"" << (*it) << "\"\n";
    }
}
