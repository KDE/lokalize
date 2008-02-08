/*
Copyright 2008 Nick Shaforostoff <shaforostoff@kde.ru>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of
the License or (at your option) version 3 or any later version
accepted by the membership of KDE e.V. (or its successor approved
by the membership of KDE e.V.), which shall act as a proxy 
defined in Section 14 of version 3 of the license.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "gettextstorage.h"

#include "catalogitem_private.h"
#include "gettextimport.h"
#include "gettextexport.h"

#include "project.h"

#include "version.h"
#include "prefs_lokalize.h"

#include <QProcess>
#include <QString>
#include <QMap>

#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kdatetime.h>

#include <kio/netaccess.h>
#include <ktemporaryfile.h>


GettextStorage::GettextStorage()
 : CatalogStorage()
{
}


GettextStorage::~GettextStorage()
{
}

//BEGIN OPEN/SAVE

bool GettextStorage::load(const KUrl& url)
{
    GettextImportPlugin importer;
    ConversionStatus status = OK;
    QString target;

    if(KDE_ISUNLIKELY( !KIO::NetAccess::download(url,target,NULL) ))
        return false;

    status = importer.open(target,this);
    KIO::NetAccess::removeTempFile( target );


    //for langs with more then 2 forms
    //we create any form-entries additionally needed
    uint i=0;
    uint lim=size();
    while (i<lim)
    {
        CatalogItem& item=m_entries[i];
        if (item.isPlural()
            && item.msgstrPlural().count()<m_numberOfPluralForms
           )
        {
            QVector<QString> msgstr(item.msgstrPlural());
            while (msgstr.count()<m_numberOfPluralForms)
                msgstr.append(QString());
            item.setMsgstr(msgstr);

        }
        ++i;

    }



    return status==OK;

}

bool GettextStorage::save(const KUrl& url)
{
    bool remote=false;
    KTemporaryFile tmpFile;

    updateHeader(true);

    GettextExportPlugin exporter(m_maxLineLength>70?m_maxLineLength:-1);// this is kinda hackish...

    ConversionStatus status = OK;

    QString localFile;
    if (KDE_ISLIKELY( url.isLocalFile() ))
    {
        localFile = url.path();
        if (!QFile::exists(url.directory()))
            KIO::NetAccess::mkdir(url.upUrl(),0);//recursion?..
    }
    else
    {
        remote=true;
        tmpFile.open();
        localFile=tmpFile.fileName();
        tmpFile.close();
    }

    //kWarning() << "SAVE NAME "<<localFile;
    status = exporter.save(localFile/*x-gettext-translation*/,this);

    if (KDE_ISUNLIKELY( status!=OK||remote && !KIO::NetAccess::upload( localFile, url, NULL) ))
        return false;

    return true;

}
//END OPEN/SAVE

//BEGIN STORAGE TRANSLATION

int GettextStorage::size() const
{
    return m_entries.size();
}

void GettextStorage::clear()
{}

bool GettextStorage::isEmpty() const
{
    return m_entries.isEmpty();
}

//flat-model interface (ignores XLIFF grouping)
const QString& GettextStorage::source(const DocPosition& pos) const
{
    return m_entries.at(pos.entry).msgid(pos.form);
}
const QString& GettextStorage::target(const DocPosition& pos) const
{
    return m_entries.at(pos.entry).msgstr(pos.form);
}


void GettextStorage::targetDelete(const DocPosition& pos, int count)
{
    m_entries[pos.entry].d->_msgstrPlural[pos.form].remove(pos.offset, count);
}
void GettextStorage::targetInsert(const DocPosition& pos, const QString& arg)
{
    m_entries[pos.entry].d->_msgstrPlural[pos.form].insert(pos.offset, arg);
}
void GettextStorage::setTarget(const DocPosition& pos, const QString& arg)
{
    m_entries[pos.entry].d->_msgstrPlural[pos.form]=arg;
}

QStringList GettextStorage::sourceAllForms(const DocPosition& pos) const
{
    return m_entries[pos.entry].d->_msgidPlural.toList();
}
QStringList GettextStorage::targetAllForms(const DocPosition& pos) const
{
    return m_entries[pos.entry].d->_msgstrPlural.toList();
}

