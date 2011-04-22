/* ****************************************************************************
  This file is part of Lokalize
  This file is based on the one from KBabel

  Copyright (C) 1999-2000 by Matthias Kiefer <matthias.kiefer@gmx.de>
                2001-2003 by Stanislav Visnovsky <visnovsky@kde.org>
                2006 by Nicolas GOUTTE <nicolasg@snafu.de>
                2007 by Nick Shaforostoff <shafff@ukr.net>

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

#include "gettextimport.h"
//#include <resources.h>

#include <QFile>
#include <QTime>
#include <QFileInfo>
#include <QRegExp>
#include <QTextCodec>
#include <QList>
#include <QTextStream>
#include <QEventLoop>

#include <kapplication.h>
#include <kdebug.h>
#include <kgenericfactory.h>
#include <klocale.h>

#include "catalogitem.h"

using namespace GettextCatalog;

// GettextImportPlugin::GettextImportPlugin(ExtraDataSaver* extraDataSaver)
//    : CatalogImportPlugin()
//    , _extraDataSaver(extraDataSaver)
GettextImportPlugin::GettextImportPlugin()
   : CatalogImportPlugin()
   , _rxMsgCtxt          ("^msgctxt\\s*\".*\"$")
   , _rxMsgId            ("^msgid\\s*\".*\"$")
   , _rxMsgIdPlural      ("^msgid_plural\\s*\".*\"$")
   , _rxMsgIdPluralBorked("^msgid_plural\\s*\"?.*\"?$")
   , _rxMsgIdBorked      ("^msgid\\s*\"?.*\"?$")
   , _rxMsgIdRemQuotes   ("^msgid\\s*\"")
   , _rxMsgLineRemEndQuote      ("\"$")
   , _rxMsgLineRemStartQuote    ("^\"")
   , _rxMsgLine          ("^\".*\\n?\"$")
   , _rxMsgLineBorked    ("^\"?.+\\n?\"?$")
   , _rxMsgStr           ("^msgstr\\s*\".*\\n?\"$")
   , _rxMsgStrOther      ("^msgstr\\s*\"?.*\\n?\"?$")
   , _rxMsgStrPluralStart("^msgstr\\[0\\]\\s*\".*\\n?\"$")
   , _rxMsgStrPluralStartBorked ("^msgstr\\[0\\]\\s*\"?.*\\n?\"?$")
   , _rxMsgStrPlural     ("^msgstr\\[[0-9]+\\]\\s*\".*\\n?\"$")
   , _rxMsgStrPluralBorked      ("^msgstr\\[[0-9]\\]\\s*\"?.*\\n?\"?$")
   , _rxMsgStrRemQuotes  ("^msgstr\\s*\"?")
//    , _rxMsgId   ("^msgid\\s*\"?.*\"?$")
   , _obsoleteStart("#~")
   , _msgctxtStart("msgctxt")

{
}

ConversionStatus GettextImportPlugin::load(QIODevice* device)
{
   _testBorked=false;
   _errorLine=0;

   // find codec for file
   //    bool hadCodec;
   QTextCodec* codec=codecForDevice(device/*, &hadCodec*/ );
   QTextStream stream(device);
   stream.seek(0);
   stream.setCodec(codec);

   //QIODevice *dev = stream.device();
   //int fileSize = dev->size();

   // if somethings goes wrong with the parsing, we don't have deleted the old contents
   CatalogItem tempHeader;

   kDebug() << "start parsing...";
   QTime aaa;
   aaa.start();
   // first read header
   const ConversionStatus status = readEntry(stream);

   bool recoveredErrorInHeader = false;
   if (KDE_ISUNLIKELY( status == RECOVERED_PARSE_ERROR ))
   {
      kDebug() << "Recovered error in header entry";
      recoveredErrorInHeader = true;
   }
   else if (KDE_ISUNLIKELY( status != OK ))
   {
      kWarning() << "Parse error in header entry";
      return status;
   }

   bool reconstructedHeader=!_msgid.isEmpty() && !_msgid.first().isEmpty();
   //kWarning() << "HEADER MSGID: " << _msgid;
   //kWarning() << "HEADER MSGSTR: " << _msgstr;
   if (KDE_ISUNLIKELY( reconstructedHeader ))
   {
      // The header must have an empty msgid
      kWarning() << "Header entry has non-empty msgid. Creating a temporary header! " << _msgid;
      tempHeader.setMsgid( QString() );
      QString tmp(
         "Content-Type: text/plain; charset=UTF-8\\n" // Unknown charset
         "Content-Transfer-Encoding: 8bit\\n"
         "Mime-Version: 1.0" );
      tempHeader.setMsgstr( tmp);
      // We keep the comment of the first entry, as it might really be a header comment (at least partially)
      const QString comment( "# Header entry was created by Lokalize.\n#\n" + _comment );
      tempHeader.setComment( comment );
      recoveredErrorInHeader = true;
   }
   else
   {
      tempHeader.setMsgid( _msgid );
      tempHeader.setMsgstr( _msgstr );
      tempHeader.setComment( _comment );
   }
