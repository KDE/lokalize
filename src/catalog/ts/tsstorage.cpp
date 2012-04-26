/*
Copyright 2008-2012 Nick Shaforostoff <shaforostoff@kde.ru>

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

#include "tsstorage.h"

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

static const QString names[]={"source" ,"translation","oldsource" ,"translatorcomment","comment" ,"name" ,"numerus"};
enum TagNames                {SourceTag,TargetTag    ,OldSourceTag,NoteTag            ,DevNoteTag,NameTag,PluralTag};

static const QString attrnames[]={"location"  ,"type"  ,"obsolete"};
enum AttrNames                   {LocationAttr,TypeAttr,ObsoleteAttr};

static const QString attrvalues[]={"obsolete"};
enum AttValues                    {ObsoleteVal};

TsStorage::TsStorage()
 : CatalogStorage()
{
}

TsStorage::~TsStorage()
{
}

int TsStorage::capabilities() const
{
    return 0;//MultipleNotes;
}

//BEGIN OPEN/SAVE

int TsStorage::load(QIODevice* device)
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


    QDomElement file=m_doc.elementsByTagName("TS").at(0).toElement();
    m_sourceLangCode=file.attribute("sourcelanguage");
    m_targetLangCode=file.attribute("language");
    m_numberOfPluralForms=numberOfPluralFormsForLangCode(m_targetLangCode);

    //Create entry mapping.
    //Along the way: for langs with more than 2 forms
    //we create any form-entries additionally needed

    entries=m_doc.elementsByTagName("message");
    int size=entries.size();

    kWarning()<<chrono.elapsed();
    return 0;
}

bool TsStorage::save(QIODevice* device, bool belongsToProject)
{
    QTextStream stream(device);
    m_doc.save(stream,4);
    return true;
}
//END OPEN/SAVE

//BEGIN STORAGE TRANSLATION

int TsStorage::size() const
{
    //return m_map.size();

    return entries.size();
}




/**
 * helper structure used during XLIFF XML walk-through
 */
struct ContentEditingData
{
    enum ActionType{Get,DeleteText,InsertText,CheckLength};

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
                cData=c.data();
            }
            //else
            //    if (data&&data->pos!=-1/*&& n.nextSibling().isNull()*/)
            //        kWarning()<<"arg!"<<startingPos<<"data->pos"<<data->pos;

            result += cData;
            startingPos+=cData.size();
        }
        n = n.nextSibling();
    }
    if (!seenCharacterDataAfterElement)
    {
        //add empty charData child so that user could add some text
        elem.appendChild( elem.ownerDocument().createTextNode(QString()) );
    }

    return result;
}



//flat-model interface (ignores XLIFF grouping)

CatalogString TsStorage::catalogString(QDomElement contentElement) const
{
    CatalogString catalogString;
    ContentEditingData data(ContentEditingData::Get);
    catalogString.string=content(contentElement, &data);
    return catalogString;
}

CatalogString TsStorage::catalogString(const DocPosition& pos) const
{
    return catalogString(pos.part==DocPosition::Target?targetForPos(pos):sourceForPos(pos.entry));
}

CatalogString TsStorage::targetWithTags(DocPosition pos) const
{
    return catalogString(targetForPos(pos));
}
CatalogString TsStorage::sourceWithTags(DocPosition pos) const
{
    return catalogString(sourceForPos(pos.entry));
}

QString TsStorage::source(const DocPosition& pos) const
{
    return content(sourceForPos(pos.entry));
}
QString TsStorage::target(const DocPosition& pos) const
{
    return content(targetForPos(pos));
}


void TsStorage::targetDelete(const DocPosition& pos, int count)
{
    ContentEditingData data(pos.offset,count);
    content(targetForPos(pos),&data);
}

void TsStorage::targetInsert(const DocPosition& pos, const QString& arg)
{
    kWarning()<<pos.entry<<arg;
    QDomElement targetEl=targetForPos(pos);
    //BEGIN add <*target>
    if (targetEl.isNull())
    {
        QDomNode unitEl=unitForPos(pos.entry);
        QDomNode refNode=unitEl.firstChildElement(names[SourceTag]);
        targetEl = unitEl.insertAfter(m_doc.createElement(names[TargetTag]),refNode).toElement();
  
        if (pos.entry<size())
        {
            targetEl.appendChild(m_doc.createTextNode(arg));//i bet that pos.offset is 0 ;)
            return;
        }
    }
    //END add <*target>
    if (arg.isEmpty()) return; //means we were called just to add <taget> tag

    ContentEditingData data(pos.offset,arg);
    content(targetEl,&data);
}

void TsStorage::setTarget(const DocPosition& pos, const QString& arg)
{
    Q_UNUSED(pos);
    Q_UNUSED(arg);
//TODO
}


QVector<AltTrans> TsStorage::altTrans(const DocPosition& pos) const
{
    QVector<AltTrans> result;

    QString oldsource=content(unitForPos(pos.entry).firstChildElement(names[OldSourceTag]));
    if (!oldsource.isEmpty())
        result<<AltTrans(CatalogString(oldsource), i18n("Previous source value, saved by lupdate tool"));

    return result;
}


