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

#include "gettextheader.h"
#include "catalogitem_private.h"
#include "gettextimport.h"
#include "gettextexport.h"

#include "project.h"

#include "version.h"
#include "prefs_lokalize.h"

#include "diff.h"

#include <QProcess>
#include <QString>
#include <QMap>

#include <kdebug.h>

// static QString GNUPluralForms(const QString& lang);

using namespace GettextCatalog;

GettextStorage::GettextStorage()
 : CatalogStorage()
{
}


GettextStorage::~GettextStorage()
{
}

//BEGIN OPEN/SAVE

int GettextStorage::load(QIODevice* device/*, bool readonly*/)
{
    //GettextImportPlugin importer=GettextImportPlugin(readonly?(new ExtraDataSaver()):(new ExtraDataListSaver()));
    GettextImportPlugin importer;
    ConversionStatus status = OK;
    int errorLine;
    status = importer.open(device,this,&errorLine);

    //for langs with more than 2 forms
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
    //qCompress(m_storage->m_catalogExtraData.join("\n\n").toUtf8(),9);

    return status==OK?0:(errorLine+1);
}

bool GettextStorage::save(QIODevice* device, bool belongsToProject)
{
    QString header=m_header.msgstr();
    QString comment=m_header.comment();
    QString catalogProjectId;//=m_url.fileName();
    //catalogProjectId=catalogProjectId.left(catalogProjectId.lastIndexOf('.'));
    updateHeader(header,
                 comment,
                 m_targetLangCode,
                 m_numberOfPluralForms,
                 catalogProjectId,
                 m_generatedFromDocbook,
                 belongsToProject,
                 /*forSaving*/true);
    m_header.setMsgstr(header);
    m_header.setComment(comment);

    //GettextExportPlugin exporter(m_maxLineLength>70?m_maxLineLength:-1, m_trailingNewLines);// this is kinda hackish...
    GettextExportPlugin exporter(Project::instance()->wordWrap(), m_trailingNewLines);

    ConversionStatus status = OK;
    status = exporter.save(device/*x-gettext-translation*/,this);

    return status==OK;
}

//END OPEN/SAVE

//BEGIN STORAGE TRANSLATION

int GettextStorage::size() const
{
    return m_entries.size();
}


static const QChar altSep(156);
static InlineTag makeInlineTag(int i)
{
    static const QString altSepText(" | ");
    static const QString ctype=i18n("separator for different-length string alternatives");
    return InlineTag(i,i,InlineTag::x,QString::number(i),QString(),altSepText,ctype);
}

static CatalogString makeCatalogString(const QString& string)
{
    CatalogString result;
    result.string=string;

    int i=0;

    while((i=result.string.indexOf(altSep, i))!=-1)
    {
        result.string[i]=TAGRANGE_IMAGE_SYMBOL;
        result.tags.append(makeInlineTag(i));
        ++i;
    }
    return result;
}

//flat-model interface (ignores XLIFF grouping)
CatalogString GettextStorage::sourceWithTags(DocPosition pos) const
{
    return makeCatalogString(source(pos));
}
CatalogString GettextStorage::targetWithTags(DocPosition pos) const
{
    return makeCatalogString(target(pos));
}

QString GettextStorage::source(const DocPosition& pos) const
{
    return m_entries.at(pos.entry).msgid(pos.form);
}
QString GettextStorage::target(const DocPosition& pos) const
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


void GettextStorage::targetInsertTag(const DocPosition& pos, const InlineTag& tag)
{
    Q_UNUSED(tag);
    targetInsert(pos,altSep);
}

InlineTag GettextStorage::targetDeleteTag(const DocPosition& pos)
{
    targetDelete(pos,1);
    return makeInlineTag(pos.offset);
}

QStringList GettextStorage::sourceAllForms(const DocPosition& pos, bool stripNewLines) const
{
    return m_entries.at(pos.entry).allPluralForms(CatalogItem::Source, stripNewLines);
}

QStringList GettextStorage::targetAllForms(const DocPosition& pos, bool stripNewLines) const
{
    return m_entries.at(pos.entry).allPluralForms(CatalogItem::Target, stripNewLines);
}

QVector<AltTrans> GettextStorage::altTrans(const DocPosition& pos) const
{
    QStringList prev=m_entries.at(pos.entry).comment().split('\n').filter(QRegExp("^#\\|"));

    QString oldSingular;
    QString oldPlural;

    QString* cur=&oldSingular;
    QStringList::iterator it=prev.begin();
    static const QString msgid_plural_alt="#| msgid_plural \"";
    while (it!=prev.end())
    {
        if (it->startsWith(msgid_plural_alt))
            cur=&oldPlural;

        int start=it->indexOf('\"')+1;
        int end=it->lastIndexOf('\"');
        if (start&&end!=-1)
        {
            if (!cur->isEmpty())
                (*cur)+='\n';
            if (!(  cur->isEmpty() && (end-start)==0 ))//for multiline msgs
                (*cur)+=it->midRef(start,end-start);
        }
        ++it;
    }
    if (pos.form==0)
        cur=&oldSingular;

    cur->replace("\\\"","\"");

    QVector<AltTrans> result;
    if (!cur->isEmpty())
        result<<AltTrans(CatalogString(*cur));

    return result;
}