//    if(tempHeader.isFuzzy())
//    {
//       tempHeader.removeFuzzy();
//    }

   // check if header seems to indicate docbook content generated by xml2pot
   const bool docbookContent = tempHeader.msgstr().contains( "application/x-xml2pot" );







   // now parse the rest of the file
   uint counter=0;
   QList<int> errorIndex;
   bool recoveredError=false;
   bool docbookFile=false;

   ExtraDataSaver _extraDataSaver;
   ConversionStatus success=OK;
   while( !stream.atEnd() )
   {
      if (reconstructedHeader)
         reconstructedHeader=false;
      else
         success=readEntry(stream);

      if(KDE_ISLIKELY(success==OK))
      {
         if( _obsolete )
            _extraDataSaver(_comment);
         else
         {
            CatalogItem tempCatItem;
            tempCatItem.setPlural(_gettextPluralForm);
            tempCatItem.setMsgid( _msgid, _msgidMultiline );
            tempCatItem.setMsgstr( _msgstr, _msgstrMultiline );
            tempCatItem.setMsgctxt( _msgctxt );
            tempCatItem.setComment( _comment );

               // add new entry to the list of entries
               appendCatalogItem(tempCatItem);
               // check if first comment seems to indicate a docbook source file
               if(counter==0)
                  docbookFile = tempCatItem.comment().contains(".docbook" );
         }
      }
      else if(KDE_ISUNLIKELY( success==RECOVERED_PARSE_ERROR ))
      {
         kDebug() << "Recovered parse error in entry: " << counter;
         recoveredError=true;
         errorIndex.append(counter);

            CatalogItem tempCatItem;
            tempCatItem.setPlural(_gettextPluralForm);
            tempCatItem.setMsgid( _msgid );
            tempCatItem.setMsgstr( _msgstr );
            tempCatItem.setMsgctxt( _msgctxt );
            tempCatItem.setComment( _comment );


         // add new entry to the list of entries
         appendCatalogItem(tempCatItem);
      }
      else if (success == PARSE_ERROR)
      {
         kDebug() << "Parse error in entry: " << counter;
         return PARSE_ERROR;
      }
      else
      {
         kDebug() << "Unknown success status, assumig parse error " << success;
         return PARSE_ERROR;
      }
      counter++;

   }

   // TODO: can we check that there is no useful entry?
   if (KDE_ISUNLIKELY( !counter && !recoveredErrorInHeader ))
   {
      // Empty file? (Otherwise, there would be a try of getting an entry and the count would be 1 !)
      kDebug() << " Empty file?";
      return PARSE_ERROR;
   }

   kDebug() << " ready";

   // We have successfully loaded the file (perhaps with recovered errors)

//    qWarning() << " done in " << aaa.elapsed() <<_extraDataSaver->extraData.size() << endl;

   setGeneratedFromDocbook(docbookContent || docbookFile);
   setHeader(tempHeader);
   setCatalogExtraData(_extraDataSaver.extraData);
   setErrorIndex(errorIndex);
   //setFileCodec(codec);
   //setMimeTypes( "text/x-gettext-translation" );
