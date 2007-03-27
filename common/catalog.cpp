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

#include <kglobal.h>
#include <klocale.h>
#include <k3process.h>
#include <ktemporaryfile.h>
#include <QTextCodec>
#include <QTextStream>

#include <stdlib.h>



#include <QString>
#include <QMap>

#include <kdebug.h>
#include <kio/netaccess.h>
#include <kmessagebox.h>
#include <kdatetime.h>

#include "gettextimport.h"
#include "catalog.h"
#include "version.h"
#include "settings.h"

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

bool Catalog::setNumberOfPluralFormsFromHeader()
{
    QRegExp rxplural("Plural-Forms:\\s*nplurals=(.);");
    if (rxplural.indexIn(d->_header.msgstr()) == -1)
        return false;
    d->_numberOfPluralForms = rxplural.cap(1).toShort();
    return true;

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
      d->_header.setMsgstr(values);
      
      updateHeader(false);
      
      if (!setNumberOfPluralFormsFromHeader())
      {
          if (d->_generatedFromDocbook)
              d->_numberOfPluralForms=1;
          else
              kWarning() << "No plural form info in header" << endl;
//           d->_numberOfPluralForms=2;
      }

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
        status = importer.open(target,QString("text/x-gettext-translation"),this);
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
}

DocPosition& Catalog::redo()
{
    QUndoStack::redo();
    return d->_posBuffer;
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







QString GNUPluralForms(const QString& lang);


void Catalog::updateHeader(bool forSaving)
{
    QStringList headerList=d->_header.msgstrAsList();
    QStringList commentList = d->_header.comment().split( '\n', QString::SkipEmptyParts );

    QStringList::Iterator it,ait;
    QString temp;
    bool found;

    KConfig* identityOptions = Settings::self()->config();
    identityOptions->setGroup("Identity");

    //    const SaveSettings saveOptions = saveSettings();


    found=false;

    temp="Last-Translator: "+identityOptions->readEntry("authorName","");
    if (!identityOptions->readEntry("Email","").isEmpty())
        temp+=(" <"+identityOptions->readEntry("Email")+'>');
    temp+="\\n";

    for ( it = headerList.begin(); it != headerList.end(); ++it )
    {
        if (it->contains(QRegExp("^ *Last-Translator:.*")))
        {
            if (forSaving)
                (*it) = temp;
            found=true;
        }
    }

    if (!found)
        headerList.append(temp);

    //    QString dateTimeString = KDateTime::currentUtcDateTime().toString("yyyy-MM-dd hh:mmz");
    QString dateTimeString =
        KDateTime::currentUtcDateTime().dateTime().toString("yyyy-MM-dd hh:mm %z");

    found=false;

    temp="PO-Revision-Date: "+dateTimeString+"\\n";

    for ( it = headerList.begin(); it != headerList.end(); ++it )
    {
        if (it->contains(QRegExp("^ *PO-Revision-Date:.*")))
        {
            if (forSaving)
                (*it) = temp;
            found=true;
        }
    }
    if (!found)
        headerList.append(temp);

    found=false;

    temp="Project-Id-Version: "+d->_url.fileName()+"\\n";
    temp.remove(".pot");
    temp.remove(".po");
    //temp.replace( "@PACKAGE@", packageName());

    for ( it = headerList.begin(); it != headerList.end(); ++it )
    {
        if (it->contains(QRegExp("^ *Project-Id-Version:.*")))
        {
            if (it->contains("PACKAGE VERSION"))
                (*it) = temp;
            found=true;
        }
    }
    if (!found)
        headerList.append(temp);

    found=false;
    KLocale locale("kdelibs");
    locale.setLanguage("en_US");
    d->_langCode=identityOptions->readEntry("DefaultLangCode",KGlobal::locale()->languageList().first());

    for ( it = headerList.begin(); it != headerList.end(); ++it )
    {
        if (it->contains(QRegExp("^ *Language-Team:.*")))
        {
            found=true;
            //really parse header
            QMap<QString,QString> map;
            QStringList langlist = KGlobal::locale()->allLanguagesTwoAlpha();
            QStringList::const_iterator myit;
            for (myit=langlist.begin();myit!=langlist.end();++myit)
                map[locale.twoAlphaToLanguageName(*myit)]=*myit;

            QRegExp re("^ *Language-Team: *(.*) *<");
            QString val;
            if ((re.indexIn(*it) != -1) && (map.contains( val=re.cap(1).trimmed() )))
                d->_langCode=map.value(val);

            ait=it;
        }
    }
    d->_language=locale.twoAlphaToLanguageName(d->_langCode);

    temp="Language-Team: "+d->_language;
    if (!identityOptions->readEntry("DefaultMailingList").isEmpty())
        temp+=(" <"+identityOptions->readEntry("DefaultMailingList")+'>');
    temp+="\\n";

//     kWarning()<< "  _'" << temp <<"' " <<endl;

    if (found)
        (*ait) = temp;
    else
        headerList.append(temp);


//     if(!usePrefs || saveOptions.updateCharset)
//     {
//         found=false;
//
//         QString encodingStr;
//         if(saveOptions.useOldEncoding && d->fileCodec)
//         {
//             encodingStr = charsetString(d->fileCodec);
//         }
//         else
//         {
//             encodingStr=charsetString(saveOptions.encoding);
//         }
//         it = headerList.begin();
//         while( it != headerList.end() )
//         {
//             if( it->contains( QRegExp( "^ *Content-Type:.*" ) ) )
//             {
//                 if ( found )
//                 {
//                     // We had already a Content-Type, so we do not need a duplicate
//                     it = headerList.erase( it );
//                 }
//                 else
//                 {
//                     found=true;
//                     QRegExp regexp( "^ *Content-Type:(.*/.*);?\\s*charset=.*$" );
//                     QString mimeType;
//                     if ( regexp.indexIn( *it )!=-1 )
//                     {
//                         mimeType = regexp.cap( 1 ).trimmed();
//                     }
//                     if ( mimeType.isEmpty() )
//                     {
//                         mimeType = "text/plain";
//                     }
//                     temp = "Content-Type: ";
//                     temp += mimeType;
//                     temp += "; charset=";
//                     temp += encodingStr;
//                     temp += "\\n";
//                     (*it) = temp;
//                 }
//             }
//             ++it;
//         }
//         if(!found)
//         {
//             headerList.append(temp);
//         }
//     }
    found=false;

    temp="Content-Transfer-Encoding: 8bit\\n";

    for ( it = headerList.begin(); it != headerList.end(); ++it )
    {
        if (it->contains(QRegExp("^ *Content-Transfer-Encoding:.*")))
            found=true;
    }
    if (!found)
        headerList.append(temp);


    temp="X-Generator: KAider %1\\n";
    temp=temp.arg(KAIDER_VERSION);
    found=false;

    for ( it = headerList.begin(); it != headerList.end(); ++it )
    {
        if (it->contains(QRegExp("^ *X-Generator:.*")))
        {
            (*it) = temp;
            found=true;
        }
    }
    if (!found)
        headerList.append(temp);

    // ensure MIME-Version header
    temp="MIME-Version: 1.0\\n";
    found=false;
    for ( it = headerList.begin(); it != headerList.end(); ++it )
    {
        if (it->contains(QRegExp("^ *MIME-Version:")))
        {
            (*it) = temp;
            found=true;
        }
    }
    if ( !found )
    {
        headerList.append(temp);
    }


    found=false;

    // update plural form header
    for ( it = headerList.begin(); it != headerList.end(); ++it )
    {
        if (it->contains(QRegExp("^ *Plural-Forms:")))
            found=true;
    }
    if ( !found && !d->_generatedFromDocbook)
    {
        QString t= GNUPluralForms(d->_langCode);
        if ( !t.isEmpty() )
        {

            temp="Plural-Forms: %1\\n";
            temp=temp.arg(t);
            headerList.append(temp);
        }
    }

    d->_header.setMsgstr( headerList.join( "\n" ) );

    //comment = description, copyrights
    found=false;

    for ( it = commentList.begin(); it != commentList.end(); ++it )
    {
        // U+00A9 is the Copyright sign
        if ( it->contains( QRegExp("^# *Copyright (\\(C\\)|\\x00a9).*Free Software Foundation, Inc") ) )
        {
            found=true;
            break;
        }
    }
    if (found)
    {
        if ( it->contains( QRegExp("^# *Copyright (\\(C\\)|\\x00a9) YEAR Free Software Foundation, Inc\\.") ) )
        {
            //template string
//     		if( saveOptions.FSFCopyright == ProjectSettingsBase::Remove)
            it->remove(" YEAR Free Software Foundation, Inc");
            /*		else
            		    it->replace("YEAR", QDate::currentDate().toString("yyyy"));*/
        } /*else
                        	    if( saveOptions.FSFCopyright == ProjectSettingsBase::Update )
                        	    {
                        		    //update years
                        		    QString cy = QDate::currentDate().toString("yyyy");
                        		    if( !it->contains( QRegExp(cy)) ) // is the year already included?
                        		    {
                        			int index = it->lastIndexOf( QRegExp("[\\d]+[\\d\\-, ]*") );
                        			if( index == -1 )
                        			{
                        			    KMessageBox::information(0,i18n("Free Software Foundation Copyright does not contain any year. "
                        			    "It will not be updated."));
                        			} else {
                        			    it->insert(index+1, QString(", ")+cy);
                        			}
                        		    }
                        	    }*/
    }
#if 0
    if ( ( !usePrefs || saveOptions.updateDescription )
            && ( !saveOptions.descriptionString.isEmpty() ) )
    {
        temp = "# "+saveOptions.descriptionString;
        temp.replace( "@PACKAGE@", packageName());
        temp.replace( "@LANGUAGE@", identityOptions.languageName);
        temp = temp.trimmed();

        // The description strings has often buggy variants already in the file, these must be removed
        QString regexpstr = "^#\\s+" + QRegExp::escape( saveOptions.descriptionString.trimmed() ) + "\\s*$";
        regexpstr.replace( "@PACKAGE@", ".*" );
        regexpstr.replace( "@LANGUAGE@", ".*" );
        //kDebug() << "REGEXPSTR: " <<  regexpstr << endl;
        QRegExp regexp ( regexpstr );

        // The buggy variants exist in English too (of a time before KBabel got a translation for the corresponding language)
        QRegExp regexpUntranslated ( "^#\\s+translation of .* to .*\\s*$" );


        kDebug () << "Temp is '" << temp << "'" << endl;

        found=false;
        bool foundTemplate=false;

        it = commentList.begin();
        while ( it != commentList.end() )
        {
            kDebug () << "testing '" << (*it) << "'" << endl;
            bool deleteItem = false;

            if ( (*it) == temp )
            {
                kDebug () << "Match " << endl;
                if ( found )
                    deleteItem = true;
                else
                    found=true;
            }
            else if ( regexp.indexIn( *it ) >= 0 )
            {
                // We have a similar (translated) string (from another project or another language (perhaps typos)). Remove it.
                deleteItem = true;
            }
            else if ( regexpUntranslated.indexIn( *it ) >= 0 )
            {
                // We have a similar (untranslated) string (from another project or another language (perhaps typos)). Remove it.
                deleteItem = true;
            }
            else if ( (*it) == "# SOME DESCRIPTIVE TITLE." )
            {
                // We have the standard title placeholder, remove it
                deleteItem = true;
            }

            if ( deleteItem )
                it = commentList.erase( it );
            else
                ++it;
        }
        if (!found) commentList.prepend(temp);
    }
#endif
    // kDebug() << "HEADER COMMENT: " << commentList << endl;

    /*    if ( (!usePrefs || saveOptions.updateTranslatorCopyright)
            && ( ! identityOptions->readEntry("authorName","").isEmpty() )
            && ( ! identityOptions->readEntry("Email","").isEmpty() ) ) // An email address can be used as ersatz of a name
        {*/
//                        return;
    QStringList foundAuthors;

    temp = "# ";
    temp += identityOptions->readEntry("authorName","");
    if (!identityOptions->readEntry("Email","").isEmpty())
    {
        temp+=(" <"+identityOptions->readEntry("Email","")+'>');
    }
    temp+=", "+QDate::currentDate().toString("yyyy")+'.';

    // ### TODO: it would be nice if the entry could start with "COPYRIGHT" and have the "(C)" symbol (both not mandatory)
    QRegExp regexpAuthorYear( "^#.*(<.+@.+>)?,\\s*([\\d]+[\\d\\-, ]*|YEAR)" );
    QRegExp regexpYearAlone( "^# , \\d{4}.?\\s*$" );
    if (commentList.isEmpty())
    {
        commentList.append(temp);
        commentList.append(QString());
    }
    else
    {
        it = commentList.begin();
        while ( it != commentList.end() )
        {
            bool deleteItem = false;
            if ( it->indexOf( "copyright", 0, Qt::CaseInsensitive ) != -1 )
            {
                // We have a line with a copyright. It should not be moved.
            }
            else if ( it->contains( QRegExp("#, *fuzzy") ) )
                deleteItem = true;
            else if ( it->contains( regexpYearAlone ) )
            {
                // We have found a year number that is preceeded by a comma.
                // That is typical of KBabel 1.10 (and earlier?) when there is neither an author name nor an email
                // Remove the entry
                deleteItem = true;
            }
            else if ( it->contains( "# FIRST AUTHOR <EMAIL@ADDRESS>, YEAR.") )
                deleteItem = true;
            else if ( it->contains( "# SOME DESCRIPTIVE TITLE"))
                deleteItem = true;
            else if ( it->contains( regexpAuthorYear ) ) // email address followed by year
            {
                if ( !foundAuthors.contains( (*it) ) )
                {
                    // The author line is new (and not a duplicate), so add it to the author line list
                    foundAuthors.append( (*it) );
                }
                // Delete also non-duplicated entry, as now all what is needed will be processed in foundAuthors
                deleteItem = true;
            }

            if ( deleteItem )
                it = commentList.erase( it );
            else
                ++it;
        }

        if ( !foundAuthors.isEmpty() )
        {
            found = false;
            bool foundAuthor = false;

            const QString cy = QDate::currentDate().toString("yyyy");

            ait = foundAuthors.end();
            for ( it = foundAuthors.begin() ; it!=foundAuthors.end(); ++it )
            {
                if ( it->indexOf( QRegExp(
                                      QRegExp::escape( identityOptions->readEntry("authorName","") )+".*"
                                      + QRegExp::escape( identityOptions->readEntry("Email","") ) ) ) != -1 )
                {
                    foundAuthor = true;
                    if ( it->indexOf( cy ) != -1 )
                        found = true;
                    else
                        ait = it;
                }
            }
            if ( !found )
            {
                if ( !foundAuthor )
                    foundAuthors.append(temp);
                else if ( ait != foundAuthors.end() )
                {
                    //update years
                    const int index = (*ait).lastIndexOf( QRegExp("[\\d]+[\\d\\-, ]*") );
                    if ( index == -1 )
                        (*ait)+=", "+cy;
                    else
                        (*ait).insert(index+1, QString(", ")+cy);
                }
                else
                    kDebug() << "INTERNAL ERROR: author found but iterator dangling!" << endl;
            }

        }
        else
            foundAuthors.append(temp);

//         kWarning() << foundAuthors << endl;
//         kWarning() << commentList << endl;


        for (ait=foundAuthors.begin();ait!=foundAuthors.end();++ait)
        {
            QString s = (*ait);

            // ensure dot at the end of copyright
            if ( !s.endsWith(".") ) s += '.';
            commentList.append(s);
        }
    }
    d->_header.setComment( commentList.join( "\n" ) );

}







QString GNUPluralForms(const QString& lang)
{
    KTemporaryFile infile, outfile;
    infile.open();
    outfile.open();

    QTextStream str ( &infile );

    str << "# SOME DESCRIPTIVE TITLE." << endl;
    str << "# Copyright (C) YEAR Free Software Foundation, Inc." << endl;
    str << "# FIRST AUTHOR <EMAIL@ADDRESS>, YEAR." << endl;
    str << "#" << endl;
    str << "#, fuzzy" << endl;
    str << "msgid \"\"" << endl;
    str << "msgstr \"\"" << endl;
    str << "\"Project-Id-Version: PACKAGE VERSION\\n\"" << endl;
    str << "\"POT-Creation-Date: 2002-06-25 03:23+0200\\n\"" << endl;
    str << "\"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\\n\"" << endl;
    str << "\"Last-Translator: FULL NAME <EMAIL@ADDRESS>\\n\"" << endl;
    str << "\"Language-Team: LANGUAGE <LL@li.org>\\n\"" << endl;
    str << "\"MIME-Version: 1.0\\n\"" << endl;
    str << "\"Content-Type: text/plain; charset=UTF-8\\n\"" << endl;
    str << "\"Content-Transfer-Encoding: ENCODING\\n\"" << endl;

    str.flush();

    K3Process msginit;

    msginit << "msginit";
    msginit
    << "-l"
    << lang
    << "-i"
    << infile.fileName()
    << "-o"
    << outfile.fileName()
    << "--no-translator"
    << "--no-wrap" ;

    msginit.start( K3Process::Block );

    QString res("");

    if ( msginit.normalExit() )
    {
        // parse out the plural form string
        outfile.seek(0);
        QTextStream str(&outfile);

        QString line;
        do
        {
            line = str.readLine();

            if ( line.startsWith( "\"Plural-Forms:" ) )
            {
                kDebug() << "Plural form line: " << line << endl;
                QRegExp re( "^\"Plural-Forms: *(.*)\\\\n\"" );
                re.indexIn( line );
                res = re.cap(1);
                break;
            }
        }
        while (!str.atEnd() );
    }

    return res;
}


#include "catalog.moc"



