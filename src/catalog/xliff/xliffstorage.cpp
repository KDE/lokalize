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
#include <QXmlSimpleReader>

static const char* const noyes[]={"no","yes"};
static const char* const bintargettarget[]={"bin-target","target"};
static const char* const binsourcesource[]={"bin-source","source"};

XliffStorage::XliffStorage()
 : CatalogStorage()
{
}

XliffStorage::~XliffStorage()
{
}

int XliffStorage::capabilities() const
{
    return KeepsNoteAuthors|MultipleNotes|Phases|ExtendedStates;
}

//BEGIN OPEN/SAVE

int XliffStorage::load(QIODevice* device)
{
    QTime chrono;chrono.start();


    QXmlSimpleReader reader;
    reader.setFeature("http://qtsoftware.com/xml/features/report-whitespace-only-CharData",true);
    reader.setFeature("http://xml.org/sax/features/namespaces",false);
    QXmlInputSource source(device);

    QString errorMsg;
    int errorLine;//+errorColumn;
    bool success=m_doc.setContent(&source, &reader, &errorMsg, &errorLine/*,errorColumn*/);

    if (!success)
    {
        kWarning()<<errorMsg;
        return errorLine+1;
    }


    QDomElement file=m_doc.elementsByTagName("file").at(0).toElement();
    m_sourceLangCode=file.attribute("source-language");
    m_targetLangCode=file.attribute("target-language");
    m_numberOfPluralForms=numberOfPluralFormsForLangCode(m_targetLangCode);

    //Create entry mapping.
    //Along the way: for langs with more than 2 forms
    //we create any form-entries additionally needed

    entries=m_doc.elementsByTagName("trans-unit");
    int size=entries.size();
    m_map.clear();
    m_map.reserve(size);
    for(int i=0;i<size;++i)
    {
        QDomElement parentElement=entries.at(i).parentNode().toElement();
        //if (KDE_ISUNLIKELY( e.isNull() ))//sanity
        //      continue;
        m_map<<i;
        m_unitsById[entries.at(i).toElement().attribute("id")]=i;

        if (parentElement.tagName()=="group" && parentElement.attribute("restype")=="x-gettext-plurals")
        {
            m_plurals.insert(i);
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

    binEntries=m_doc.elementsByTagName("bin-unit");
    size=binEntries.size();
    int offset=m_map.size();
    for(int i=0;i<size;++i)
        m_unitsById[binEntries.at(i).toElement().attribute("id")]=offset+i;

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


    QDomElement header=file.firstChildElement("header");
    if (header.isNull())
        header=file.insertBefore(m_doc.createElement("header"), QDomElement()).toElement();
    QDomElement toolElem=header.firstChildElement("tool");
    while (!toolElem.isNull() && toolElem.attribute("tool-id")!="lokalize-" LOKALIZE_VERSION)
        toolElem=toolElem.nextSiblingElement("tool");

    if (toolElem.isNull())
    {
        toolElem=header.appendChild(m_doc.createElement("tool")).toElement();
        toolElem.setAttribute("tool-id","lokalize-" LOKALIZE_VERSION);
        toolElem.setAttribute("tool-name","Lokalize");
        toolElem.setAttribute("tool-version",LOKALIZE_VERSION);
    }

    kWarning()<<chrono.elapsed();
    return 0;
}

bool XliffStorage::save(QIODevice* device, bool belongsToProject)
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




/**
 * helper structure used during XLIFF XML walk-through
 */
struct ContentEditingData
{
    enum ActionType{Get,DeleteText,InsertText,DeleteTag,InsertTag,CheckLength};

    QList<InlineTag> tags;
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
    ContentEditingData(int p,const InlineTag& range)
    : pos(p)
    , lengthOfStringToRemove(-1)
    , actionType(InsertTag)
    {
        tags.append(range);
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
                    const InlineTag& tag=data->tags.first();
                    QString mid=cData.mid(localStartPos);
                    qWarning()<<"inserting tag"<<tag.name()<<tag.id<<tag.start<<tag.end<<mid<<data->pos<<startingPos;
                    if (mid.size())
                        c.deleteData(localStartPos,mid.size());
                    QDomElement newNode=elem.insertAfter( elem.ownerDocument().createElement(tag.getElementName()),n).toElement();
                    newNode.setAttribute("id",tag.id);
                    if (!tag.xid.isEmpty())
                        newNode.setAttribute("xid",tag.xid);

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
                                    childrenCumulativeLen+=InlineTag::isPaired(InlineTag::getElementType(tmp.toElement().tagName().toUtf8()));
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
                data->tags.append(InlineTag(startingPos, -1, InlineTag::getElementType(el.tagName().toUtf8()), el.attribute("id"), el.attribute("xid")));
                if (data->tags.first().isPaired())
                {
                    //get end position
                    ContentEditingData subData(ContentEditingData::Get);
                    QString subContent=doContent(el,startingPos,&subData);
                    data->tags[0].end=1+startingPos+subContent.size();//tagsymbol+text
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

            if (data)
                {result += QChar(TAGRANGE_IMAGE_SYMBOL); ++startingPos;}
            int oldStartingPos=startingPos;

            //detect type of the tag
            InlineTag::InlineElement i=InlineTag::getElementType(el.tagName().toUtf8());

            //1 or 2 images to represent it?
            //2 = there may be content inside
            if (InlineTag::isPaired(i))
            {
                QString recursiveContent=doContent(el,startingPos,data);
                if (!recursiveContent.isEmpty())
                    result += recursiveContent; startingPos+=recursiveContent.size();
                if (data)
                    {result += QChar(TAGRANGE_IMAGE_SYMBOL); ++startingPos;}
            }

            if (data&&data->actionType==ContentEditingData::Get)
            {
                QString id=el.attribute("id");
                if (i==InlineTag::mrk)//TODO attr map
                    id=el.attribute("mtype");

                //kWarning()<<"tagName"<<el.tagName()<<"id"<<id<<"start"<<oldStartingPos-1<<startingPos-1;
                data->tags.append(InlineTag(oldStartingPos-1,startingPos-1,i,id,el.attribute("xid")));
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

CatalogString XliffStorage::catalogString(QDomElement unit,  DocPosition::Part part) const
{
    static const char* names[]={"source","target"};
    CatalogString catalogString;
    ContentEditingData data(ContentEditingData::Get);
    catalogString.string=content(unit.firstChildElement( names[part==DocPosition::Target]), &data );
    catalogString.tags=data.tags;
    return catalogString;
}

CatalogString XliffStorage::catalogString(const DocPosition& pos) const
{
    return catalogString(unitForPos(pos.entry), pos.part);
}

CatalogString XliffStorage::targetWithTags(DocPosition pos) const
{
    return catalogString(unitForPos(pos.entry), DocPosition::Target);
}
CatalogString XliffStorage::sourceWithTags(DocPosition pos) const
{
    return catalogString(unitForPos(pos.entry), DocPosition::Source);
}

static QString genericContent(QDomElement elem, bool nonbin)
{
    return nonbin?content(elem):elem.firstChildElement("external-file").attribute("href");
}
QString XliffStorage::source(const DocPosition& pos) const
{
    return genericContent(sourceForPos(pos.entry),pos.entry<size());
}
QString XliffStorage::target(const DocPosition& pos) const
{
    return genericContent(targetForPos(pos.entry),pos.entry<size());
}


void XliffStorage::targetDelete(const DocPosition& pos, int count)
{
    if (pos.entry<size())
    {
        ContentEditingData data(pos.offset,count);
        content(targetForPos(pos.entry),&data);
    }
    else
    {
        //only bulk delete requests are generated
        targetForPos(pos.entry).firstChildElement("external-file").setAttribute("href","");
    }
}

void XliffStorage::targetInsert(const DocPosition& pos, const QString& arg)
{
    kWarning()<<pos.entry<<arg;
    QDomElement targetEl=targetForPos(pos.entry);
    //BEGIN add <*target>
    if (targetEl.isNull())
    {
        QDomNode unitEl=unitForPos(pos.entry);
        QDomNode refNode=unitEl.firstChildElement("seg-source");//obey standard
        if (refNode.isNull()) refNode=unitEl.firstChildElement(binsourcesource[pos.entry<size()]);
        targetEl = unitEl.insertAfter(m_doc.createElement(bintargettarget[pos.entry<size()]),refNode).toElement();
        targetEl.setAttribute("state","new");

        if (pos.entry<size())
        {
            targetEl.appendChild(m_doc.createTextNode(arg));//i bet that pos.offset is 0 ;)
            return;
        }
    }
    //END add <*target>
    if (arg.isEmpty()) return; //means we were called just to add <taget> tag

    if (pos.entry>=size())
    {
        QDomElement ef=targetEl.firstChildElement("external-file");
        if (ef.isNull())
            ef=targetEl.appendChild(m_doc.createElement("external-file")).toElement();
        ef.setAttribute("href",arg);
        return;
    }

    ContentEditingData data(pos.offset,arg);
    content(targetEl,&data);
}

void XliffStorage::targetInsertTag(const DocPosition& pos, const InlineTag& tag)
{
    targetInsert(pos,QString()); //adds <taget> if needed
    ContentEditingData data(tag.start,tag);
    content(targetForPos(pos.entry),&data);
}

InlineTag XliffStorage::targetDeleteTag(const DocPosition& pos)
{
    ContentEditingData data(pos.offset);
    content(targetForPos(pos.entry),&data);
    if (data.tags[0].end==-1) data.tags[0].end=data.tags[0].start;
    return data.tags.first();
}

void XliffStorage::setTarget(const DocPosition& pos, const QString& arg)
{
    Q_UNUSED(pos);
    Q_UNUSED(arg);
//TODO
}


QVector<AltTrans> XliffStorage::altTrans(const DocPosition& pos) const
{
    QVector<AltTrans> result;

    QDomElement elem = unitForPos(pos.entry).firstChildElement("alt-trans");
    while (!elem.isNull())
    {
        AltTrans aTrans;
        aTrans.source=catalogString(elem, DocPosition::Source);
        aTrans.target=catalogString(elem, DocPosition::Target);
        aTrans.phase=elem.attribute("phase-name");
        aTrans.origin=elem.attribute("origin");
        aTrans.score=elem.attribute("match-quality").toInt();
        aTrans.lang=elem.firstChildElement("target").attribute("xml:lang");

        const char* const types[]={
            "proposal",
            "previous-version",
            "rejected",
            "reference",
            "accepted"
        };
        QString typeStr=elem.attribute("alttranstype");
        int i=-1;
        while (++i<int(sizeof(types)/sizeof(char*)) && types[i]!=typeStr)
            ;
        aTrans.type=AltTrans::Type(i);

        result<<aTrans;

        elem=elem.nextSiblingElement("alt-trans");
    }
    return result;
}

static QDomElement phaseElement(QDomDocument m_doc, const QString& name, QDomElement& phasegroup)
{
    QDomElement file=m_doc.elementsByTagName("file").at(0).toElement();
    QDomElement header=file.firstChildElement("header");
    phasegroup=header.firstChildElement("phase-group");
    if (phasegroup.isNull())
    {
        phasegroup=m_doc.createElement("phase-group");
        //order following XLIFF spec
        QDomElement skl=header.firstChildElement("skl");
        if (!skl.isNull())
            header.insertAfter(phasegroup, skl);
        else
            header.insertBefore(phasegroup, header.firstChildElement());
    }
    QDomElement phaseElem=phasegroup.firstChildElement("phase");
    while (!phaseElem.isNull() && phaseElem.attribute("phase-name")!=name)
        phaseElem=phaseElem.nextSiblingElement("phase");

    return phaseElem;
}

static Phase phaseFromElement(QDomElement phaseElem)
{
    Phase phase;
    phase.name      =phaseElem.attribute("phase-name");
    phase.process   =phaseElem.attribute("process-name");
    phase.company   =phaseElem.attribute("company-name");
    phase.contact   =phaseElem.attribute("contact-name");
    phase.email     =phaseElem.attribute("contact-email");
    phase.phone     =phaseElem.attribute("contact-phone");
    phase.tool      =phaseElem.attribute("tool-id");
    phase.date=QDate::fromString(phaseElem.attribute("date"),Qt::ISODate);
    return phase;
}

Phase XliffStorage::updatePhase(const Phase& phase)
{
    QDomElement phasegroup;
    QDomElement phaseElem=phaseElement(m_doc,phase.name,phasegroup);
    Phase prev=phaseFromElement(phaseElem);

    if (phaseElem.isNull()&&!phase.name.isEmpty())
    {
        phaseElem=phasegroup.appendChild(m_doc.createElement("phase")).toElement();
        phaseElem.setAttribute("phase-name",phase.name);
    }

    phaseElem.setAttribute("process-name", phase.process);
    if (!phase.company.isEmpty()) phaseElem.setAttribute("company-name", phase.company);
    phaseElem.setAttribute("contact-name", phase.contact);
    phaseElem.setAttribute("contact-email",phase.email);
    if (!phase.phone.isEmpty()) phaseElem.setAttribute("contact-phone",phase.phone);
    phaseElem.setAttribute("tool-id",      phase.tool);
    if (phase.date.isValid()) phaseElem.setAttribute("date",phase.date.toString(Qt::ISODate));
    return prev;
}

QList<Phase> XliffStorage::allPhases() const
{
    QList<Phase> result;
    QDomElement file=m_doc.elementsByTagName("file").at(0).toElement();
    QDomElement header=file.firstChildElement("header");
    QDomElement phasegroup=header.firstChildElement("phase-group");
    QDomElement phaseElem=phasegroup.firstChildElement("phase");
    while (!phaseElem.isNull())
    {
        result.append(phaseFromElement(phaseElem));
        phaseElem=phaseElem.nextSiblingElement("phase");
    }
    return result;
}

Phase XliffStorage::phase(const QString& name) const
{
    QDomElement phasegroup;
    QDomElement phaseElem=phaseElement(m_doc,name,phasegroup);

    return phaseFromElement(phaseElem);
}

QMap<QString,Tool> XliffStorage::allTools() const
{
    QMap<QString,Tool> result;
    QDomElement file=m_doc.elementsByTagName("file").at(0).toElement();
    QDomElement header=file.firstChildElement("header");
    QDomElement toolElem=header.firstChildElement("tool");
    while (!toolElem.isNull())
    {
        Tool tool;
        tool.tool       =toolElem.attribute("tool-id");
        tool.name       =toolElem.attribute("tool-name");
        tool.version    =toolElem.attribute("tool-version");
        tool.company    =toolElem.attribute("tool-company");

        result.insert(tool.tool, tool);
        toolElem=toolElem.nextSiblingElement("tool");
    }
    return result;
}

QStringList XliffStorage::sourceFiles(const DocPosition& pos) const
{
    QStringList result;

    QDomElement elem = unitForPos(pos.entry).firstChildElement("context-group");
    while (!elem.isNull())
    {
        if (elem.attribute("purpose").contains("location"))
        {
            QDomElement context = elem.firstChildElement("context");
            while (!context.isNull())
            {
                QString sourcefile;
                QString linenumber;
                if (context.attribute("context-type")=="sourcefile")
                    sourcefile=context.text();
                else if (context.attribute("context-type")=="linenumber")
                    linenumber=context.text();
                if (!( sourcefile.isEmpty()&&linenumber.isEmpty() ))
                    result.append(sourcefile+':'+linenumber);

                context=context.nextSiblingElement("context");
            }
        }

        elem=elem.nextSiblingElement("context-group");
    }
    //qSort(result);

    return result;
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

QVector<Note> XliffStorage::notes(const DocPosition& pos) const
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
    return result.toVector();
}

QVector<Note> XliffStorage::developerNotes(const DocPosition& pos) const
{
    Q_UNUSED(pos);
    //TODO
    return QVector<Note>();
}

Note XliffStorage::setNote(DocPosition pos, const Note& note)
{
    //kWarning()<<int(pos.form)<<note.content;
    QDomElement unit=unitForPos(pos.entry);
    QDomElement elem;
    Note oldNote;
    if (pos.form==-1 && !note.content.isEmpty())
    {
        QDomElement ref=unit.lastChildElement("note");
        elem=unit.insertAfter( m_doc.createElement("note"),ref).toElement();
        elem.appendChild(m_doc.createTextNode(""));
    }
    else
    {
        QDomNodeList list=unit.elementsByTagName("note");
        if (pos.form==-1) pos.form=list.size()-1;
        if (pos.form<list.size())
        {
            elem = unit.elementsByTagName("note").at(pos.form).toElement();
            initNoteFromElement(oldNote,elem);
        }
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
    QSet<QString> result;
    QDomNodeList notes=m_doc.elementsByTagName("note");
    int i=notes.size();
    while (--i>=0)
    {
        QString from=notes.at(i).toElement().attribute("from");
        if (!from.isEmpty())
            result.insert(from);
    }
    return result.toList();
}

QVector<Note> phaseNotes(QDomDocument m_doc, const QString& phasename, bool remove=false)
{
    QVector<Note> result;

    QDomElement phasegroup;
    QDomElement phaseElem=phaseElement(m_doc,phasename,phasegroup);

    QDomElement noteElem=phaseElem.firstChildElement("note");
    while (!noteElem.isNull())
    {
        Note note;
        initNoteFromElement(note,noteElem);
        result.append(note);
        QDomElement old=noteElem;
        noteElem=noteElem.nextSiblingElement("note");
        if (remove) phaseElem.removeChild(old);
    }
    return result;
}

QVector<Note> XliffStorage::phaseNotes(const QString& phasename) const
{
    return ::phaseNotes(m_doc, phasename, false);
}

QVector<Note> XliffStorage::setPhaseNotes(const QString& phasename, QVector<Note> notes)
{
    QVector<Note> result=::phaseNotes(m_doc, phasename, true);

    QDomElement phasegroup;
    QDomElement phaseElem=phaseElement(m_doc,phasename,phasegroup);

    foreach(const Note& note, notes)
    {
        QDomElement elem=phaseElem.appendChild(m_doc.createElement("note")).toElement();
        elem.appendChild(m_doc.createTextNode(note.content));
        if (!note.from.isEmpty()) elem.setAttribute("from",note.from);
        if (note.priority) elem.setAttribute("priority",note.priority);
    }

    return result;
}


QString XliffStorage::setPhase(const DocPosition& pos, const QString& phase)
{
    targetInsert(pos,QString()); //adds <taget> if needed

    QDomElement target=targetForPos(pos.entry);
    QString result=target.attribute("phase-name");
    if (phase.isEmpty())
        target.removeAttribute("phase-name");
    else
        target.setAttribute("phase-name",phase);

    return result;
}

QString XliffStorage::phase(const DocPosition& pos) const
{
    QDomElement target=targetForPos(pos.entry);
    return target.attribute("phase-name");
}

QStringList XliffStorage::context(const DocPosition& pos) const
{
    Q_UNUSED(pos);
    //TODO
    return QStringList(QString());
}

QStringList XliffStorage::matchData(const DocPosition& pos) const
{
    Q_UNUSED(pos);
    return QStringList();
}

QString XliffStorage::id(const DocPosition& pos) const
{
    return unitForPos(pos.entry).attribute("id");
}

bool XliffStorage::isPlural(const DocPosition& pos) const
{
    return m_plurals.contains(pos.entry);
}
/*
bool XliffStorage::isApproved(const DocPosition& pos) const
{
    return entries.at(m_map.at(pos.entry)).toElement().attribute("approved")=="yes";
}
void XliffStorage::setApproved(const DocPosition& pos, bool approved)
{
    static const char* const noyes[]={"no","yes"};
    entries.at(m_map.at(pos.entry)).toElement().setAttribute("approved",noyes[approved]);
}
*/

static const char* const states[]={
    "new", "needs-translation", "needs-l10n", "needs-adaptation", "translated",
    "needs-review-translation", "needs-review-l10n", "needs-review-adaptation", "final",
    "signed-off"};


static TargetState stringToState(const QString& state)
{
    int i=sizeof(states)/sizeof(char*);
    while (--i>0 && state!=states[i])
        ;
    return TargetState(i);
}

TargetState XliffStorage::setState(const DocPosition& pos, TargetState state)
{
    targetInsert(pos,QString()); //adds <taget> if needed
    QDomElement target=targetForPos(pos.entry);
    TargetState prev=stringToState(target.attribute("state"));
    target.setAttribute("state",states[state]);
    return prev;
}

TargetState XliffStorage::state(const DocPosition& pos) const
{
    QDomElement target=targetForPos(pos.entry);
    if (!target.hasAttribute("state") && unitForPos(pos.entry).attribute("approved")=="yes")
        return SignedOff;
    return stringToState(target.attribute("state"));
}

bool XliffStorage::isEmpty(const DocPosition& pos) const
{
    ContentEditingData data(ContentEditingData::CheckLength);
    return content(targetForPos(pos.entry),&data).isEmpty();
}

bool XliffStorage::isEquivTrans(const DocPosition& pos) const
{
    return targetForPos(pos.entry).attribute("equiv-trans")!="no";
}

void XliffStorage::setEquivTrans(const DocPosition& pos, bool equivTrans)
{
    targetForPos(pos.entry).setAttribute("equiv-trans",noyes[equivTrans]);
}

QDomElement XliffStorage::unitForPos(int pos) const
{
    if (pos<size())
        return entries.at(m_map.at(pos)).toElement();

    return binEntries.at(pos-size()).toElement();
}

QDomElement XliffStorage::targetForPos(int pos) const
{
    return unitForPos(pos).firstChildElement(bintargettarget[pos<size()]);
}

QDomElement XliffStorage::sourceForPos(int pos) const
{
    return unitForPos(pos).firstChildElement(binsourcesource[pos<size()]);
}

int XliffStorage::binUnitsCount() const
{
    return binEntries.size();
}

int XliffStorage::unitById(const QString& id) const
{
    return m_unitsById.contains(id)?m_unitsById.value(id):-1;
}


//END STORAGE TRANSLATION