#if 0
   if (KDE_ISUNLIKELY( recoveredErrorInHeader ))
   {
      kDebug() << " Returning: header error";
      return RECOVERED_HEADER_ERROR;
   }
   else if (KDE_ISUNLIKELY( recoveredError ))
   {
      kDebug() << " Returning: recovered parse error";
      return RECOVERED_PARSE_ERROR;
   }
   else
#endif
   {
      kDebug() << " Returning: OK! :-)";
      return OK;
   }
}

QTextCodec* GettextImportPlugin::codecForDevice(QIODevice* device/*, bool* hadCodec*/)
{
    QTextStream stream( device );
    stream.seek(0);
    _errorLine=0;
    stream.setCodec( "UTF-8" );
    stream.setAutoDetectUnicode(true); //this way we can
    QTextCodec* codec=stream.codec();  //detect UTF-16

    ConversionStatus status = readEntry(stream);
    if (KDE_ISUNLIKELY( status!=OK && status != RECOVERED_PARSE_ERROR ))
    {
        kDebug() << "wasn't able to read header";
        return codec;
    }

    QRegExp regexp("Content-Type:\\s*\\w+/[-\\w]+;?\\s*charset\\s*=\\s*(\\S+)\\s*\\\\n");
    if ( regexp.indexIn( _msgstr.first() ) == -1 )
    {
        kDebug() << "no charset entry found";
        return codec;
    }

    const QString charset = regexp.cap(1);
    kDebug() << "charset: " << charset;

    if (charset.isEmpty())
    {
        kWarning() << "No charset defined! Assuming UTF-8!";
        return codec;
    }

    // "CHARSET" is the default charset entry in a template (pot).
    // characters in a template should be either pure ascii or
    // at least utf8, so utf8-codec can be used for both.
    if ( charset.contains("CHARSET"))
    {
        kDebug() << QString("file seems to be a template: using utf-8 encoding.");
        return QTextCodec::codecForName("utf8");;
    }

    QTextCodec* t=0;
    t = QTextCodec::codecForName(charset.toLatin1());

    if (t)
        return t;
    else
       kWarning() << "charset found, but no codec available, using UTF-8 instead";

    return codec;//UTF-8
}

ConversionStatus GettextImportPlugin::readEntry(QTextStream& stream)
{
   ConversionStatus result=readEntryRaw(stream);
   _msgstr.replaceInStrings("\\\"","\"");
   _msgid.replaceInStrings("\\\"","\"");
   _msgctxt.replace("\\\"","\"");
   return result;
}