//DocPosition.form - number of <note>
const QString& GettextStorage::note(const DocPosition& pos) const
{
    return m_entries.at(pos.entry).comment();
}
int GettextStorage::noteCount(const DocPosition& pos) const
{
    return m_entries.at(pos.entry).comment().isEmpty()?0:1;
}

//DocPosition.form - number of <context>
const QString& GettextStorage::context(const DocPosition& pos) const
{
    return m_entries.at(pos.entry).msgctxt();
}
int GettextStorage::contextCount(const DocPosition& pos) const
{
    return m_entries.at(pos.entry).msgctxt().isEmpty()?0:1;
}

QStringList GettextStorage::matchData(const DocPosition& pos) const
{
    QString ctxt=m_entries.at(pos.entry).msgctxt();
    //KDE-specific
    //Splits @info:whatsthis and actual note
    if (ctxt.startsWith('@') && ctxt.contains(' '))
    {
        QStringList result(ctxt.section(' ',0,0,QString::SectionSkipEmpty));
        result<<ctxt.section(' ',1,-1,QString::SectionSkipEmpty);
        return result;
    }
    return QStringList(ctxt);
}

QString GettextStorage::id(const DocPosition& pos) const
{
    QString result=source(pos);
    result+=m_entries.at(pos.entry).msgctxt();
    return result;
/*    QByteArray result=source(pos).toUtf8();
    result+=m_entries.at(pos.entry).msgctxt().toUtf8();
    return QString qChecksum(result);*/
}

bool GettextStorage::isPlural(const DocPosition& pos) const
{
    return m_entries.at(pos.entry).isPlural();
}

bool GettextStorage::isApproved(const DocPosition& pos) const
{
    return m_entries.at(pos.entry).isFuzzy();
}
void GettextStorage::setApproved(const DocPosition& pos, bool fuzzy)
{
    if (fuzzy)
        m_entries[pos.entry].setFuzzy();
    else
        m_entries[pos.entry].unsetFuzzy();
}

bool GettextStorage::isUntranslated(const DocPosition& pos) const
{
    return m_entries.at(pos.entry).isUntranslated();
}

//END STORAGE TRANSLATION




//called from importer
bool GettextStorage::setHeader(const CatalogItem& newHeader)
{
   if(newHeader.isValid())
   {
      // normalize the values - ensure every key:value pair is only on a single line
      QString values = newHeader.msgstr();
      values.replace ("\n", "");
      values.replace ("\\n", "\\n\n");

      kDebug () << "Normalized header: " << values;

      m_header=newHeader;
      m_header.setMsgstr(values);

      updateHeader(false);

//       setClean(false);
      //emit signalHeaderChanged();

      return true;
   }
   kWarning () << "header Not valid";
   return false;
}



static int numberOfPluralFormsFromHeader(const QString& header)
{
    QRegExp rxplural("Plural-Forms:\\s*nplurals=(.);");
    if (rxplural.indexIn(header) == -1)
        return 0;
    bool ok;
    int result=rxplural.cap(1).toShort(&ok);
    return ok?result:0;

}


