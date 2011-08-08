/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2011 by Nick Shaforostoff <shafff@ukr.net>

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

**************************************************************************** */

#include "glossary.h"

#include "stemming.h"
// #include "tbxparser.h"
#include "project.h"
#include "prefs_lokalize.h"
#include "domroutines.h"

#include <kdebug.h>

#include <QFile>
#include <QXmlSimpleReader>
#include <QXmlStreamReader>
#include <QBuffer>
#include <QApplication>

using namespace GlossaryNS;

static const QString defaultLang="en_US";
static const QString xmlLang="xml:lang";
static const QString ntig="ntig";
static const QString tig="tig";
static const QString termGrp="termGrp";
static const QString langSet="langSet";
static const QString term="term";
static const QString id="id";



QList<QByteArray> Glossary::idsForLangWord(const QString& lang, const QString& word) const
{
    //return glossary.wordHash.values(word);
    return idsByLangWord[lang].values(word);
}


Glossary::Glossary(QObject* parent)
 : QObject(parent)
{
}


//BEGIN DISK
bool Glossary::load(const QString& newPath)
{
    QTime a;a.start();
//BEGIN NEW
    QIODevice* device=new QFile(newPath);
    if (!device->open(QFile::ReadOnly | QFile::Text))
    {
        delete device;
        //return;
        device=new QBuffer();
        static_cast<QBuffer*>(device)->setData(QByteArray(
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<!DOCTYPE martif PUBLIC \"ISO 12200:1999A//DTD MARTIF core (DXFcdV04)//EN\" \"TBXcdv04.dtd\">\n"
"<martif type=\"TBX\" xml:lang=\"en_US\">\n"
"    <text>\n"
"        <body>\n"
"        </body>\n"
"    </text>\n"
"</martif>\n"
));
    }

    QXmlSimpleReader reader;
    //reader.setFeature("http://qtsoftware.com/xml/features/report-whitespace-only-CharData",true);
    reader.setFeature("http://xml.org/sax/features/namespaces",false);
    QXmlInputSource source(device);

    QDomDocument newDoc;
    QString errorMsg;
    int errorLine;//+errorColumn;
    bool success=newDoc.setContent(&source, &reader, &errorMsg, &errorLine/*,errorColumn*/);

    delete device;

    if (!success)
    {
        kWarning()<<errorMsg;
        return false; //errorLine+1;
    }
    clear();//does setClean(true);
    m_path=newPath;
    m_doc=newDoc;

    //QDomElement file=m_doc.elementsByTagName("file").at(0).toElement();
    m_entries=m_doc.elementsByTagName("termEntry");
    for(int i=0;i<m_entries.size();i++)
        hashTermEntry(m_entries.at(i).toElement());
    m_idsForEntriesById=m_entriesById.keys();

//END NEW
#if 0
    TbxParser parser(this);
    QXmlSimpleReader reader1;
    reader1.setContentHandler(&parser);

    QFile file(p);
    if (!file.open(QFile::ReadOnly | QFile::Text))
         return;
    QXmlInputSource xmlInputSource(&file);
    if (!reader1.parse(xmlInputSource))
         kWarning() << "failed to load "<< path;
#endif
    emit loaded();
    return true;
}

bool Glossary::save()
{
    if (m_path.isEmpty())
        return false;
    
    QFile* device=new QFile(m_path);
    if (!device->open(QFile::WriteOnly | QFile::Truncate))
    {
        device->deleteLater();
        return false;
    }
    QTextStream stream(device);
    m_doc.save(stream,2);

    device->deleteLater();

    setClean(true);
    return true;
}

void Glossary::setClean(bool clean)
{
    m_clean=clean;
    emit changed();//may be emitted multiple times in a row. so what? :)
}

//END DISK

//BEGIN MODEL
#define FETCH_SIZE 128

void GlossarySortFilterProxyModel::setFilterRegExp(const QString& s)
{
    if (!sourceModel())
      return;

    //static const QRegExp lettersOnly("^[a-z]");
    QSortFilterProxyModel::setFilterRegExp(s);

    fetchMore(QModelIndex());
}