ConversionStatus GettextImportPlugin::readEntryRaw(QTextStream& stream)
{
   //kDebug() << " START";
   enum {Begin,Comment,Msgctxt,Msgid,Msgstr} part=Begin;

   _trailingNewLines=0;
   bool error=false;
   bool recoverableError=false;
   bool seenMsgctxt=false;
   _msgstr.clear();
   _msgstr.append(QString());
   _msgid.clear();
   _msgid.append(QString());
   _msgctxt.clear();
   _comment.clear();
   _gettextPluralForm=false;
   _obsolete=false;

   QStringList::Iterator msgstrIt=_msgstr.begin();
   QString line;

   while( !stream.atEnd() )
   {
       _errorLine++;
       //line=stream.readLine();
       if (!_bufferedLine.isEmpty())
       {
            line=_bufferedLine;
            _bufferedLine.clear();
       }
       else
           line=stream.readLine();

       kDebug() << "Parsing line: " << line;

       static const QString lesslessless="<<<<<<<";
       static const QString isisis="=======";
       static const QString moremoremore=">>>>>>>";
       if (KDE_ISUNLIKELY( line.startsWith( lesslessless ) || line.startsWith( isisis ) || line.startsWith( moremoremore ) ))
       {
          // We have found a CVS/SVN conflict marker. Abort.
          // (It cannot be any useful data of the PO file, as otherwise the line would start with at least a quote)
          kError() << "CVS/SVN conflict marker found! Aborting!" << endl << line << endl;
          return PARSE_ERROR;
       }

       // remove whitespaces from beginning and end of line
       line = line.trimmed();

       // remember wrapping state to save file nicely
       int len=line.length();
       if (len)
       {
          _trailingNewLines=0;
         if (_maxLineLength<len && line.at(0)!='#')
           _maxLineLength=len;
       }
       else
          ++_trailingNewLines;


       if(part==Begin)
       {
           // ignore trailing newlines
           if(!len)
              continue;

           if(line.startsWith(_obsoleteStart))
           {
               _obsolete=true;
               part=Comment;
               _comment=line;
           }
           else if(line.startsWith('#'))
           {
               part=Comment;
               _comment=line;
           }
           else if( line.startsWith(_msgctxtStart) && line.contains( _rxMsgCtxt ) )
           {
               part=Msgctxt;

               // remove quotes at beginning and the end of the lines
               line.remove(QRegExp("^msgctxt\\s*\""));
               line.remove(_rxMsgLineRemEndQuote);
               _msgctxt=line;
               seenMsgctxt=true;
           }
           else if( line.contains( _rxMsgId ) )
           {
               part=Msgid;

               // remove quotes at beginning and the end of the lines
               line.remove(_rxMsgIdRemQuotes);
               line.remove(_rxMsgLineRemEndQuote);

               (*(_msgid).begin())=line;

           }
           // one of the quotation marks is missing
           else if(KDE_ISUNLIKELY( /*_testBorked&&*/ line.contains( _rxMsgIdBorked ) ))
           {
               part=Msgid;

               // remove quotes at beginning and the end of the lines
               line.remove(QRegExp("^msgid\\s*\"?"));
               line.remove(_rxMsgLineRemEndQuote);

               (*(_msgid).begin())=line;

               if(!line.isEmpty())
                  recoverableError=true;
           }
           else
           {
               kDebug() << "no comment, msgctxt or msgid found after a comment: " << line;
               error=true;
               break;
           }
       }
       else if(part==Comment)
       {
            if(!len && _obsolete ) return OK;
            if(!len) continue;
            else if(line.startsWith(_obsoleteStart))
            {
               _comment+=('\n'+line);
               _obsolete=true;
            }
            else if(line.startsWith('#'))
            {
               _comment+=('\n'+line);
            }
            else if( line.startsWith(_msgctxtStart) &&line.contains( _rxMsgCtxt ) )
            {
               part=Msgctxt;

               // remove quotes at beginning and the end of the lines
               line.remove(QRegExp("^msgctxt\\s*\""));
               line.remove(_rxMsgLineRemEndQuote);
               _msgctxt=line;
               seenMsgctxt=true;
            }
            else if( line.contains( _rxMsgId ) )
            {
               part=Msgid;

               // remove quotes at beginning and the end of the lines
               line.remove(_rxMsgIdRemQuotes);
               line.remove(_rxMsgLineRemEndQuote);

               (*(_msgid).begin())=line;
            }
            // one of the quotation marks is missing
            else if(KDE_ISUNLIKELY( /*_testBorked&&*/line.contains( _rxMsgIdBorked ) ))
            {
               part=Msgid;

               // remove quotes at beginning and the end of the lines
               line.remove(QRegExp("^msgid\\s*\"?"));
               line.remove(_rxMsgLineRemEndQuote);

               (*(_msgid).begin())=line;

               if(!line.isEmpty())
                     recoverableError=true;
            }
            else
            {
               kDebug() << "no comment or msgid found after a comment while parsing: " << _comment;
               error=true;
               break;
            }
        }
        else if(part==Msgctxt)
        {
            if(!len)
               continue;
            else if( line.contains( _rxMsgLine ) )
            {
               // remove quotes at beginning and the end of the lines
               line.remove(_rxMsgLineRemStartQuote);
               line.remove(_rxMsgLineRemEndQuote);

               // add Msgctxt line to item
               if(_msgctxt.isEmpty())
                  _msgctxt=line;
               else
                  _msgctxt+=('\n'+line);
            }
            else if( line.contains( _rxMsgId ) )
            {
               part=Msgid;

               // remove quotes at beginning and the end of the lines
               line.remove(_rxMsgIdRemQuotes);
               line.remove(_rxMsgLineRemEndQuote);

               _msgidMultiline=line.isEmpty();
               (*(_msgid).begin())=line;
            }
            // one of the quotation marks is missing
            else if(KDE_ISUNLIKELY(/*_testBorked&&*/ line.contains ( _rxMsgIdBorked ) ))
            {
               part=Msgid;

               // remove quotes at beginning and the end of the lines
               line.remove(QRegExp("^msgid\\s*\"?"));
               line.remove(_rxMsgLineRemEndQuote);

               (*(_msgid).begin())=line;

               if(!line.isEmpty())
                     recoverableError=true;
            }
            else
            {
               kDebug() << "no msgid found after a msgctxt while parsing: " << _msgctxt;
               error=true;
               break;
            }
        }
        else if(part==Msgid)
        {
            if(!len)
               continue;
            else if( line.contains( _rxMsgLine ) )
            {
               // remove quotes at beginning and the end of the lines
               line.remove(_rxMsgLineRemStartQuote);
               line.remove(_rxMsgLineRemEndQuote);

               QStringList::Iterator it;
               if(_gettextPluralForm)
               {
                   it=_msgid.end();
                   --it;
               }
               else
                   it = _msgid.begin();

               // add Msgid line to item
               if(it->isEmpty())
                  (*it)=line;
               else
                  (*it)+=('\n'+line);
            }
            else if( line.contains( _rxMsgIdPlural) )
            {
               part=Msgid;
               _gettextPluralForm = true;

               // remove quotes at beginning and the end of the lines
               line.remove(QRegExp("^msgid_plural\\s*\""));
               line.remove(_rxMsgLineRemEndQuote);

               _msgid.append(line);
            }
            // one of the quotation marks is missing
            else if(KDE_ISUNLIKELY(/*_testBorked&&*/ line.contains( _rxMsgIdPluralBorked ) ))
            {
               part=Msgid;
               _gettextPluralForm = true;

               // remove quotes at beginning and the end of the lines
               line.remove(QRegExp("^msgid_plural\\s*\"?"));
               line.remove(_rxMsgLineRemEndQuote);

               _msgid.append(line);

               if(!line.isEmpty())
                  recoverableError=true;
            }
            else if( !_gettextPluralForm && ( line.contains( _rxMsgStr ) ) )
            {
               part=Msgstr;

               // remove quotes at beginning and the end of the lines
               line.remove(_rxMsgStrRemQuotes);
               line.remove(_rxMsgLineRemEndQuote);

               _msgstrMultiline=line.isEmpty();
               (*msgstrIt)=line;
            }
            else if( !_gettextPluralForm && ( line.contains( _rxMsgStrOther )) )
            {
               part=Msgstr;

               // remove quotes at beginning and the end of the lines
               line.remove(_rxMsgStrRemQuotes);
               line.remove(_rxMsgLineRemEndQuote);

               (*msgstrIt)=line;

               if(!line.isEmpty())
                  recoverableError=true;
            }
            else if( _gettextPluralForm && ( line.contains( _rxMsgStrPluralStart ) ) )
            {
               part=Msgstr;

               // remove quotes at beginning and the end of the lines
               line.remove(QRegExp("^msgstr\\[0\\]\\s*\"?"));
               line.remove(_rxMsgLineRemEndQuote);

               (*msgstrIt)=line;
            }
            else if(KDE_ISUNLIKELY( /*_testBorked&&*/ _gettextPluralForm &&  line.contains( _rxMsgStrPluralStartBorked ) ))
            {
               part=Msgstr;

               // remove quotes at beginning and the end of the lines
               line.remove(QRegExp("^msgstr\\[0\\]\\s*\"?"));
               line.remove(_rxMsgLineRemEndQuote);

               (*msgstrIt)=line;

               if(!line.isEmpty())
                  recoverableError=true;
            }
            else if ( line.startsWith( '#' ) )
            {
               // ### TODO: could this be considered recoverable?
               kDebug() << "comment found after a msgid while parsing: " << _msgid.first();
               error=true;
               break;
            }
            else if ( line.startsWith( "msgid" ) )
            {
               kDebug() << "Another msgid found after a msgid while parsing: " << _msgid.first();
               error=true;
               break;
            }
            // a line of the msgid with a missing quotation mark
            else if(KDE_ISUNLIKELY( /*_testBorked&&*/line.contains( _rxMsgLineBorked ) ))
            {
               recoverableError=true;

               // remove quotes at beginning and the end of the lines
               line.remove(_rxMsgLineRemStartQuote);
               line.remove(_rxMsgLineRemEndQuote);

               QStringList::Iterator it;
               if( _gettextPluralForm )
               {
                   it=_msgid.end();
                   --it;
               }
               else
                   it = _msgid.begin();

               // add Msgid line to item
               if(it->isEmpty())
                  (*it)=line;
               else
                  (*it)+=('\n'+line);
            }
            else
            {
               kDebug() << "no msgstr found after a msgid while parsing: " << _msgid.first();
               error=true;
               break;
            }
        }
        else if(part==Msgstr)
        {
            if(!len)
               break;
            // another line of the msgstr
            else if( line.contains( _rxMsgLine ) )
            {
               // remove quotes at beginning and the end of the lines
               line.remove(_rxMsgLineRemStartQuote);
               line.remove(_rxMsgLineRemEndQuote);

               if(!(*msgstrIt).isEmpty())
                  (*msgstrIt)+='\n';
               (*msgstrIt)+=line;
            }
            else if( _gettextPluralForm && ( line.contains( _rxMsgStrPlural ) ) )
            {
               // remove quotes at beginning and the end of the lines
               line.remove(QRegExp("^msgstr\\[[0-9]+\\]\\s*\"?"));
               line.remove(_rxMsgLineRemEndQuote);

               _msgstr.append(line);
               msgstrIt=_msgstr.end();
               --msgstrIt;
            }
            else if ( line.startsWith( '#' ) || line.startsWith( "msgid" ) )
            {
               _errorLine--;
               _bufferedLine=line;
               break;
            }
            else if(KDE_ISUNLIKELY(/*_testBorked&&*/ _gettextPluralForm && ( line.contains( _rxMsgStrPluralBorked ) ) ))
            {
               // remove quotes at beginning and the end of the lines
               line.remove(QRegExp("^msgstr\\[[0-9]\\]\\s*\"?"));
               line.remove(_rxMsgLineRemEndQuote);

               _msgstr.append(line);
               msgstrIt=_msgstr.end();
               --msgstrIt;

               if(!line.isEmpty())
                  recoverableError=true;
            }
            else if(line.startsWith("msgstr"))
            {
               kDebug() << "Another msgstr found after a msgstr while parsing: " << line << _msgstr.last();
               error=true;
               break;
            }
            // another line of the msgstr with a missing quotation mark
            else if(KDE_ISUNLIKELY( /*_testBorked&&*/line.contains( _rxMsgLineBorked ) ))
            {
               recoverableError=true;

               // remove quotes at beginning and the end of the lines
               line.remove(_rxMsgLineRemStartQuote);
               line.remove(_rxMsgLineRemEndQuote);

               if(!(*msgstrIt).isEmpty())
                  (*msgstrIt)+='\n';
               (*msgstrIt)+=line;
            }
            else
            {
               kDebug() << "no msgid or comment found after a msgstr while parsing: " << _msgstr.last();
               error=true;
               break;
            }
        }
    }

/*
   if(_gettextPluralForm)
   {
       kDebug() << "gettext plural form:\n"
                 << "msgid:\n" << _msgid.first() << "\n"
                 << "msgid_plural:\n" << _msgid.last() << "\n" << endl;
       int counter=0;
       for(QStringList::Iterator it = _msgstr.begin(); it != _msgstr.end(); ++it)
       {
           kDebug() << "msgstr[" << counter << "]:\n" 
                     << (*it) << endl;
           counter++;
       }
   }
  */

    //kDebug() << " NEAR RETURN";
    if(KDE_ISUNLIKELY(error))
        return PARSE_ERROR;
    else if(KDE_ISUNLIKELY(recoverableError))
        return RECOVERED_PARSE_ERROR;
    else
        return OK;
}

// kate: space-indent on; indent-width 4; replace-tabs on;