Note GettextStorage::setNote(DocPosition pos, const Note& note)
{
    //kWarning()<<"s"<<m_entries.at(pos.entry).comment();
    Note oldNote;
    QVector<Note> l=notes(pos);
    if (l.size()) oldNote=l.first();

    QStringList comment=m_entries.at(pos.entry).comment().split('\n');
    //remove previous comment;
    QStringList::iterator it=comment.begin();
    while (it!=comment.end())
    {
        if (it->startsWith("# "))
            it=comment.erase(it);
        else
            ++it;
    }
    if (note.content.size())
        comment.prepend("# "+note.content.split('\n').join("\n# "));
    m_entries[pos.entry].setComment(comment.join("\n"));

    //kWarning()<<"e"<<m_entries.at(pos.entry).comment();
    return oldNote;
}

QVector<Note> GettextStorage::notes(const DocPosition& docPosition, const QRegExp& re, int preLen) const
{
    QVector<Note> result;
    QString content;

    QStringList note=m_entries.at(docPosition.entry).comment().split('\n').filter(re);

    foreach(const QString &s, note)
    {
        if (s.size()>=preLen)
        {
            content+=s.midRef(preLen);
            content+='\n';
        }
    }

    if (!content.isEmpty())
    {
        content.chop(1);
        result<<Note(content);
    }
    return result;

//i18nc("@info PO comment parsing. contains filename","<i>Place:</i>");
//i18nc("@info PO comment parsing","<i>GUI place:</i>");
}

QVector<Note> GettextStorage::notes(const DocPosition& docPosition) const
{
    static const QRegExp nre("^# ");
    return notes(docPosition,nre,2);
}

QVector<Note> GettextStorage::developerNotes(const DocPosition& docPosition) const
{
    static const QRegExp dnre("^#\\. (?!i18n: file:)");
    return notes(docPosition,dnre,3);
}

QStringList GettextStorage::sourceFiles(const DocPosition& pos) const
{
    QStringList result;
    QStringList commentLines=m_entries.at(pos.entry).comment().split('\n');

    static const QRegExp i18n_file_re("^#. i18n: file: ");
    foreach(const QString &uiLine, commentLines.filter(i18n_file_re))
    {
        //QStringList uiFiles=uiLine.mid(15).split(' ');
        result+=QString::fromRawData(uiLine.unicode()+15, uiLine.length()-15).split(' ');
    }
    bool hasUi=!result.isEmpty();

    static const QRegExp cpp_re("^#: ");
    foreach(const QString &cppLine, commentLines.filter(cpp_re))
    {
        if (hasUi && cppLine.startsWith("#: rc.cpp")) continue;
        QStringList cppFiles=QString::fromRawData(cppLine.unicode()+3, cppLine.length()-3).split(' ');
        result+=cppFiles;
    }
    return result;
}

QStringList GettextStorage::context(const DocPosition& pos) const
{
    return matchData(pos);
}

QStringList GettextStorage::matchData(const DocPosition& pos) const
{
    QString ctxt=m_entries.at(pos.entry).msgctxt();

    //KDE-specific
    //Splits @info:whatsthis and actual note
/*    if (ctxt.startsWith('@') && ctxt.contains(' '))
    {
        QStringList result(ctxt.section(' ',0,0,QString::SectionSkipEmpty));
        result<<ctxt.section(' ',1,-1,QString::SectionSkipEmpty);
        return result;
    }*/
    return QStringList(ctxt);
}

QString GettextStorage::id(const DocPosition& pos) const
{
    //entries in gettext format may be non-unique
    //only if their msgctxts are different

    QString result=source(pos);
    result.remove('\n');
    result.prepend(m_entries.at(pos.entry).msgctxt()+":\n");
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
    return !m_entries.at(pos.entry).isFuzzy();
}
void GettextStorage::setApproved(const DocPosition& pos, bool approved)
{
    if (approved)
        m_entries[pos.entry].unsetFuzzy();
    else
        m_entries[pos.entry].setFuzzy();
}

bool GettextStorage::isEmpty(const DocPosition& pos) const
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
      values.replace ("\\n", "\\n\n");
//       kDebug () << "Normalized header: " << values;
      QString comment=newHeader.comment();
      QString catalogProjectId;//=m_url.fileName(); FIXME m_url is always empty
      //catalogProjectId=catalogProjectId.left(catalogProjectId.lastIndexOf('.'));
      bool belongsToProject=m_url.path().contains(Project::instance()->poDir());

      updateHeader(values,
                   comment,
                   m_targetLangCode,
                   m_numberOfPluralForms,
                   catalogProjectId,
                   m_generatedFromDocbook,
                   belongsToProject,
                  /*forSaving*/true);
      m_header=newHeader;
      m_header.setComment(comment);
      m_header.setMsgstr(values);

//       setClean(false);
      //emit signalHeaderChanged();

      return true;
   }
   kWarning () << "header Not valid";
   return false;
}