void GlossarySortFilterProxyModel::fetchMore(const QModelIndex& parent)
{
    int expectedCount=rowCount()+FETCH_SIZE/2;
    while (rowCount(QModelIndex())<expectedCount && sourceModel()->canFetchMore(QModelIndex()))
    {
        sourceModel()->fetchMore(QModelIndex());
        //qDebug()<<"filter:"<<rowCount(QModelIndex())<<"/"<<sourceModel()->rowCount();
        qApp->processEvents();
    }
}

GlossaryModel::GlossaryModel(QObject* parent)
 : QAbstractListModel(parent)
 , m_visibleCount(0)
 , m_glossary(Project::instance()->glossary())
{
    connect(m_glossary, SIGNAL(loaded()), this, SLOT(forceReset()));
}

void GlossaryModel::forceReset()
{
    beginResetModel();
    m_visibleCount=0;
    endResetModel();
}

bool GlossaryModel::canFetchMore(const QModelIndex& parent) const
{
    return false;//!parent.isValid() && m_glossary->size()!=m_visibleCount;
}

void GlossaryModel::fetchMore(const QModelIndex& parent)
{
    int newVisibleCount=qMin(m_visibleCount+FETCH_SIZE,m_glossary->size());
    beginInsertRows(parent, m_visibleCount, newVisibleCount-1);
    m_visibleCount=newVisibleCount;
    endInsertRows();
}

int GlossaryModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_glossary->size();//m_visibleCount;
}

QVariant GlossaryModel::headerData( int section, Qt::Orientation /*orientation*/, int role) const
{
    if (role!=Qt::DisplayRole)
        return QVariant();

    switch (section)
    {
        //case ID: return i18nc("@title:column","ID");
        case English: return i18nc("@title:column Original text","Source");;
        case Target: return i18nc("@title:column Text in target language","Target");
        case SubjectField: return i18nc("@title:column","Subject Field");
    }
    return QVariant();
}

QVariant GlossaryModel::data(const QModelIndex& index, int role) const
{
    //if (role==Qt::SizeHintRole)
    //    return QVariant(QSize(50, 30));

    if (role!=Qt::DisplayRole)
        return QVariant();

    static const QString nl=QString(" ")+QChar(0x00B7)+' ';
    static Project* project=Project::instance();
    Glossary* glossary=m_glossary;

    QByteArray id=glossary->id(index.row());
    switch (index.column())
    {
        case ID:      return id;
        case English: return glossary->terms(id, project->sourceLangCode()).join(nl);
        case Target:  return glossary->terms(id, project->targetLangCode()).join(nl);
        case SubjectField: return glossary->subjectField(id);
    }
    return QVariant();
}

/*
QModelIndex GlossaryModel::index (int row,int column,const QModelIndex& parent) const
{
    return createIndex (row, column);
}
*/

int GlossaryModel::columnCount(const QModelIndex&) const
{
    return GlossaryModelColumnCount;
}

/*
Qt::ItemFlags GlossaryModel::flags ( const QModelIndex & index ) const
{
    return Qt::ItemIsSelectable|Qt::ItemIsEnabled;
    //if (index.column()==FuzzyFlag)
    //    return Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled;
    //return QAbstractItemModel::flags(index);
}
*/


//END MODEL general (GlossaryModel continues below)

//BEGIN EDITING

QByteArray Glossary::generateNewId()
{
    // generate unique ID
    int idNumber=0;
    QList<int> busyIdNumbers;

    QString authorId(Settings::authorName().toLower());
    authorId.replace(' ','_');
    QRegExp rx('^'+authorId+"\\-([0-9]*)$");


    foreach(const QByteArray& id, m_idsForEntriesById)
    {
        if (rx.exactMatch(QString::fromLatin1(id)))
            busyIdNumbers.append(rx.cap(1).toInt());
    }

    int i=removedIds.size();
    while(--i>=0)
    {
        if (rx.exactMatch(QString::fromLatin1(removedIds.at(i))))
            busyIdNumbers.append(rx.cap(1).toInt());
    }

    if (!busyIdNumbers.isEmpty())
    {
        qSort(busyIdNumbers);
        while (busyIdNumbers.contains(idNumber))
            ++idNumber;
    }

    return QString(authorId+"-%1").arg(idNumber).toLatin1();
}