static QString GNUPluralForms(const QString& lang)
{
//  TODO   Needs testing for M$ OS
    QString def="nplurals=2; plural=n != 1;";

    QStringList arguments;
    arguments << "-l" << lang
              << "-i" << "-"
              << "-o" << "-"
              << "--no-translator"
              << "--no-wrap";
    QProcess msginit;
    msginit.start("msginit", arguments);

    msginit.waitForStarted(5000);
    if (KDE_ISUNLIKELY( msginit.state()!=QProcess::Running ))
    {
        kWarning()<<"msginit error";
        return def;
    }

    msginit.write(
                   "# SOME DESCRIPTIVE TITLE.\n"
                   "# Copyright (C) YEAR Free Software Foundation, Inc.\n"
                   "# FIRST AUTHOR <EMAIL@ADDRESS>, YEAR.\n"
                   "#\n"
                   "#, fuzzy\n"
                   "msgid \"\"\n"
                   "msgstr \"\"\n"
                   "\"Project-Id-Version: PACKAGE VERSION\\n\"\n"
                   "\"POT-Creation-Date: 2002-06-25 03:23+0200\\n\"\n"
                   "\"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\\n\"\n"
                   "\"Last-Translator: FULL NAME <EMAIL@ADDRESS>\\n\"\n"
                   "\"Language-Team: LANGUAGE <LL@li.org>\\n\"\n"
                   "\"MIME-Version: 1.0\\n\"\n"
                   "\"Content-Type: text/plain; charset=UTF-8\\n\"\n"
                   "\"Content-Transfer-Encoding: ENCODING\\n\"\n"
//                   "\"Plural-Forms: nplurals=INTEGER; plural=EXPRESSION;\\n\"\n"
                  );
    msginit.closeWriteChannel();

    if (KDE_ISUNLIKELY( !msginit.waitForFinished(5000) ))
    {
        kWarning()<<"msginit error";
        return def;
    }


    QByteArray result = msginit.readAll();
    int pos = result.indexOf("Plural-Forms: ");
    if (KDE_ISUNLIKELY( pos==-1 ))
    {
        kWarning()<<"msginit error"<<result;
        return def;
    }
    pos+=14;

    int end = result.indexOf('"',pos);
    if (KDE_ISUNLIKELY( pos==-1 ))
    {
        kWarning()<<"msginit error"<<result;
        return def;
    }

    return QString( result.mid(pos,end-pos-2) );
}