QStringList TsStorage::sourceFiles(const DocPosition& pos) const
{
    QStringList result;

    QDomElement elem = unitForPos(pos.entry).firstChildElement(attrnames[LocationAttr]);
    while (!elem.isNull())
    {
        QString sourcefile=elem.attribute("filename");
        QString linenumber=elem.attribute("line");
        if (!( sourcefile.isEmpty()&&linenumber.isEmpty() ))
            result.append(sourcefile+':'+linenumber);

        elem=elem.nextSiblingElement(attrnames[LocationAttr]);
    }
    //qSort(result);

    return result;
}

QVector<Note> TsStorage::notes(const DocPosition& pos) const
{
    QVector<Note> result;

    QDomElement elem = unitForPos(pos.entry).firstChildElement(names[NoteTag]);
    while (!elem.isNull())
    {
        Note note;
        note.content=elem.text();
        result.append(note);

        elem=elem.nextSiblingElement(names[NoteTag]);
    }
    return result;
}

QVector<Note> TsStorage::developerNotes(const DocPosition& pos) const
{
    QVector<Note> result;

    QDomElement elem = unitForPos(pos.entry).firstChildElement(names[DevNoteTag]);
    while (!elem.isNull())
    {
        Note note;
        note.content=elem.text();
        result.append(note);

        elem=elem.nextSiblingElement(names[DevNoteTag]);
    }
    return result;
}

Note TsStorage::setNote(DocPosition pos, const Note& note)
{
    //kWarning()<<int(pos.form)<<note.content;
    QDomElement unit=unitForPos(pos.entry);
    QDomElement elem;
    Note oldNote;
    if (pos.form==-1 && !note.content.isEmpty())
    {
        QDomElement ref=unit.lastChildElement(names[NoteTag]);
        elem=unit.insertAfter( m_doc.createElement(names[NoteTag]),ref).toElement();
        elem.appendChild(m_doc.createTextNode(QString()));
    }
    else
    {
        QDomNodeList list=unit.elementsByTagName(names[NoteTag]);
        if (pos.form==-1) pos.form=list.size()-1;
        if (pos.form<list.size())
        {
            elem = unit.elementsByTagName(names[NoteTag]).at(pos.form).toElement();
            oldNote.content=elem.text();
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
        ContentEditingData data(0,note.content);
        content(elem,&data);
    }
    else
        unit.removeChild(elem);

    return oldNote;
}

QStringList TsStorage::context(const DocPosition& pos) const
{
    QStringList result;

    QDomElement unit=unitForPos(pos.entry);
    QDomElement context=unit.parentNode().toElement();
    //if (context.isNull())
    //    return result;

    QDomElement name=context.firstChildElement(names[NameTag]);
    if (name.isNull())
        return result;
 
    result.append(name.text());
    return result;
}

QStringList TsStorage::matchData(const DocPosition& pos) const
{
    Q_UNUSED(pos);
    return QStringList();
}

QString TsStorage::id(const DocPosition& pos) const
{
    QString result=source(pos);
    result.remove('\n');
    QStringList ctxt=context(pos);
    if (ctxt.size())
        result.prepend(ctxt.first());
    return result;
}

bool TsStorage::isPlural(const DocPosition& pos) const
{
    QDomElement unit=unitForPos(pos.entry);

    return unit.hasAttribute(names[PluralTag]);
}

void TsStorage::setApproved(const DocPosition& pos, bool approved)
{
    targetInsert(pos,QString()); //adds <taget> if needed
    QDomElement target=unitForPos(pos.entry).firstChildElement(names[TargetTag]); //asking directly to bypass plural state detection
    if (target.attribute(attrnames[TypeAttr])==attrvalues[ObsoleteVal])
        return;
    if (approved)
        target.removeAttribute(attrnames[TypeAttr]);
    else
        target.setAttribute(attrnames[TypeAttr],"unfinished");
}

bool TsStorage::isApproved(const DocPosition& pos) const
{
    QDomElement target=unitForPos(pos.entry).firstChildElement(names[TargetTag]);
    return !target.hasAttribute(attrnames[TypeAttr]);
}

bool TsStorage::isObsolete(int entry) const
{
    QDomElement target=unitForPos(entry).firstChildElement(names[TargetTag]);
    return target.attribute(attrnames[TypeAttr])==attrvalues[ObsoleteVal];
}

bool TsStorage::isEmpty(const DocPosition& pos) const
{
    ContentEditingData data(ContentEditingData::CheckLength);
    return content(targetForPos(pos),&data).isEmpty();
}

bool TsStorage::isEquivTrans(const DocPosition& pos) const
{
    return true;//targetForPos(pos.entry).attribute("equiv-trans")!="no";
}

void TsStorage::setEquivTrans(const DocPosition& pos, bool equivTrans)
{
    //targetForPos(pos.entry).setAttribute("equiv-trans",noyes[equivTrans]);
}

QDomElement TsStorage::unitForPos(int pos) const
{
    return entries.at(pos).toElement();
}

QDomElement TsStorage::targetForPos(DocPosition pos) const
{
    QDomElement unit=unitForPos(pos.entry);
    QDomElement translation=unit.firstChildElement(names[TargetTag]);
    if (!unit.hasAttribute(names[PluralTag]))
        return translation;
    
    if (pos.form==-1) pos.form=0;
    
    QDomNodeList forms=translation.elementsByTagName("numerusform");
    while (pos.form>=forms.size())
        translation.appendChild( unit.ownerDocument().createElement("numerusform") );
    return forms.at(pos.form).toElement();
}

QDomElement TsStorage::sourceForPos(int pos) const
{
    return unitForPos(pos).firstChildElement(names[SourceTag]);
}

//END STORAGE TRANSLATION


