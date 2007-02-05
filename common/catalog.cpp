/* ****************************************************************************
  This file is part of KAider
  This file contains parts of KBabel code

  Copyright (C) 1999-2000	by Matthias Kiefer <matthias.kiefer@gmx.de>
		2001-2005	by Stanislav Visnovsky <visnovsky@kde.org>
		2006	by Nicolas Goutte <goutte@kde.org>
		2007	by Nick Shaforostoff <shafff@ukr.net>

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





#include <QString>

#include <kdebug.h>
#include <kio/netaccess.h>
#include <kmessagebox.h>

#include "gettextimport.h"
#include "catalog.h"

Catalog* Catalog::_instance=0;

Catalog* Catalog::instance()
{
    if (_instance==0)
        _instance=new Catalog();
    return _instance;
}


Catalog::Catalog(/*QObject *parent*/):QUndoStack()
{
    d = new CatalogPrivate();
}

Catalog::~Catalog()
{
    delete d;
}


void Catalog::clear()
{
    QUndoStack::clear();
    d->_errorIndex.clear();
    d->_fuzzyIndex.clear();
    d->_untransIndex.clear();
    d->_entries.clear();
    d->_url=KUrl();
    d->_obsoleteEntries.clear();
/*
    d->msgidDiffList.clear();
    d->msgstr2MsgidDiffList.clear();
    d->diffCache.clear();
    */
}


//ConversionStatus Catalog::populateFromPO(const KUrl& url)
// void Catalog::setEntries(QVector<CatalogItem> entries)
// {
//     d->_entries=entries;
// }

QString Catalog::msgid(uint index, const uint form, const bool noNewlines) const
{
    if (  d->_entries.isEmpty() )
        return QString();

   return d->_entries[index].msgid(form,noNewlines);
}

QStringList Catalog::msgidPlural(uint index, const bool noNewlines) const
{
   return d->_entries[index].msgidPlural(noNewlines);
}

QString Catalog::msgstr(uint index, const uint form, const bool noNewlines) const
{
    if (  d->_entries.isEmpty() )
        return QString();

   return d->_entries[index].msgstr(form, noNewlines);
}

QStringList Catalog::msgstrPlural(uint index, const bool noNewlines) const
{
   return d->_entries[index].msgstrPlural(noNewlines);
}

QString Catalog::comment(uint index) const
{
    if (  d->_entries.isEmpty() )
        return QString();

   return d->_entries[index].comment();
}

QString Catalog::msgctxt(uint index) const
{
    if (  d->_entries.isEmpty() )
        return QString();

    return d->_entries[index].msgctxt();
}

PluralFormType Catalog::pluralFormType(uint index) const
{
    if (  d->_entries.isEmpty() )
        return NoPluralForm;

//    uint max=d->_entries.count()-1;
//    if(index > max)
//       index=max;

   return d->_entries[index].pluralFormType();
}

bool Catalog::isFuzzy(uint index) const
{
    if (  d->_entries.isEmpty() )
        return false;

   return d->_entries[index].isFuzzy();
}

bool Catalog::isUntranslated(uint index) const
{
    if (  d->_entries.isEmpty() )
        return false;

   return d->_entries[index].isUntranslated();
}


bool Catalog::setHeader(CatalogItem newHeader)
{
   if(newHeader.isValid())
   {
      // normalize the values - ensure every key:value pair is only on a single line
      QString values = newHeader.msgstr();
      values.replace ("\n", "");
      values.replace ("\\n", "\\n\n");
      
      kDebug () << "Normalized header: " << values << endl;
      
      d->_header=newHeader;
      d->_header.setMsgstr (values);

//       setClean(false);

      //emit signalHeaderChanged();

      return true;
   }

   return false;
}

ConversionStatus Catalog::openUrl(const KUrl& url/*, const QString& package*/)
{
    GettextImportPlugin importer(this);
    ConversionStatus status = OK;
    QString target;
    if( KIO::NetAccess::download( url, target, NULL ) )
    {
        status = importer.open(target,QString("application/x-gettext"),this);
        KIO::NetAccess::removeTempFile( target );
        d->_url=url;

        return status;

    }

    return OS_ERROR;

}

