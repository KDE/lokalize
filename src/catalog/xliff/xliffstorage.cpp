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

#include "xliffstorage.h"

#include "gettextheader.h"
#include "project.h"
#include "version.h"
#include "prefs_lokalize.h"

#include <QProcess>
#include <QString>
#include <QMap>
#include <QDomDocument>
#include <QTime>
#include <QPair>
#include <QList>


#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kdatetime.h>

#include <kio/netaccess.h>
#include <ktemporaryfile.h>


XliffStorage::XliffStorage()
 : CatalogStorage()
//  , m_doc(0)
{
    tmp="dd";
}


XliffStorage::~XliffStorage()
{
//     delete m_doc;
}

//BEGIN OPEN/SAVE

bool XliffStorage::load(const KUrl& url)
{
    QTime chrono;chrono.start();

    QString target;

    if(KDE_ISUNLIKELY( !KIO::NetAccess::download(url,target,NULL) ))
        return false;

//     QFileInfo info( filename );
//     if ( !info.exists( ) || info.isDir( ) )
//         return NULL; // NO_FILE;
//     if ( !info.isReadable( ) )
//         return NULL; // NO_PERMISSIONS;

    QFile file(target);
    if ( !file.open( QIODevice::ReadOnly ) )
        return false;

    //delete m_doc;m_doc=new QDomDocument();

    QString errorMsg;
    //int errorLine, errorColumn;
    bool success=m_doc.setContent( &file, false, &errorMsg/*, errorLine, errorColumn*/);
    file.close();
    KIO::NetAccess::removeTempFile( target );

    if (!success)
    {
//         delete m_doc;m_doc=0;
        return false;
    }

//     m_resView->setModel(new XliffResourceModel(m_doc));
//     m_resView->setItemsExpandedRecursive(m_resView->rootIndex());
//     connect(m_resView->selectionModel(), SIGNAL(currentRowChanged(const QModelIndex &,const QModelIndex &)),
//             this, SLOT(selectionChanged(const QModelIndex &,const QModelIndex &)));


    m_langCode=m_doc.elementsByTagName("file").at(0).attributes().namedItem("target-language").toCharacterData().data();
    m_numberOfPluralForms=numberOfPluralFormsForLangCode(m_langCode);

    //Create entry mapping.
    //Along the way: for langs with more then 2 forms
    //we create any form-entries additionally needed

    entries=m_doc.elementsByTagName("trans-unit");
    int i=0;
    int size=entries.size();
    m_map.clear();
    m_map.reserve(size);
    for(;i<size;++i)
    {
        QDomElement parentElement=entries.at(i).parentNode().toElement();
//         if (KDE_ISUNLIKELY( e.isNull() ))//sanity
//             continue;
        m_map<<i;

        if (parentElement.tagName()=="group" && parentElement.attribute("restype")=="x-gettext-plurals")
        {
            m_plurals<<i;
            int localPluralNum=m_numberOfPluralForms;
            while (--localPluralNum>0 && (++i)<size)
            {
                QDomElement p=entries.at(i).parentNode().toElement();
                if (p.tagName()=="group" && p.attribute("restype")=="x-gettext-plurals")
                    continue;

                parentElement.appendChild(entries.at(m_plurals.last()).cloneNode());
            }
        }
    }

//    entries=m_doc.elementsByTagName("body");
//     uint i=0;
//     uint lim=size();
//     while (i<lim)
//     {
//         CatalogItem& item=m_entries[i];
//         if (item.isPlural()
//             && item.msgstrPlural().count()<m_numberOfPluralForms
//            )
//         {
//             QVector<QString> msgstr(item.msgstrPlural());
//             while (msgstr.count()<m_numberOfPluralForms)
//                 msgstr.append(QString());
//             item.setMsgstr(msgstr);
// 
//         }
//         ++i;
// 
//     }


    kWarning()<<chrono.elapsed();
    return true;

}

bool XliffStorage::save(const KUrl& url)
{
    bool remote=false;

    //updateHeader(true);


    QString localFile;
    if (KDE_ISLIKELY( url.isLocalFile() ))
    {
        localFile = url.path();
        if (!QFile::exists(url.directory()))
            KIO::NetAccess::mkdir(url.upUrl(),0);//TODO recursion?
    }
    else
    {
        KTemporaryFile tmpFile;
        remote=true;
        tmpFile.open();
        localFile=tmpFile.fileName();
        tmpFile.close();
    }


    QFile file(localFile);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    file.write(m_doc.toByteArray(/*indent*/2));

    if (KDE_ISUNLIKELY( remote && !KIO::NetAccess::upload( localFile, url, NULL) ))
        return false;

    return true;

}
//END OPEN/SAVE