QStringList Glossary::subjectFields() const
{
    QSet<QString> result;
    foreach(const QByteArray& id, m_idsForEntriesById)
        result.insert(subjectField(id));
    return result.toList();
}

QByteArray Glossary::id(int index) const
{
    if (index<m_idsForEntriesById.size())
        return m_idsForEntriesById.at(index);
    return QByteArray();
}

QStringList Glossary::terms(const QByteArray& id, const QString& language) const
{
    QStringList result;
    QDomElement n = m_entriesById.value(id).firstChildElement(langSet);
    while (!n.isNull())
    {
        QString lang=n.attribute(xmlLang, defaultLang);
        if (lang=="en") //NOTE COMPAT
            lang=defaultLang;
        lang.replace('-', '_');

        if (language==lang)
        {
            QDomElement ntigElem=n.firstChildElement(ntig);
            while (!ntigElem.isNull())
            {
                result<<ntigElem.firstChildElement(termGrp).firstChildElement(term).text();
                ntigElem=ntigElem.nextSiblingElement(ntig);
            }
            QDomElement tigElem=n.firstChildElement(tig);
            while (!tigElem.isNull())
            {
                result<<tigElem.firstChildElement(term).text();
                tigElem=tigElem.nextSiblingElement(tig);
            }
        }

        n = n.nextSiblingElement(langSet);
    }

    return result;
}

void Glossary::setTerm(const QByteArray& id, QString lang, int index, const QString& termText)
{
    setClean(false);

    QDomElement n = m_entriesById.value(id).firstChildElement(langSet);
    QDomElement ourLangSetElement; //will reference the lang we want if it exists
    QDomDocument document=n.ownerDocument();
    int i=0;
    while (!n.isNull())
    {
        QString nLang=n.attribute(xmlLang, defaultLang);
        nLang.replace('-','_');
        if (lang=="en") //NOTE COMPAT
        {
            lang=defaultLang;
            nLang.replace('_','-');
            n.setAttribute(xmlLang, defaultLang);
        }

        if (lang==nLang)
        {
            ourLangSetElement=n;
            QDomElement ntigElem=n.firstChildElement(ntig);
            while(!ntigElem.isNull())
            {
                if (i==index)
                {
                    QDomElement termElement=ntigElem.firstChildElement(termGrp).firstChildElement(term);
                    setText(termElement,termText);
                    return;
                }
                ntigElem = ntigElem.nextSiblingElement(ntig);
                i++;
            }
            QDomElement tigElem=n.firstChildElement(tig);
            while(!tigElem.isNull())
            {
                if (i==index)
                {
                    QDomElement termElement=tigElem.firstChildElement(term);
                    setText(termElement,termText);
                    return;
                }
                tigElem = tigElem.nextSiblingElement(tig);
                i++;
            }
        }
        n = n.nextSiblingElement(langSet);
    }
    n = m_entriesById.value(id).toElement();
    if (ourLangSetElement.isNull())
    {
        ourLangSetElement=n.appendChild( document.createElement(langSet)).toElement();
        lang.replace('_', '-');
        ourLangSetElement.setAttribute(xmlLang,lang);
    }
/*
    QDomElement ntigElement=ourLangSetElement.appendChild( document.createElement(ntig)).toElement();
    QDomElement termGrpElement=ntigElement.appendChild( document.createElement(termGrp)).toElement();
    QDomElement termElement=termGrpElement.appendChild( document.createElement(term)).toElement();
    termElement.appendChild( document.createTextNode(termText));
*/
    QDomElement tigElement=ourLangSetElement.appendChild( document.createElement(tig)).toElement();
    QDomElement termElement=tigElement.appendChild( document.createElement(term)).toElement();
    termElement.appendChild( document.createTextNode(termText));
}