DocPosition& Catalog::undo()
{
    QUndoStack::undo();
    return d->_posBuffer;
//     emit signalGotoEntry(d->_posBuffer,0);
}

DocPosition& Catalog::redo()
{
    QUndoStack::redo();
    return d->_posBuffer;
//     emit signalGotoEntry(d->_posBuffer,0);
}









int Catalog::findNextInList(const QList<uint>& list,uint index) const
{
    if( list.isEmpty() )
        return -1;

    int nextIndex=-1;
    for ( int i = 0; i < list.size(); ++i ) 
    {
        if ( list.at(i) > index ) 
        {
            nextIndex = list.at(i);
            break;
        }
    }

    return nextIndex;
}

int Catalog::findPrevInList(const QList<uint>& list,uint index) const
{
    if ( list.isEmpty() )
        return -1;

    int prevIndex=-1;
    for ( int i = list.size()-1; i >= 0; --i )
    {
        if ( list.at(i) < index ) 
        {
            prevIndex = list.at(i);
            break;
        }
    }

    return prevIndex;
}















#if 0
bool Catalog::findNext(/*const FindOptions* findOpts,*/QString a, DocPosition& docPos, int& len)
{
    bool success = false; // true, when string found
    bool endReached=false;

    kDebug() << "findNext active" << endl;

//     MiscSettings miscOptions = miscSettings();

    len=0;
    int pos=0;

    QString searchStr = findOpts->findStr;
    QRegExp regexp(searchStr);

//     if ( findOpts->isRegExp )
//     {
//         regexp.setCaseSensitivity( ( findOpts->caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive ) ); // TODO: change this bool to a Qt::CaseSensitivity in findOpts
//     }

//     if ( docPos.entry == numberOfEntries()-1)
//     {
//         switch (docPos.part)
//         {
//         case Msgid:
//             // FIXME: we should search in plurals as well
//             if (!findOpts->inMsgstr && !findOpts->inComment
//                     && docPos.offset >= msgid(docPos.entry).first().length() )
//             {
//                 endReached=true;
//             }
//             break;
//         case Msgstr:
//             if (!findOpts->inComment && (int)(docPos.form+1) >= numberOfPluralForms(docPos.entry)
//                     && docPos.offset >= msgstr(docPos.entry).last().length() )
//             {
//                 endReached=true;
//             }
//             break;
//         case Comment:
//             if (docPos.offset >= comment(docPos.entry).length() )
//             {
//                 endReached=true;
//             }
//             break;
//         case UndefPart:
//             break;
//         }
//     }

    while (!success)
    {
        int accelMarkerPos = -1;
        int contextInfoLength = 0;
        int contextInfoPos = -1;
        QString targetStr;

        switch (docPos.part)
        {
        case Msgid:
            // FIXME: should care about plural forms in msgid
            targetStr = msgid(docPos.entry);
            break;
        case Msgstr:
            targetStr = msgstr(docPos.entry,docPos.form);
            break;
        case Comment:
            targetStr = comment(docPos.entry);
            break;
        case UndefPart:
            break;
        }

//         if (findOpts->ignoreContextInfo)
//         {
//             contextInfoPos = miscOptions.contextInfo.indexIn( targetStr );
//             contextInfoLength = miscOptions.contextInfo.matchedLength();
//             if (contextInfoPos >= 0)
//             {
//                 targetStr.remove(contextInfoPos,contextInfoLength);
// 
//                 if (docPos.offset > (uint)contextInfoPos)
//                     docPos.offset -= contextInfoLength;
//             }
//         }

//         if (findOpts->ignoreAccelMarker
//                 && targetStr.contains(miscOptions.accelMarker))
//         {
//             accelMarkerPos = targetStr.indexOf( miscOptions.accelMarker );
//             targetStr.remove(accelMarkerPos,1);
// 
//             if (docPos.offset > (uint)accelMarkerPos)
//                 docPos.offset--;
//         }
// 
//         if ( findOpts->isRegExp )
//         {
//             if ( ( pos = regexp.indexIn( targetStr, docPos.offset ) ) >= 0 )
//             {
//                 len = regexp.matchedLength();
//                 if (findOpts->wholeWords)
//                 {
//                     QString pre=targetStr.mid(pos-1,1);
//                     QString post=targetStr.mid(pos+len,1);
//                     if (!pre.contains(QRegExp("[a-zA-Z0-9]")) && !post.contains(QRegExp("[a-zA-Z0-9]")) )
//                     {
//                         success=true;
//                         docPos.offset=pos;
//                     }
//                 }
//                 else
//                 {
//                     success=true;
//                     docPos.offset=pos;
//                 }
//             }
//         }
//         else
        {
            if ( ( pos = targetStr.indexOf( searchStr, docPos.offset, /*( findOpts->caseSensitive ?*/ Qt::CaseSensitive/* : Qt::CaseInsensitive )*/ ) ) >= 0 )
            {
                len=searchStr.length();

//                 if (findOpts->wholeWords)
//                 {
//                     QString pre=targetStr.mid(pos-1,1);
//                     QString post=targetStr.mid(pos+len,1);
//                     if (!pre.contains(QRegExp("[a-zA-Z0-9]")) && !post.contains(QRegExp("[a-zA-Z0-9]")) )
//                     {
//                         success=true;
//                         docPos.offset=pos;
//                     }
//                 }
//                 else
                {
                    success=true;
                    docPos.offset=pos;
                }
            }
        }


        if (!success)
        {
            docPos.offset=0;
            switch (docPos.part)
            {
            case Msgid:
            {
                if (findOpts->inMsgstr)
                {
                    docPos.part = Msgstr;
                    docPos.form = 0;
                }
                else if (findOpts->inComment)
                {
                    docPos.part = Comment;
                }
                else
                {
                    if (docPos.entry >= numberOfEntries()-1)
                    {
                        endReached=true;
                    }
                    else
                    {
                        docPos.entry++;
                        docPos.form = 0;
                    }
                }
                break;
            }
            case Msgstr:
                if ( (int)docPos.form < numberOfPluralForms(docPos.entry)-1 && pluralFormType(docPos.entry)==Gettext)
                {
                    docPos.form++;
                }
                else
                    if (findOpts->inComment)
                    {
                        docPos.part = Comment;
                    }
                    else if (findOpts->inMsgid)
                    {
                        if (docPos.entry >= numberOfEntries()-1)
                        {
                            endReached=true;
                        }
                        else
                        {
                            docPos.part = Msgid;
                            docPos.entry++;
                        }
                    }
                    else
                    {
                        if (docPos.entry >= numberOfEntries()-1)
                        {
                            endReached=true;
                        }
                        else
                        {
                            docPos.entry++;
                            docPos.form = 0;
                        }
                    }
                break;
            case Comment:
                if (findOpts->inMsgid)
                {
                    if (docPos.entry >= numberOfEntries()-1)
                    {
                        endReached=true;
                    }
                    else
                    {
                        docPos.part = Msgid;
                        docPos.entry++;
                        docPos.form = 0;
                    }
                }
                else if (findOpts->inMsgstr)
                {
                    if (docPos.entry >= numberOfEntries()-1)
                    {
                        endReached=true;
                    }
                    else
                    {
                        docPos.part = Msgstr;
                        docPos.form = 0;
                        docPos.entry++;
                    }
                }
                else
                {
                    if (docPos.entry >= numberOfEntries()-1)
                    {
                        endReached=true;
                    }
                    else
                    {
                        docPos.entry++;
                        docPos.form = 0;
                    }
                }
                break;
            case UndefPart:
                break;
            }
        }
        else
        {
            if (accelMarkerPos >= 0)
            {
                if (docPos.offset >= (uint)accelMarkerPos)
                {
                    docPos.offset++;
                }
                else if (docPos.offset+len > (uint)accelMarkerPos)
                {
                    len++;
                }
            }

            if (contextInfoPos >= 0)
            {
                if (docPos.offset >= (uint)contextInfoPos)
                {
                    docPos.offset+=contextInfoLength;
                }
                else if (docPos.offset+len > (uint)contextInfoPos)
                {
                    len+=contextInfoLength;
                }

            }
        }
    }

    disconnect( this, SIGNAL( signalStopActivity() ), this, SLOT( stopInternal() ));
    kDebug(KBABEL) << "findNext not active" << endl;
    d->_active=false;
    d->_stop=false;

    return true;
}
#endif








#include "catalog.moc"