//BEGIN STORAGE TRANSLATION

int XliffStorage::size() const
{
    return m_map.size();
}

void XliffStorage::clear()
{
}

bool XliffStorage::isEmpty() const
{
    return m_doc.isNull();
//     return m_doc==0;
}



static const char* inlineElementNames[(int)InlineElementCount]={
    "_unknown",
    "bpt",
    "ept",
    "ph",
    "it",
//    "_NEVERSHOULDBECHOSEN",
    "mrk",
    "g",
    "sub",
    "_NEVERSHOULDBECHOSEN",
    "x",
    "bx",
    "ex",
};

/**
 * helper structure used during XLIFF XML walk-through
 */
struct ContentEditingData
{
    QList<TagRange>* ranges;
    QString stringToInsert;
    int pos;
    int lengthOfStringToRemove;

    //get
    ContentEditingData(QList<TagRange>* r)
    : ranges(r)
    , pos(-1)
    , lengthOfStringToRemove(-1)
    {}

    //delete
    ContentEditingData(int p,
                      int l)
    : ranges(0)
    , pos(p)
    , lengthOfStringToRemove(l)
    {}

    //insert
    ContentEditingData(int p,const QString& s)
    : ranges(0)
    , stringToInsert(s)
    , pos(p)
    , lengthOfStringToRemove(-1)
    {}

};

/**
 * walks through XLIFF XML and performs actions depending on ContentEditingData:
 * - reads content
 * - deletes content, or
 * - inserts content
 */
static QString content(QDomElement elem, int startingPos, ContentEditingData* data)
{
    if (elem.isNull())
        return QString();
    QString result;

    bool seenCharacterDataAfterElement=false;

    QDomNode n = elem.firstChild();
    while (!n.isNull())
    {
        if (n.isCharacterData())
        {
            seenCharacterDataAfterElement=true;

            QDomCharacterData c=n.toCharacterData();
            QString cData=c.data();

            if (data && data->pos!=-1 &&
               data->pos>=startingPos && data->pos<=startingPos+cData.size())
            {
                //DELETE
                if (data->lengthOfStringToRemove!=-1)
                {
                    if (data->pos-startingPos+data->lengthOfStringToRemove>cData.size())//sanity
                        kWarning()<<"SHIT HAPPENED. need to return number of actually deleted chars (modify ContentEditingData)";
                    c.deleteData(data->pos-startingPos,data->lengthOfStringToRemove);
                }
                else //INSERT
                    c.insertData(data->pos-startingPos,data->stringToInsert);
                cData=c.data();
            }
            else
                if (data&&data->pos!=-1/*&& n.nextSibling().isNull()*/)
                    kWarning()<<"arg!"<<startingPos<<"data->pos"<<data->pos;

            result += cData;
            startingPos+=cData.size();
        }
        else if (n.isElement())
        {
            QDomElement el=n.toElement();

            if (!seenCharacterDataAfterElement)
            {
                //add empty charData child so that user could add some text
                elem.insertBefore( elem.ownerDocument().createTextNode(""),n);
            }
            seenCharacterDataAfterElement=false;

            result += QChar(TAGRANGE_IMAGE_SYMBOL); ++startingPos;
            int oldStartingPos=startingPos;

            //detect type of the tag
            QByteArray tag=el.tagName().toUtf8();
            int i=InlineElementCount;
            while(--i>0)
                if (inlineElementNames[i]==tag)
                    break;

            //1 or 2 images to represent it?
            //2 = there may be content inside
            if (i<(int)_pairedXmlTagDelimiter)
            {
                QString recursiveContent=content(el,
                                                startingPos,
                                                data);
                if (!recursiveContent.isEmpty())
                    result += recursiveContent; startingPos+=recursiveContent.size();
                result += QChar(TAGRANGE_IMAGE_SYMBOL); ++startingPos;
            }

            if (data&&data->ranges)
            {
                QString id=el.attributeNode("id").value();
                if ((InlineElement)i==mrk)
                    id=el.attributeNode("mtype").value();

                kWarning()<<"id"<<id<<"tagName"<<el.tagName();
                *(data->ranges) <<TagRange(oldStartingPos-1,
                                startingPos-1,
                                (InlineElement)i,
                                id
                                );
            }

                    //
                    //QDomCharacterData cData = nodeContent.toCharacterData();
//                        cursor.insertText(cData.data(),phFormat());
//                        cursor.insertText(QString::number(++m_inlineCount),identifierFormat());
        }
            n = n.nextSibling();
    }
    if (!seenCharacterDataAfterElement)
    {
        //add empty charData child so that user could add some text
        elem.appendChild( elem.ownerDocument().createTextNode(""));
    }

    return result;
}