QString Glossary::descrip(const QByteArray& id, const QString& lang, const QString& type) const
{
    QDomElement n = m_entriesById.value(id).firstChildElement("descrip");
    while (!n.isNull())
    {
        if (n.attribute("type")==type)
            return n.text();

        n = n.nextSiblingElement("descrip");
    }

    return QString();
}

void Glossary::setDescrip(const QByteArray& id, const QString& lang, const QString& type, const QString& value)
{
    setClean(false);

    QDomElement n = m_entriesById.value(id).firstChildElement("descrip");
    QDomDocument document=n.ownerDocument();
    while (!n.isNull())
    {
        if (n.attribute("type")==type)
            return setText(n,value);;

        n = n.nextSiblingElement("descrip");
    }
    //create new descrip element
    n = m_entriesById.value(id).toElement();
    QDomElement descrip=n.insertBefore( document.createElement("descrip"), n.firstChild() ).toElement();
    descrip.setAttribute("type",type);
    descrip.appendChild( document.createTextNode(value));
}

QString Glossary::subjectField(const QByteArray& id, const QString& lang) const
{
    return descrip(id, lang, "subjectField");
}

QString Glossary::definition(const QByteArray& id, const QString& lang) const
{
    return descrip(id, lang, "definition");
}

void Glossary::setSubjectField(const QByteArray& id, const QString& lang, const QString& value)
{
    setDescrip(id, lang, "subjectField", value);
}

void Glossary::setDefinition(const QByteArray& id, const QString& lang, const QString& value)
{
    setDescrip(id, lang, "definition", value);
}



//add words to the hash
// static void addToHash(const QMultiHash<QString,int>& wordHash,
//                       const QString& what,
//                       int index)
void Glossary::hashTermEntry(const QDomElement& termEntry)
{
    QByteArray entryId=termEntry.attribute(::id).toLatin1();
    if (entryId.isEmpty())
        return;

    m_entriesById.insert(entryId,termEntry);

    QDomElement n = termEntry.firstChildElement(langSet);
    while (!n.isNull())
    {
        QString lang=n.attribute(xmlLang, defaultLang);
        if (lang=="en") //NOTE COMPAT
        {
            lang=defaultLang;
            //n.setAttribute(xmlLang, defaultLang); freezes lokalize on large TBX
        }

        QString termText=n.firstChildElement(ntig).firstChildElement(termGrp).firstChildElement(term).text();
        foreach(const QString& word, termText.split(' ',QString::SkipEmptyParts))
            idsByLangWord[lang].insert(stem(lang,word),entryId);

        n = n.nextSiblingElement(langSet);
    }
}

void Glossary::unhashTermEntry(const QDomElement& termEntry)
{
    QByteArray entryId=termEntry.attribute(::id).toLatin1();
    m_entriesById.remove(entryId);

    QDomElement n = termEntry.firstChildElement(langSet);
    while (!n.isNull())
    {
        QString lang=n.attribute(xmlLang, defaultLang);
        if (lang=="en") //NOTE COMPAT
            lang=defaultLang;

        QString termText=n.firstChildElement(ntig).firstChildElement(termGrp).firstChildElement(term).text();
        foreach(const QString& word, termText.split(' ',QString::SkipEmptyParts))
            idsByLangWord[lang].remove(stem(lang,word),entryId);

        n = n.nextSiblingElement(langSet);
    }
}

#if 0
void Glossary::hashTermEntry(int index)
{
    Q_ASSERT(index<termList.size());
    foreach(const QString& term, termList_.at(index).english)
    {
        foreach(const QString& word, term.split(' ',QString::SkipEmptyParts))
            wordHash_.insert(stem(Project::instance()->sourceLangCode(),word),index);
    }
}

void Glossary::unhashTermEntry(int index)
{
    Q_ASSERT(index<termList.size());
    foreach(const QString& term, termList_.at(index).english)
    {
        foreach(const QString& word, term.split(' ',QString::SkipEmptyParts))
            wordHash_.remove(stem(Project::instance()->sourceLangCode(),word),index);
    }
}
#endif

