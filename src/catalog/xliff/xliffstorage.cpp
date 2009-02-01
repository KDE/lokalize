/*
Copyright 2008-2009 Nick Shaforostoff <shaforostoff@kde.ru>

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


XliffStorage::XliffStorage()
 : CatalogStorage()
{
    tmp="dd";
}

XliffStorage::~XliffStorage()
{
}

int XliffStorage::capabilities() const
{
    return KeepsNoteAuthors|MultipleNotes;
}

//BEGIN OPEN/SAVE

int XliffStorage::load(QIODevice* device)
{
    QTime chrono;chrono.start();


    QString errorMsg;
    int errorLine;//+errorColumn;
    bool success=m_doc.setContent( device, false, &errorMsg, &errorLine/*,errorColumn*/);

    if (!success)
    {
        kWarning()<<errorMsg;
        return errorLine+1;
    }


    m_langCode=m_doc.elementsByTagName("file").at(0).attributes().namedItem("target-language").toCharacterData().data();
    m_numberOfPluralForms=numberOfPluralFormsForLangCode(m_langCode);

    //Create entry mapping.
    //Along the way: for langs with more than 2 forms
    //we create any form-entries additionally needed

    entries=m_doc.elementsByTagName("trans-unit");
    int i=0;
    int size=entries.size();
    m_map.clear();
    m_map.reserve(size);
    for(;i<size;++i)
    {
        QDomElement parentElement=entries.at(i).parentNode().toElement();
        //if (KDE_ISUNLIKELY( e.isNull() ))//sanity
        //      continue;
        m_map<<i;

        if (parentElement.tagName()=="group" && parentElement.attribute("restype")=="x-gettext-plurals")
        {
            m_plurals[i]=true;
            int localPluralNum=m_numberOfPluralForms;
            while (--localPluralNum>0 && (++i)<size)
            {
                QDomElement p=entries.at(i).parentNode().toElement();
                if (p.tagName()=="group" && p.attribute("restype")=="x-gettext-plurals")
                    continue;

                parentElement.appendChild(entries.at(m_map.last()).cloneNode());
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
    return 0;

}

bool XliffStorage::save(QIODevice* device)
{
    QTextStream stream(device);
    m_doc.save(stream,2);
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
}




/**
 * helper structure used during XLIFF XML walk-through
 */
struct ContentEditingData
{
    enum ActionType{Get,DeleteText,InsertText,DeleteTag,InsertTag,CheckLength};

    QList<TagRange> ranges;
    QString stringToInsert;
    int pos;
    int lengthOfStringToRemove;
    ActionType actionType;

    ///Get
    ContentEditingData(ActionType type=Get)
    : pos(-1)
    , lengthOfStringToRemove(-1)
    , actionType(type)
    {}

    ///DeleteText
    ContentEditingData(int p, int l)
    : pos(p)
    , lengthOfStringToRemove(l)
    , actionType(DeleteText)
    {}

    ///InsertText
    ContentEditingData(int p,const QString& s)
    : stringToInsert(s)
    , pos(p)
    , lengthOfStringToRemove(-1)
    , actionType(InsertText)
    {}

    ///InsertTag
    ContentEditingData(int p,const TagRange& range)
    : pos(p)
    , lengthOfStringToRemove(-1)
    , actionType(InsertTag)
    {
        ranges.append(range);
    }

    ///DeleteTag
    ContentEditingData(int p)
    : pos(p)
    , lengthOfStringToRemove(-1)
    , actionType(DeleteTag)
    {}

};

static QString doContent(QDomElement elem, int startingPos, ContentEditingData* data);

/**
 * walks through XLIFF XML and performs actions depending on ContentEditingData:
 * - reads content
 * - deletes content, or
 * - inserts content
 */
static QString content(QDomElement elem, ContentEditingData* data=0)
{
    return doContent(elem, 0, data);
}

static QString doContent(QDomElement elem, int startingPos, ContentEditingData* data)
{
    //actually startingPos is current pos

    QString result;

    if (elem.isNull()
    || (!result.isEmpty() && ContentEditingData::CheckLength))
    return QString();

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
                // time to do some action! ;)
                int localStartPos=data->pos-startingPos;

                //BEGIN DELETE TEXT
                if (data->actionType==ContentEditingData::DeleteText) //(data->lengthOfStringToRemove!=-1)
                {
                    if (localStartPos+data->lengthOfStringToRemove>cData.size())
                    {
                        //text is fragmented into several QDomCharacterData
                        int localDelLen=cData.size()-localStartPos;
                        //qWarning()<<"text is fragmented into several QDomCharacterData. localDelLen:"<<localDelLen<<"cData:"<<cData;
                        c.deleteData(localStartPos,localDelLen);
                        //setup data for future iterations
                        data->lengthOfStringToRemove=data->lengthOfStringToRemove-localDelLen;
                        //data->pos=startingPos;
                        //qWarning()<<"\tsetup:"<<data->pos<<data->lengthOfStringToRemove;
                    }
                    else
                    {
                        //qWarning()<<"simple delete"<<localStartPos<<data->lengthOfStringToRemove;
                        c.deleteData(localStartPos,data->lengthOfStringToRemove);
                        data->actionType=ContentEditingData::CheckLength;
                        return QString('a');//so it exits 100%
                    }
                }
                //END DELETE TEXT
                //INSERT
                else if (data->actionType==ContentEditingData::InsertText)
                {
                    c.insertData(localStartPos,data->stringToInsert);
                    data->actionType=ContentEditingData::CheckLength;
                    return QString('a');//so it exits 100%
                }
                //BEGIN INSERT TAG
                else if (data->actionType==ContentEditingData::InsertTag)
                {
                    const TagRange& tag=data->ranges.first();
                    QString mid=cData.mid(localStartPos);
                    qWarning()<<"inserting tag"<<tag.name()<<tag.id<<tag.start<<tag.end<<mid<<data->pos<<startingPos;
                    if (mid.size())
                        c.deleteData(localStartPos,mid.size());
                    QDomNode newNode=elem.insertAfter( elem.ownerDocument().createElement(tag.getElementName()),n);
                    QDomAttr attr=elem.ownerDocument().createAttribute("id");
                    attr.setValue(tag.id);
                    newNode.attributes().setNamedItem(attr);//setAttributeNode

                    if (tag.isPaired()&&tag.end>(tag.start+1))
                    {
                        //qWarning()<<"isPaired";
                        int len=tag.end-tag.start-1;//-image symbol
                        int localLen=qMin(len,mid.size());
                        if (localLen)//appending text
                        {
                            //qWarning()<<"localLen. appending"<<localLen<<mid.left(localLen);
                            newNode.appendChild( elem.ownerDocument().createTextNode(mid.left(localLen)) );
                            mid=mid.mid(localLen);
                        }
                        if (len-localLen) //need to eat more (strings or elements) into newNode
                        {
                            int missingLen=len-localLen;
                            //qWarning()<<"len-localLen";
                            //iterate over siblings until we get childrenCumulativeLen>missingLen (or siblings end)
                            int childrenCumulativeLen=0;
                            QDomNode sibling=newNode.nextSibling();
                            while(!sibling.isNull())//&&(childrenCumulativeLen<missingLen))
                            {
                                QDomNode tmp=sibling;
                                sibling=sibling.nextSibling();
                                if (tmp.isAttr())
                                    continue;
                                ContentEditingData subData(ContentEditingData::Get);
                                if (tmp.isElement())
                                {
                                    childrenCumulativeLen++;
                                    childrenCumulativeLen+=TagRange::isPaired(TagRange::getElementType(tmp.toElement().tagName().toUtf8()));
                                    kWarning()<<"calling sub";
                                    QString subContent=doContent(tmp.toElement(),/*we don't care about position*/0,&subData);
                                    kWarning()<<"called sub";
                                    childrenCumulativeLen+=subContent.size();
                                }
                                else if (tmp.isCharacterData())
                                    childrenCumulativeLen+=tmp.toCharacterData().data().size();
                                //qWarning()<<"brbr"<<tmp.nodeName()<<tmp.nodeValue()
                                //<<childrenCumulativeLen<<missingLen;

                                if (childrenCumulativeLen>missingLen)
                                {
                                    if (tmp.isCharacterData())
                                    {
                                        //divide the last string
                                        const QString& endData=tmp.toCharacterData().data();
                                        QString last=endData.left(endData.size()-(childrenCumulativeLen-missingLen));
                                        newNode.appendChild( elem.ownerDocument().createTextNode(last));
                                        tmp.toCharacterData().deleteData(0,last.size());
                                        //qWarning()<<"end of add"<<last;
                                    }
                                    break;
                                }
                                newNode.appendChild( tmp );
                            }

                        }
                        if (!newNode.lastChild().isCharacterData())
                            newNode.appendChild( elem.ownerDocument().createTextNode(""));
                    }
                    if (!mid.isEmpty())
                        elem.insertAfter( elem.ownerDocument().createTextNode(mid),newNode);
                    else if (!newNode.nextSibling().isCharacterData()) //keep our DOM in a nice state
                        elem.insertAfter( elem.ownerDocument().createTextNode(""),newNode);

                    data->actionType=ContentEditingData::CheckLength;
                    return QString('a');//we're done here
                }
                //END INSERT TAG
                cData=c.data();
            }
            //else
            //    if (data&&data->pos!=-1/*&& n.nextSibling().isNull()*/)
            //        kWarning()<<"arg!"<<startingPos<<"data->pos"<<data->pos;

            result += cData;
            startingPos+=cData.size();
        }
        else if (n.isElement())
        {
            QDomElement el=n.toElement();
            //BEGIN DELETE TAG
            if (data&&data->actionType==ContentEditingData::DeleteTag
                &&data->pos==startingPos)
            {
                //qWarning()<<"start deleting tag";
                data->ranges.append(TagRange(startingPos, -1, TagRange::getElementType(el.tagName().toUtf8()), el.attribute("id")));
                if (data->ranges.first().isPaired())
                {
                    //get end position
                    ContentEditingData subData(ContentEditingData::Get);
                    QString subContent=doContent(el,startingPos,&subData);
                    data->ranges[0].end=1+startingPos+subContent.size();//tagsymbol+text
                    //qWarning()<<"get end position"<<startingPos<<subContent.size();

                    //move children upper
                    QDomNode local = n.firstChild();
                    QDomNode refNode=n;
                    while (!local.isNull())
                    {
                        QDomNode tmp=local;
                        local = local.nextSibling();
                        if (!tmp.isAttr())
                        {
                            //qWarning()<<"here is another child"<<tmp.nodeType()<<tmp.nodeName()<<tmp.nodeValue();
                            refNode=elem.insertAfter(tmp,refNode);
                        }
                    }

                }
                QDomNode temp=n;
                n=n.nextSibling();
                elem.removeChild(temp);
                data->actionType=ContentEditingData::CheckLength;
                return QString('a');//we're done here
            }
            //END DELETE TAG

            if (!seenCharacterDataAfterElement)  //add empty charData child so that user could add some text
                elem.insertBefore( elem.ownerDocument().createTextNode(""),n);
            seenCharacterDataAfterElement=false;

            result += QChar(TAGRANGE_IMAGE_SYMBOL); ++startingPos;
            int oldStartingPos=startingPos;

            //detect type of the tag
            TagRange::InlineElement i=TagRange::getElementType(el.tagName().toUtf8());

            //1 or 2 images to represent it?
            //2 = there may be content inside
            if (TagRange::isPaired(i))
            {
                QString recursiveContent=doContent(el,startingPos,data);
                if (!recursiveContent.isEmpty())
                    result += recursiveContent; startingPos+=recursiveContent.size();
                result += QChar(TAGRANGE_IMAGE_SYMBOL); ++startingPos;
            }

            if (data&&data->actionType==ContentEditingData::Get)
            {
                QString id=el.attributeNode("id").value();
                if (i==TagRange::mrk)//TODO attr map
                    id=el.attributeNode("mtype").value();

                //kWarning()<<"tagName"<<el.tagName()<<"id"<<id<<"start"<<oldStartingPos-1<<startingPos-1;
                data->ranges.append(TagRange(oldStartingPos-1,startingPos-1,i,id));
            }
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

CatalogString XliffStorage::catalogString(const DocPosition& pos) const
{
    static const char* names[]={"source","target"};
    CatalogString catalogString;
    ContentEditingData data(ContentEditingData::Get);
    catalogString.string=content(entries.at(m_map.at(pos.entry)).firstChildElement(
                                 names[pos.part==DocPosition::Target]),&data);
    catalogString.ranges=data.ranges;
    return catalogString;
}

CatalogString XliffStorage::targetWithTags(DocPosition pos) const
{
    pos.part=DocPosition::Target;return catalogString(pos);
}
CatalogString XliffStorage::sourceWithTags(DocPosition pos) const
{
    pos.part=DocPosition::Source;return catalogString(pos);
}

QString XliffStorage::source(const DocPosition& pos) const
{
    return content(entries.at(m_map.at(pos.entry)).firstChildElement("source"));
}
QString XliffStorage::target(const DocPosition& pos) const
{
    return content(entries.at(m_map.at(pos.entry)).firstChildElement("target"));
}


void XliffStorage::targetDelete(const DocPosition& pos, int count)
{
    ContentEditingData data(pos.offset,count);
    content(entries.at(m_map.at(pos.entry)).firstChildElement("target"),&data);
}

void XliffStorage::targetInsert(const DocPosition& pos, const QString& arg)
{
    //BEGIN add <target>
    QDomNode unit=entries.at(m_map.at(pos.entry));
    QDomElement targetEl=unit.firstChildElement("target");
    if (targetEl.isNull())
    {
        QDomNode refNode=unit.firstChildElement("seg-source");//obey standard
        if (refNode.isNull()) refNode=unit.firstChildElement("source");
        targetEl = unit.insertAfter(m_doc.createElement("target"),refNode).toElement();
        targetEl.appendChild(m_doc.createTextNode(arg));//i bet that pos.offset is 0 ;)
        return;
    }
    //END add <target>
    if (arg.isEmpty()) return; //means we were called from targetInsertTag()
    ContentEditingData data(pos.offset,arg);
    content(targetEl,&data);
}

void XliffStorage::targetInsertTag(const DocPosition& pos, const TagRange& tag)
{
    targetInsert(pos,QString()); //adds <taget> if needed
    ContentEditingData data(tag.start,tag);
    content(entries.at(m_map.at(pos.entry)).firstChildElement("target"),&data);
}

TagRange XliffStorage::targetDeleteTag(const DocPosition& pos)
{
    ContentEditingData data(pos.offset);
    content(entries.at(m_map.at(pos.entry)).firstChildElement("target"),&data);
    if (data.ranges[0].end==-1) data.ranges[0].end=data.ranges[0].start;
    return data.ranges.first();
}

void XliffStorage::setTarget(const DocPosition& pos, const QString& arg)
{
}

QString XliffStorage::alttrans(const DocPosition& pos) const
{
    return QString();
}

static void initNoteFromElement(Note& note, QDomElement elem)
{
    note.content=elem.text();
    note.from=elem.attribute("from");
    note.lang=elem.attribute("xml:lang");
    if (elem.attribute("annotates")=="source")
        note.annotates=Note::Source;
    else if (elem.attribute("annotates")=="target")
        note.annotates=Note::Target;
    bool ok;
    note.priority=elem.attribute("priority").toInt(&ok);
    if (!ok) note.priority=0;
}

QList<Note> XliffStorage::notes(const DocPosition& pos) const
{
    QList<Note> result;

    QDomElement elem = entries.at(m_map.at(pos.entry)).firstChildElement("note");
    while (!elem.isNull())
    {
        Note note;
        initNoteFromElement(note,elem);
        result.append(note);
        elem=elem.nextSiblingElement("note");
    }
    qSort(result);
    return result;
}

Note XliffStorage::setNote(const DocPosition& pos, const Note& note)
{
    QDomElement unit=entries.at(m_map.at(pos.entry)).toElement();
    QDomElement elem;
    Note oldNote;
    if (pos.form==-1)
    {
        QDomElement ref=unit.lastChildElement("note");
        elem=unit.insertAfter( m_doc.createElement("note"),ref).toElement();
        elem.appendChild(m_doc.createTextNode(""));
    }
    else
    {
        elem = unit.elementsByTagName("note").at(pos.form).toElement();
        initNoteFromElement(oldNote,elem);
    }

    if (elem.isNull()) return oldNote;

    if (!elem.text().isEmpty())
    {
        ContentEditingData data(0,elem.text().size());
        content(elem,&data);
    }

    if (!note.content.isEmpty())
    {
        ContentEditingData data(0,note.content); content(elem,&data);
        if (!note.from.isEmpty()) elem.setAttribute("from",note.from);
        if (note.priority) elem.setAttribute("priority",note.priority);
    }
    else
        unit.removeChild(elem);

    return oldNote;
}

QStringList XliffStorage::noteAuthors() const
{
    QMap<QString,bool> result;
    QDomNodeList notes=m_doc.elementsByTagName("note");
    int i=notes.size();
    while (--i>=0)
    {
        QString from=notes.at(i).toElement().attribute("from");
        if (!from.isEmpty())
            result[from]=true;
    }
    return result.keys();
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
    return m_plurals.contains(pos.entry)&&m_plurals.value(pos.entry);
}

bool XliffStorage::isApproved(const DocPosition& pos) const
{
    return entries.at(m_map.at(pos.entry)).toElement().attribute("approved")=="yes";
}
void XliffStorage::setApproved(const DocPosition& pos, bool approved)
{
    static const char* noyes[]={"no","yes"};
    entries.at(m_map.at(pos.entry)).toElement().setAttribute("approved",noyes[approved]);
}

bool XliffStorage::isUntranslated(const DocPosition& pos) const
{
    ContentEditingData data(ContentEditingData::CheckLength);
    return content(entries.at(m_map.at(pos.entry)).firstChildElement("target"),&data).isEmpty();
}

//END STORAGE TRANSLATION


