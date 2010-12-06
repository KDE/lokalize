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

#include <kdebug.h>

#include <QFile>
#include <QXmlSimpleReader>
#include <QXmlStreamReader>
#include <QBuffer>
#include <QApplication>

using namespace GlossaryNS;

static const QString defaultLang="en_US";

QStringList Glossary::idsForLangWord(const QString& lang, const QString& word) const
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
#define FETCH_SIZE 64

void GlossarySortFilterProxyModel::setFilterRegExp(const QString& s)
{
    //static const QRegExp lettersOnly("^[a-z]");
    QSortFilterProxyModel::setFilterRegExp(s);
    while (rowCount(QModelIndex())<FETCH_SIZE/2 && sourceModel()->canFetchMore(QModelIndex()))
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
    return !parent.isValid() && m_glossary->size()!=m_visibleCount;
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
    return m_visibleCount;
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

    QString id=glossary->id(index.row());
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

QString Glossary::generateNewId()
{
    // generate unique ID
    int idNumber=0;
    QList<int> busyIdNumbers;

    QString authorId(Settings::authorName().toLower());
    authorId.replace(' ','_');
    QRegExp rx('^'+authorId+"\\-([0-9]*)$");


    foreach(const QString& id, m_entriesById.keys())
    {
        if (rx.exactMatch(id))
            busyIdNumbers.append(rx.cap(1).toInt());
    }

    int i=removedIds.size();
    while(--i>=0)
    {
        if (rx.exactMatch(removedIds.at(i)))
            busyIdNumbers.append(rx.cap(1).toInt());
    }

    if (!busyIdNumbers.isEmpty())
    {
        qSort(busyIdNumbers);
        while (busyIdNumbers.contains(idNumber))
            ++idNumber;
    }

    return QString(authorId+"-%1").arg(idNumber);
}

QStringList Glossary::subjectFields() const
{
    QSet<QString> result;
    foreach(const QString& id, m_entriesById.keys())
        result.insert(subjectField(id));
    return result.toList();
}

QString Glossary::id(int index) const
{
    const QList<QString>& keys=m_entriesById.keys();
    if (index<keys.size())
        return m_entriesById.keys().at(index);
    return QString();
}

QStringList Glossary::terms(const QString& id, const QString& language) const
{
    QStringList result;
    QDomElement n = m_entriesById.value(id).firstChildElement("langSet");
    while (!n.isNull())
    {
        QString lang=n.attribute("xml:lang", defaultLang);
        if (lang=="en") //NOTE COMPAT
            lang=defaultLang;

        if (language==lang)
        {
            QString term=n.firstChildElement("ntig").firstChildElement("termGrp").firstChildElement("term").text();
            result<<term;
        }
        n = n.nextSiblingElement("langSet");
    }

    return result;
}


static void setText(QDomElement element, QString text)
{
    QDomNodeList children=element.childNodes();
    for (int i=0;i<children.count();i++)
    {
        if (children.at(i).isCharacterData())
        {
            children.at(i).toCharacterData().setData(text);
            text.clear();
        }
    }

    if (!text.isEmpty())
        element.appendChild( element.ownerDocument().createTextNode(text));
}

void Glossary::setTerm(const QString& id, QString lang, int index, const QString& termText)
{
    setClean(false);

    QDomElement n = m_entriesById.value(id).firstChildElement("langSet");
    QDomDocument document=n.ownerDocument();
    int i=0;
    while (!n.isNull())
    {
        QString nLang=n.attribute("xml:lang", defaultLang);
        if (lang=="en") //NOTE COMPAT
        {
            lang=defaultLang;
            n.setAttribute("xml:lang", defaultLang);
        }

        if (lang==nLang)
        {
            if (i==index)
            {
                QDomElement term=n.firstChildElement("ntig").firstChildElement("termGrp").firstChildElement("term");
                setText(term,termText);
                return;
            }
            i++;
        }
        n = n.nextSiblingElement("langSet");
    }
    n = m_entriesById.value(id).toElement();
    QDomElement langSet=n.appendChild( document.createElement("langSet")).toElement();
    langSet.setAttribute("xml:lang",lang);
    QDomElement ntig=langSet.appendChild( document.createElement("ntig")).toElement();
    QDomElement termGrp=ntig.appendChild( document.createElement("termGrp")).toElement();
    QDomElement term=termGrp.appendChild( document.createElement("term")).toElement();
    term.appendChild( document.createTextNode(termText));
}


QString Glossary::descrip(const QString& id, const QString& type) const
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

void Glossary::setDescrip(const QString& id, const QString& type, const QString& value)
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

QString Glossary::subjectField(const QString& id) const
{
    return descrip(id, "subjectField");
}

QString Glossary::definition(const QString& id) const
{
    return descrip(id, "definition");
}

void Glossary::setSubjectField(const QString& id, const QString& value)
{
    setDescrip(id, "subjectField", value);
}

void Glossary::setDefinition(const QString& id, const QString& value)
{
    setDescrip(id, "definition", value);
}

    
QStringList Glossary::terms(int index, const QString& language) const
{
    QStringList result;
    Q_ASSERT(index<m_entries.size());
    QDomElement n = m_entries.at(index).firstChildElement("langSet");
    while (!n.isNull())
    {
        QString lang=n.attribute("xml:lang", defaultLang);
        if (lang=="en") //NOTE COMPAT
            lang=defaultLang;

        if (language==lang)
            result<<n.firstChildElement("ntig").firstChildElement("termGrp").firstChildElement("term").text();
        n = n.nextSiblingElement("langSet");
    }

    return result;
}


//add words to the hash
// static void addToHash(const QMultiHash<QString,int>& wordHash,
//                       const QString& what,
//                       int index)
void Glossary::hashTermEntry(const QDomElement& termEntry)
{
    m_entriesById.insert(termEntry.attribute("id"),termEntry);

    QDomElement n = termEntry.firstChildElement("langSet");
    while (!n.isNull())
    {
        QString lang=n.attribute("xml:lang", defaultLang);
        if (lang=="en") //NOTE COMPAT
        {
            lang=defaultLang;
            //n.setAttribute("xml:lang", defaultLang); freezes lokalize on large TBX
        }

        QString term=n.firstChildElement("ntig").firstChildElement("termGrp").firstChildElement("term").text();
        foreach(const QString& word, term.split(' ',QString::SkipEmptyParts))
            idsByLangWord[lang].insert(stem(lang,word),termEntry.attribute("id"));

        n = n.nextSiblingElement("langSet");
    }
}

void Glossary::unhashTermEntry(const QDomElement& termEntry)
{
    m_entriesById.remove(termEntry.attribute("id"));

    QDomElement n = termEntry.firstChildElement("langSet");
    while (!n.isNull())
    {
        QString lang=n.attribute("xml:lang", defaultLang);
        if (lang=="en") //NOTE COMPAT
            lang=defaultLang;

        QString term=n.firstChildElement("ntig").firstChildElement("termGrp").firstChildElement("term").text();
        foreach(const QString& word, term.split(' ',QString::SkipEmptyParts))
            idsByLangWord[lang].remove(stem(lang,word),termEntry.attribute("id"));

        n = n.nextSiblingElement("langSet");
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

void Glossary::remove(const QString& id)
{
    if (!m_entriesById.contains(id))
        return;

    QDomElement entry = m_entriesById.value(id);
    if (entry.nextSibling().isCharacterData())
        entry.parentNode().removeChild(entry.nextSibling()); //nice formatting
    entry.parentNode().removeChild(entry);
    m_entriesById.remove(id);

    unhashTermEntry(entry);
    removedIds.append(id); //for new id generation goodness

    setClean(false);
}

static void appendTerm(QDomElement entry, const QString& termText, const QString& lang)
{
    QDomDocument doc=entry.ownerDocument();

    QDomElement source=doc.createElement("langSet"); entry.appendChild(source);
    source.setAttribute("xml:lang", lang);
    QDomElement ntig=doc.createElement("ntig"); source.appendChild(ntig);
    QDomElement termGrp=doc.createElement("termGrp"); ntig.appendChild(termGrp);
    QDomElement term=doc.createElement("term"); termGrp.appendChild(term);
    term.appendChild(doc.createTextNode(termText));
}

QString Glossary::append(const QStringList& sourceTerms, const QStringList& targetTerms)
{
    if (!m_doc.elementsByTagName("body").count())
        return QString();

    setClean(false);
    QDomElement termEntry=m_doc.createElement("termEntry");
    m_doc.elementsByTagName("body").at(0).appendChild(termEntry);
    
    //m_entries=m_doc.elementsByTagName("termEntry");
    
    QString id=generateNewId();
    termEntry.setAttribute("id", id);
    
    foreach (QString sourceTerm, sourceTerms)
        appendTerm(termEntry, sourceTerm, Project::instance()->sourceLangCode());
    foreach (QString targetTerm, targetTerms)
        appendTerm(termEntry, targetTerm, Project::instance()->targetLangCode());

    hashTermEntry(termEntry);

    return id;
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
QString GlossaryModel::appendRow(const QString& _english, const QString& _target)
{
    bool notify=!canFetchMore(QModelIndex());
    if (notify)
        beginInsertRows(QModelIndex(),rowCount(),rowCount());

    QString id=m_glossary->append(QStringList(_english), QStringList(_target));

    if (notify)
    {
        m_visibleCount++;
        endInsertRows();
    }
    return id;
}

//END EDITING

#include "glossary.moc"