//flat-model interface (ignores XLIFF grouping)


CatalogString XliffStorage::sourceWithTags(const DocPosition& pos) const
{
    CatalogString catalogString;

    ContentEditingData data(&catalogString.ranges);
    catalogString.string=content(entries.at(m_map.at(pos.entry)).firstChildElement("source"),0,&data);
    return catalogString;
}

//reorders targetRanges to correspond sourceRanges
//also inserts placholders for ranges that are present only in sourceRanges, but not in targetRanges
CatalogString XliffStorage::targetWithTags(const DocPosition& pos) const
{
    CatalogString catalogString;

    QList<TagRange> sourceRanges;
    QList<TagRange> targetRanges;

    ContentEditingData data(&sourceRanges);
    content(entries.at(m_map.at(pos.entry)).firstChildElement("source"),0,&data);

    data=ContentEditingData(&targetRanges);
    catalogString.string=content(entries.at(m_map.at(pos.entry)).firstChildElement("target"),0,&data);

    int i;

    QHash<QString,int> sourceIds;
    for (i=0;i<sourceRanges.size();++i)
        sourceIds.insert(sourceRanges.at(i).id,i);

    QHash<QString,int> targetIds;
    for (i=0;i<targetRanges.size();++i)
        targetIds.insert(targetRanges.at(i).id,i);

    //first, insert tags that have no corresponding tag in source
    //(this should be really rare case)
    for (i=0;i<targetRanges.size();++i)
        if (!sourceIds.contains(targetRanges.at(i).id))
            catalogString.ranges.append(targetRanges.at(i));

    for (i=0;i<sourceRanges.size();++i)
    {
        if (targetIds.contains(sourceRanges.at(i).id))
            catalogString.ranges.append(targetRanges.at(      targetIds.value(sourceRanges.at(i).id)      ));//copy from targetRanges, because it contains position info!
        else
            catalogString.ranges.append(sourceRanges.at(i).getPlaceholder()); //to have parallel numbering in view
    }

    return catalogString;
}


QString XliffStorage::source(const DocPosition& pos) const
{
    return content(entries.at(m_map.at(pos.entry)).firstChildElement("source"), 0, NULL);
}
QString XliffStorage::target(const DocPosition& pos) const
{
    return content(entries.at(m_map.at(pos.entry)).firstChildElement("target"), 0, NULL);
}


void XliffStorage::targetDelete(const DocPosition& pos, int count)
{
    ContentEditingData data(pos.offset,count);
    content(entries.at(m_map.at(pos.entry)).firstChildElement("target"),0,&data);
}
void XliffStorage::targetInsert(const DocPosition& pos, const QString& arg)
{
    QDomNode unit=entries.at(m_map.at(pos.entry));
    QDomElement targetEl=unit.firstChildElement("target");
    if (targetEl.isNull())
    {
        targetEl=unit.insertAfter( m_doc.createElement("target")
                ,unit.firstChildElement("source")).toElement();
        targetEl.appendChild(m_doc.createTextNode(arg));//i bet that pos.offset is 0 ;)
        return;
    }
    ContentEditingData data(pos.offset,arg);
    content(targetEl,0,&data);
}

void XliffStorage::setTarget(const DocPosition& pos, const QString& arg)
{
}

QStringList XliffStorage::sourceAllForms(const DocPosition& pos) const
{
    return QStringList(tmp);
}
QStringList XliffStorage::targetAllForms(const DocPosition& pos) const
{
    return QStringList(tmp);
}

//DocPosition.form - number of <note>
QString XliffStorage::note(const DocPosition& pos) const
{
    return tmp;
}
int XliffStorage::noteCount(const DocPosition& pos) const
{
    return 0;
}

//DocPosition.form - number of <context>
QString XliffStorage::context(const DocPosition& pos) const
{
    return tmp;
}
int XliffStorage::contextCount(const DocPosition& pos) const
{
    return 0;
}

QStringList XliffStorage::matchData(const DocPosition& pos) const
{
    return QStringList(tmp);
}

QString XliffStorage::id(const DocPosition& pos) const
{
    return tmp;
}

bool XliffStorage::isPlural(const DocPosition& pos) const
{
    return false;
}

bool XliffStorage::isApproved(const DocPosition& pos) const
{
    return true;
}
void XliffStorage::setApproved(const DocPosition& pos, bool approved)
{
}

bool XliffStorage::isUntranslated(const DocPosition& pos) const
{
    return false;
}

//END STORAGE TRANSLATION