void GettextStorage::updateHeader(bool forSaving)
{
    QStringList headerList(m_header.msgstrAsList());
    QStringList commentList(m_header.comment().split('\n',QString::SkipEmptyParts));

    QStringList::Iterator it,ait;
    QString temp;


    bool found=false;

    temp="Last-Translator: "+Settings::authorName();
    if (!Settings::authorEmail().isEmpty())
        temp+=(" <"+Settings::authorEmail()+'>');
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

    if (KDE_ISUNLIKELY( !found ))
        headerList.append(temp);

    QString dateTimeString = KDateTime::currentLocalDateTime().toString("%Y-%m-%d %H:%M%z");
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
    if (KDE_ISUNLIKELY( !found ))
        headerList.append(temp);

    found=false;

    temp="Project-Id-Version: "+m_url.fileName()+"\\n";
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
    if (KDE_ISUNLIKELY( !found ))
        headerList.append(temp);

    found=false;
    m_langCode=Project::instance()->isLoaded()?
               Project::instance()->langCode():
               m_langCode=Settings::defaultLangCode();

    KConfig lll("all_languages", KConfig::NoGlobals, "locale");
    lll.setLocale("");
    for ( it = headerList.begin(); it != headerList.end(); ++it )
    {
        if (it->contains(QRegExp("^ *Language-Team:.*")))
        {
            found=true;
            //really parse header
            QMap<QString,QString> map;
            QStringList langlist = KGlobal::locale()->languageList();
            QStringList::const_iterator myit;
            for (myit=langlist.begin();myit!=langlist.end();++myit)
            {
                KConfigGroup cg(&lll, *myit);
                map[cg.readEntry("Name")]=*myit;
            }

            QRegExp re("^ *Language-Team: *(.*) *<");
            QString val;
            if ((re.indexIn(*it) != -1) && (map.contains( val=re.cap(1).trimmed() )))
                m_langCode=map.value(val);

            ait=it;
        }
    }
    //m_language=locale.languageCodeToName(d->_langCode);
    KConfigGroup cg(&lll, m_langCode);
    m_language=cg.readEntry("Name");
    if (m_language.isEmpty())
        m_language=m_langCode;

    temp="Language-Team: "+m_language;
    if (Project::instance()->isLoaded())
        temp+=" <"+Project::instance()->mailingList()+'>';
    else
        temp+=" <"+Settings::defaultMailingList()+'>';
/*    if (!identityOptions->readEntry("DefaultMailingList").isEmpty())
        temp+=(" <"+identityOptions->readEntry("DefaultMailingList")+'>');*/
    temp+="\\n";

    if (KDE_ISLIKELY( found ))
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

    temp="X-Generator: LoKalize %1\\n";
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
    if (KDE_ISUNLIKELY( !found ))
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
    if (KDE_ISUNLIKELY( !found ))
    {
        headerList.append(temp);
    }


    found=false;

    kDebug()<<"testing for GNUPluralForms";
    // update plural form header
    for ( it = headerList.begin(); it != headerList.end(); ++it )
    {
        if (it->contains(QRegExp("^ *Plural-Forms:")))
        {
            found=true;
            break;
        }
    }
    if (found)
    {
        kDebug()<<"GNUPluralForms found";
        int num=numberOfPluralFormsFromHeader(m_header.msgstr());
        if (!num)
        {
            if (m_generatedFromDocbook)
                num=1;
            else
            {
                kWarning()<<"No plural form info in header, using project-defined one"<<m_langCode;
                QString t=GNUPluralForms(m_langCode);
                kWarning()<<"generated: " << t;
                if ( !t.isEmpty() )
                {
                    QRegExp pf("^ *Plural-Forms:\\s*nplurals.*\\\\n");
                    pf.setMinimal(true);
                    temp=QString("Plural-Forms: %1\\n").arg(t);
                    it->replace(pf,temp);
                    num=numberOfPluralFormsFromHeader(temp);
                }
                else
                {
                    kWarning()<<"no... smth went wrong :(\ncheck your gettext install";
                    num=2;
                }
            }
        }
        //setNumberOfPluralForms(num);
        m_numberOfPluralForms=num;

    }
    else if ( !m_generatedFromDocbook)
    {
        kDebug()<<"generating GNUPluralForms"<<m_langCode;
        QString t= GNUPluralForms(m_langCode);
        kDebug()<<"here it is:";
        if ( !t.isEmpty() )
            headerList.append(QString("Plural-Forms: %1\\n").arg(t));
    }

    m_header.setMsgstr( headerList.join( "\n" ) );

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
        //kDebug() << "REGEXPSTR: " <<  regexpstr;
        QRegExp regexp ( regexpstr );

        // The buggy variants exist in English too (of a time before KBabel got a translation for the corresponding language)
        QRegExp regexpUntranslated ( "^#\\s+translation of .* to .*\\s*$" );


        kDebug () << "Temp is '" << temp << "'";

        found=false;
        bool foundTemplate=false;

        it = commentList.begin();
        while ( it != commentList.end() )
        {
            kDebug () << "testing '" << (*it) << "'";
            bool deleteItem = false;

            if ( (*it) == temp )
            {
                kDebug () << "Match ";
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
    // kDebug() << "HEADER COMMENT: " << commentList;

    /*    if ( (!usePrefs || saveOptions.updateTranslatorCopyright)
            && ( ! identityOptions->readEntry("authorName","").isEmpty() )
            && ( ! identityOptions->readEntry("Email","").isEmpty() ) ) // An email address can be used as ersatz of a name
        {*/
//                        return;
    QStringList foundAuthors;

    temp = "# ";
    //temp += identityOptions->readEntry("authorName","");
    temp += Settings::authorName();
    if (!Settings::authorEmail().isEmpty())
        temp+=(" <"+Settings::authorEmail()+'>');
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
                // We have found a year number that is preceded by a comma.
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
                                      QRegExp::escape( Settings::authorName() )+".*"
                                      + QRegExp::escape( Settings::authorEmail() ) ) ) != -1 )
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
                        ait->insert(index+1, QString(", ")+cy);
                }
                else
                    kDebug() << "INTERNAL ERROR: author found but iterator dangling!";
            }

        }
        else
            foundAuthors.append(temp);


        for (ait=foundAuthors.begin();ait!=foundAuthors.end();++ait)
        {
            QString s = (*ait);

            // ensure dot at the end of copyright
            if ( !s.endsWith('.') ) s += '.';
            commentList.append(s);
        }
    }
    m_header.setComment( commentList.join( "\n" ) );

}