void Glossary::remove(const QByteArray& id)
{
    if (!m_entriesById.contains(id))
        return;

    QDomElement entry = m_entriesById.value(id);
    if (entry.nextSibling().isCharacterData())
        entry.parentNode().removeChild(entry.nextSibling()); //nice formatting
    entry.parentNode().removeChild(entry);
    m_entriesById.remove(id);

    unhashTermEntry(entry);
    m_idsForEntriesById=m_entriesById.keys();
    removedIds.append(id); //for new id generation goodness

    setClean(false);
}

static void appendTerm(QDomElement langSetElem, const QString& termText)
{
    QDomDocument doc=langSetElem.ownerDocument();
/*
    QDomElement ntigElement=doc.createElement(ntig); langSetElem.appendChild(ntigElement);
    QDomElement termGrpElement=doc.createElement(termGrp); ntigElement.appendChild(termGrpElement);
    QDomElement termElement=doc.createElement(term); termGrpElement.appendChild(termElement);
    termElement.appendChild(doc.createTextNode(termText));
*/
    QDomElement tigElement=doc.createElement(tig); langSetElem.appendChild(tigElement);
    QDomElement termElement=doc.createElement(term); tigElement.appendChild(termElement);
    termElement.appendChild(doc.createTextNode(termText));
}

QByteArray Glossary::append(const QStringList& sourceTerms, const QStringList& targetTerms)
{
    if (!m_doc.elementsByTagName("body").count())
        return QByteArray();

    setClean(false);
    QDomElement termEntry=m_doc.createElement("termEntry");
    m_doc.elementsByTagName("body").at(0).appendChild(termEntry);

    //m_entries=m_doc.elementsByTagName("termEntry");

    QByteArray newId=generateNewId();
    termEntry.setAttribute(::id, QString::fromLatin1(newId));

    QDomElement sourceElem=m_doc.createElement(langSet); termEntry.appendChild(sourceElem);
    sourceElem.setAttribute(xmlLang, Project::instance()->sourceLangCode());
    foreach (QString sourceTerm, sourceTerms)
        appendTerm(sourceElem, sourceTerm);

    QDomElement targetElem=m_doc.createElement(langSet); termEntry.appendChild(targetElem);
    targetElem.setAttribute(xmlLang, Project::instance()->targetLangCode());
    foreach (QString targetTerm, targetTerms)
        appendTerm(targetElem, targetTerm);

    hashTermEntry(termEntry);
    m_idsForEntriesById=m_entriesById.keys();

    return newId;
}

void Glossary::append(const QString& _english, const QString& _target)
{
    append(QStringList(_english), QStringList(_target));
}

void Glossary::clear()
{
    setClean(true);
    //path.clear();
    idsByLangWord.clear();
    m_entriesById.clear();

    removedIds.clear();
    changedIds_.clear();
    addedIds_.clear();
    wordHash_.clear();
    termList_.clear();
    langWordEntry_.clear();
    subjectFields_=QStringList(QLatin1String(""));
    
    m_doc.clear();
}


bool GlossaryModel::removeRows(int row, int count, const QModelIndex& parent)
{
    beginRemoveRows(parent,row,row+count-1);

    Glossary* glossary=Project::instance()->glossary();
    int i=row+count;
    while (--i>=row)
        glossary->remove(glossary->id(i));

    endRemoveRows();
    return true;
}

// bool GlossaryModel::insertRows(int row,int count,const QModelIndex& parent)
// {
//     if (row!=rowCount())
//         return false;
QByteArray GlossaryModel::appendRow(const QString& _english, const QString& _target)
{
    bool notify=!canFetchMore(QModelIndex());
    if (notify)
        beginInsertRows(QModelIndex(),rowCount(),rowCount());

    QByteArray id=m_glossary->append(QStringList(_english), QStringList(_target));

    if (notify)
    {
        m_visibleCount++;
        endInsertRows();
    }
    return id;
}

//END EDITING

#include "glossary.moc"
