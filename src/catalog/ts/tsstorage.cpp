/*
Copyright 2008-2014 Nick Shaforostoff <shaforostoff@kde.ru>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

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

#include "lokalize_debug.h"

#include "gettextheader.h"
#include "project.h"
#include "version.h"
#include "prefs_lokalize.h"

#include <QProcess>
#include <QString>
#include <QMap>
#include <QDomDocument>
#include <QElapsedTimer>
#include <QPair>
#include <QList>
#include <QXmlSimpleReader>

#include <klocalizedstring.h>

#ifdef Q_OS_WIN
#define U QLatin1String
#else
#define U QStringLiteral
#endif

//static const char* const noyes[]={"no","yes"};

static const QString names[] = {U("source"), U("translation"), U("oldsource"), U("translatorcomment"), U("comment"), U("name"), U("numerus")};
enum TagNames                {SourceTag, TargetTag, OldSourceTag, NoteTag, DevNoteTag, NameTag, PluralTag};

static const QString attrnames[] = {U("location"), U("type"), U("obsolete")};
enum AttrNames                   {LocationAttr, TypeAttr, ObsoleteAttr};

static const QString attrvalues[] = {U("obsolete"), U("vanished")};
enum AttValues                    {ObsoleteVal, VanishedVal};

static QString protect(const QString & str)
{
    QString p = str;
    p.replace(QLatin1Char('&'), QLatin1String("&amp;"));
    p.replace(QLatin1Char('\"'), QLatin1String("&quot;"));
    p.replace(QLatin1Char('>'),  QLatin1String("&gt;"));
    p.replace(QLatin1Char('<'),  QLatin1String("&lt;"));
    p.replace(QLatin1Char('\''), QLatin1String("&apos;"));
    return p;
}

static QString unprotect(const QString & str)
{
    QString p = str;
    p.replace(QLatin1String("&amp;"), QLatin1String("&"));
    p.replace(QLatin1String("&quot;"), QLatin1String("\""));
    p.replace(QLatin1String("&gt;"), QLatin1String(">"));
    p.replace(QLatin1String("&lt;"), QLatin1String("<"));
    p.replace(QLatin1String("&apos;"), QLatin1String("\'"));
    return p;
}

TsStorage::TsStorage()
    : CatalogStorage()
{
}

int TsStorage::capabilities() const
{
    return 0;//MultipleNotes;
}

//BEGIN OPEN/SAVE

int TsStorage::load(QIODevice* device)
{
    QElapsedTimer chrono; chrono.start();

    QXmlSimpleReader reader;
    reader.setFeature(QStringLiteral("http://qt-project.org/xml/features/report-whitespace-only-CharData"), true);
    reader.setFeature(QStringLiteral("http://xml.org/sax/features/namespaces"), false);
    QXmlInputSource source(device);

    QString errorMsg;
    int errorLine;//+errorColumn;
    bool success = m_doc.setContent(&source, &reader, &errorMsg, &errorLine/*,errorColumn*/);

    if (!success) {
        qCWarning(LOKALIZE_LOG) << "parse error" << errorMsg << errorLine;
        return errorLine + 1;
    }


    QDomElement file = m_doc.elementsByTagName(QStringLiteral("TS")).at(0).toElement();
    m_sourceLangCode = file.attribute(QStringLiteral("sourcelanguage"));
    m_targetLangCode = file.attribute(QStringLiteral("language"));
    m_numberOfPluralForms = numberOfPluralFormsForLangCode(m_targetLangCode);

    //Create entry mapping.
    //Along the way: for langs with more than 2 forms
    //we create any form-entries additionally needed

    entries = m_doc.elementsByTagName(QStringLiteral("message"));
    if (!Settings::self()->convertXMLChars()) {
        for (int pos = 0; pos < entries.size(); pos++) {
            QDomElement refNode = entries.at(pos).firstChildElement(names[SourceTag]).toElement();
            QDomNode firstRef = refNode.firstChild();
            refNode.appendChild(m_doc.createCDATASection(refNode.text()));
            refNode.removeChild(firstRef);
            QDomElement targetEl = unitForPos(pos).firstChildElement(names[TargetTag]).toElement();
            if (!targetEl.isNull()) {
                if (unitForPos(pos).hasAttribute(names[PluralTag])) {
                    QDomNodeList forms = targetEl.elementsByTagName(QStringLiteral("numerusform"));
                    for (int f = 0; f < forms.size(); f++) {
                        QDomNode firstTarget = forms.at(f).firstChild();
                        forms.at(f).appendChild(m_doc.createCDATASection(forms.at(f).toElement().text()));
                        forms.at(f).removeChild(firstTarget);
                    }
                } else {
                    QDomNode firstTarget = targetEl.firstChild();
                    targetEl.appendChild(m_doc.createCDATASection(targetEl.text()));
                    targetEl.removeChild(firstTarget);
                }
            }

            QDomElement noteEl = unitForPos(pos).firstChildElement(names[NoteTag]).toElement();
            if (!noteEl.isNull()) {
                QDomNode firstNote = noteEl.firstChild();
                noteEl.appendChild(m_doc.createCDATASection(noteEl.text()));
                noteEl.removeChild(firstNote);
            }
        }
    }

    qCWarning(LOKALIZE_LOG) << chrono.elapsed() << "secs, " << entries.size() << "entries";
    return 0;
}

bool TsStorage::save(QIODevice* device, bool belongsToProject)
{
    Q_UNUSED(belongsToProject)
    QTextStream stream(device);
    if (Settings::self()->convertXMLChars()) {
        m_doc.save(stream, 4);
    } else {
        QString tempString;
        QTextStream tempStream(&tempString);
        entries = m_doc.elementsByTagName(QStringLiteral("message"));
        if (!Settings::self()->convertXMLChars()) {
            for (int pos = 0; pos < entries.size(); pos++) {
                QDomElement refNode = entries.at(pos).firstChildElement(names[SourceTag]).toElement();
                QDomCDATASection firstRef = refNode.firstChild().toCDATASection();
                firstRef.setData(protect(firstRef.data()));
                QDomElement targetEl = unitForPos(pos).firstChildElement(names[TargetTag]).toElement();
                if (!targetEl.isNull()) {
                    if (unitForPos(pos).hasAttribute(names[PluralTag])) {
                        QDomNodeList forms = targetEl.elementsByTagName(QStringLiteral("numerusform"));
                        for (int f = 0; f < forms.size(); f++) {
                            QDomCDATASection firstTarget = forms.at(f).firstChild().toCDATASection();
                            firstTarget.setData(protect(firstTarget.data()));
                        }
                    } else {
                        QDomCDATASection firstTarget = targetEl.firstChild().toCDATASection();
                        firstTarget.setData(protect(firstTarget.data()));
                    }
                }

                QDomElement noteEl = unitForPos(pos).firstChildElement(names[NoteTag]).toElement();
                if (!noteEl.isNull()) {
                    QDomCDATASection firstNote = noteEl.firstChild().toCDATASection();
                    firstNote.setData(protect(firstNote.data()));
                }
            }
        }
        m_doc.save(tempStream, 4);
        if (!Settings::self()->convertXMLChars()) {
            for (int pos = 0; pos < entries.size(); pos++) {
                QDomElement refNode = entries.at(pos).firstChildElement(names[SourceTag]).toElement();
                QDomCDATASection firstRef = refNode.firstChild().toCDATASection();
                firstRef.setData(unprotect(firstRef.data()));
                QDomElement targetEl = unitForPos(pos).firstChildElement(names[TargetTag]).toElement();
                if (!targetEl.isNull()) {
                    if (unitForPos(pos).hasAttribute(names[PluralTag])) {
                        QDomNodeList forms = targetEl.elementsByTagName(QStringLiteral("numerusform"));
                        for (int f = 0; f < forms.size(); f++) {
                            QDomCDATASection firstTarget = forms.at(f).firstChild().toCDATASection();
                            firstTarget.setData(unprotect(firstTarget.data()));
                        }
                    } else {
                        QDomCDATASection firstTarget = targetEl.firstChild().toCDATASection();
                        firstTarget.setData(unprotect(firstTarget.data()));
                    }
                }

                QDomElement noteEl = unitForPos(pos).firstChildElement(names[NoteTag]).toElement();
                if (!noteEl.isNull()) {
                    QDomCDATASection firstNote = noteEl.firstChild().toCDATASection();
                    firstNote.setData(unprotect(firstNote.data()));
                }
            }
        }
        stream << tempString.replace(QStringLiteral("<source><![CDATA["), QStringLiteral("<source>"))
               .replace(QStringLiteral("<translation><![CDATA["), QStringLiteral("<translation>"))
               .replace(QStringLiteral("<numerusform><![CDATA["), QStringLiteral("<numerusform>"))
               .replace(QStringLiteral("<translation type=\"vanished\"><![CDATA["), QStringLiteral("<translation type=\"vanished\">"))
               .replace(QStringLiteral("<translation type=\"unfinished\"><![CDATA["), QStringLiteral("<translation type=\"unfinished\">"))
               .replace(QStringLiteral("<translatorcomment><![CDATA["), QStringLiteral("<translatorcomment>"))
               .replace(QStringLiteral("]]></source>"), QStringLiteral("</source>"))
               .replace(QStringLiteral("]]></translation>"), QStringLiteral("</translation>"))
               .replace(QStringLiteral("]]></numerusform>"), QStringLiteral("</numerusform>"))
               .replace(QStringLiteral("]]></translatorcomment>"), QStringLiteral("</translatorcomment>"));
    }
    return true;
}
//END OPEN/SAVE

//BEGIN STORAGE TRANSLATION

int TsStorage::size() const
{
    return entries.size();
}

/**
 * helper structure used during XLIFF XML walk-through
 */
struct TsContentEditingData {
    enum ActionType {Get, DeleteText, InsertText, CheckLength};

    QString stringToInsert;
    int pos;
    int lengthOfStringToRemove;
    ActionType actionType;

    ///Get
    TsContentEditingData(ActionType type = Get)
        : pos(-1)
        , lengthOfStringToRemove(-1)
        , actionType(type)
    {}

    ///DeleteText
    TsContentEditingData(int p, int l)
        : pos(p)
        , lengthOfStringToRemove(l)
        , actionType(DeleteText)
    {}

    ///InsertText
    TsContentEditingData(int p, const QString& s)
        : stringToInsert(s)
        , pos(p)
        , lengthOfStringToRemove(-1)
        , actionType(InsertText)
    {}
};

static QString doContent(QDomElement elem, int startingPos, TsContentEditingData* data);

/**
 * walks through XLIFF XML and performs actions depending on TsContentEditingData:
 * - reads content
 * - deletes content, or
 * - inserts content
 */
static QString content(QDomElement elem, TsContentEditingData* data = nullptr)
{
    return doContent(elem, 0, data);
}

static QString doContent(QDomElement elem, int startingPos, TsContentEditingData* data)
{
    //actually startingPos is current pos

    QString result;

    if (elem.isNull()
        || (!result.isEmpty() && data && data->actionType == TsContentEditingData::CheckLength))
        return QString();

    bool seenCDataAfterElement = false;

    QDomNode n = elem.firstChild();
    while (!n.isNull()) {
        if (Settings::self()->convertXMLChars() ? n.isCharacterData() : n.isCDATASection()) {
            seenCDataAfterElement = true;

            QDomCharacterData c = Settings::self()->convertXMLChars() ? n.toCharacterData() : n.toCDATASection();
            QString cData = c.data();
            if (data && data->pos != -1 &&
                data->pos >= startingPos && data->pos <= startingPos + cData.size()) {
                // time to do some action! ;)
                int localStartPos = data->pos - startingPos;

                //BEGIN DELETE TEXT
                if (data->actionType == TsContentEditingData::DeleteText) { //(data->lengthOfStringToRemove!=-1)
                    if (localStartPos + data->lengthOfStringToRemove > cData.size()) {
                        //text is fragmented into several QDomCData
                        int localDelLen = cData.size() - localStartPos;
                        c.deleteData(localStartPos, localDelLen);
                        //setup data for future iterations
                        data->lengthOfStringToRemove = data->lengthOfStringToRemove - localDelLen;
                        //data->pos=startingPos;
                        //qCWarning(LOKALIZE_LOG)<<"\tsetup:"<<data->pos<<data->lengthOfStringToRemove;
                    } else {
                        //qCWarning(LOKALIZE_LOG)<<"simple delete"<<localStartPos<<data->lengthOfStringToRemove;
                        c.deleteData(localStartPos, data->lengthOfStringToRemove);
                        data->actionType = TsContentEditingData::CheckLength;
                        return QString('a');//so it exits 100%
                    }
                }
                //END DELETE TEXT
                //INSERT
                else if (data->actionType == TsContentEditingData::InsertText) {
                    c.insertData(localStartPos, data->stringToInsert);
                    data->actionType = TsContentEditingData::CheckLength;
                    return QString('a');//so it exits 100%
                }
                cData = c.data();
            }

            result += cData;
            startingPos += cData.size();
        }
        n = n.nextSibling();
    }
    if (!seenCDataAfterElement) {
        //add empty cDATA child so that user could add some text
        elem.appendChild(Settings::self()->convertXMLChars() ?
                         elem.ownerDocument().createTextNode(QString()) :
                         elem.ownerDocument().createCDATASection(QString())
                        );
    }

    return result;
}



//flat-model interface (ignores XLIFF grouping)

CatalogString TsStorage::catalogString(QDomElement contentElement) const
{
    CatalogString catalogString;
    TsContentEditingData data(TsContentEditingData::Get);
    catalogString.string = content(contentElement, &data);
    return catalogString;
}

CatalogString TsStorage::catalogString(const DocPosition& pos) const
{
    return catalogString(pos.part == DocPosition::Target ? targetForPos(pos) : sourceForPos(pos.entry));
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

QString TsStorage::sourceWithPlurals(const DocPosition& pos, bool truncateFirstLine) const
{
    QString str = source(pos);
    if (truncateFirstLine) {
        int truncatePos = str.indexOf("\n");
        if (truncatePos != -1)
            str.truncate(truncatePos);
    }
    return str;
}
QString TsStorage::targetWithPlurals(const DocPosition& pos, bool truncateFirstLine) const
{
    QString str = target(pos);
    if (truncateFirstLine) {
        int truncatePos = str.indexOf("\n");
        if (truncatePos != -1)
            str.truncate(truncatePos);
    }
    return str;
}


void TsStorage::targetDelete(const DocPosition& pos, int count)
{
    TsContentEditingData data(pos.offset, count);
    content(targetForPos(pos), &data);
}

void TsStorage::targetInsert(const DocPosition& pos, const QString& arg)
{
    QDomElement targetEl = targetForPos(pos);
    //BEGIN add <*target>
    if (targetEl.isNull()) {
        QDomNode unitEl = unitForPos(pos.entry);
        QDomNode refNode = unitEl.firstChildElement(names[SourceTag]);
        targetEl = unitEl.insertAfter(m_doc.createElement(names[TargetTag]), refNode).toElement();

        if (pos.entry < size()) {
            targetEl.appendChild(Settings::self()->convertXMLChars() ?
                                 m_doc.createTextNode(arg) :
                                 m_doc.createCDATASection(arg));//i bet that pos.offset is 0 ;)
            return;
        }
    }
    //END add <*target>
    if (arg.isEmpty()) return; //means we were called just to add <target> tag

    TsContentEditingData data(pos.offset, arg);
    content(targetEl, &data);
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

    QString oldsource = content(unitForPos(pos.entry).firstChildElement(names[OldSourceTag]));
    if (!oldsource.isEmpty())
        result << AltTrans(CatalogString(oldsource), i18n("Previous source value, saved by lupdate tool"));

    return result;
}


QStringList TsStorage::sourceFiles(const DocPosition& pos) const
{
    QStringList result;

    QDomElement elem = unitForPos(pos.entry).firstChildElement(attrnames[LocationAttr]);
    while (!elem.isNull()) {
        QString sourcefile = elem.attribute(QStringLiteral("filename"));
        QString linenumber = elem.attribute(QStringLiteral("line"));
        if (!(sourcefile.isEmpty() && linenumber.isEmpty()))
            result.append(sourcefile + ':' + linenumber);

        elem = elem.nextSiblingElement(attrnames[LocationAttr]);
    }
    //qSort(result);

    return result;
}

QVector<Note> TsStorage::notes(const DocPosition& pos) const
{
    QVector<Note> result;

    QDomElement elem = unitForPos(pos.entry).firstChildElement(names[NoteTag]);
    while (!elem.isNull()) {
        QDomNode n = elem.firstChild();
        if (!n.isNull()) {
            QDomCharacterData c = Settings::self()->convertXMLChars() ? n.toCharacterData() : n.toCDATASection();
            Note note;
            note.content = c.data();
            result.append(note);
        }

        elem = elem.nextSiblingElement(names[NoteTag]);
    }
    return result;
}

QVector<Note> TsStorage::developerNotes(const DocPosition& pos) const
{
    QVector<Note> result;

    QDomElement elem = unitForPos(pos.entry).firstChildElement(names[DevNoteTag]);
    while (!elem.isNull()) {
        Note note;
        note.content = elem.text();
        result.append(note);

        elem = elem.nextSiblingElement(names[DevNoteTag]);
    }
    return result;
}

Note TsStorage::setNote(DocPosition pos, const Note& note)
{
    QDomElement unit = unitForPos(pos.entry);
    QDomElement elem;
    Note oldNote;
    if (pos.form == -1 && !note.content.isEmpty()) {
        QDomElement ref = unit.lastChildElement(names[NoteTag]);
        elem = unit.insertAfter(m_doc.createElement(names[NoteTag]), ref).toElement();
        elem.appendChild(Settings::self()->convertXMLChars() ?
                         m_doc.createTextNode(QString()) :
                         m_doc.createCDATASection(QString()));
    } else {
        QDomNodeList list = unit.elementsByTagName(names[NoteTag]);
        //if (pos.form==-1) pos.form=list.size()-1;
        if (pos.form < list.size()) {
            elem = unit.elementsByTagName(names[NoteTag]).at(pos.form).toElement();
            QDomNode n = elem.firstChild();
            if (!n.isNull()) {
                QDomCharacterData c = Settings::self()->convertXMLChars() ? n.toCharacterData() : n.toCDATASection();

                oldNote.content = c.data();
            }
        }
    }

    if (elem.isNull()) return oldNote;

    QDomNode n = elem.firstChild();
    if (!n.isNull()) {
        QDomCharacterData c = Settings::self()->convertXMLChars() ? n.toCharacterData() : n.toCDATASection();
        if (!c.data().isEmpty()) {
            TsContentEditingData data(0, c.data().size());
            content(elem, &data);
        }
    }

    if (!note.content.isEmpty()) {
        TsContentEditingData data(0, note.content);
        content(elem, &data);
    } else
        unit.removeChild(elem);

    return oldNote;
}

QStringList TsStorage::context(const DocPosition& pos) const
{
    QStringList result;

    QDomElement unit = unitForPos(pos.entry);
    QDomElement context = unit.parentNode().toElement();
    //if (context.isNull())
    //    return result;

    QDomElement name = context.firstChildElement(names[NameTag]);
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
    QString result = source(pos);
    result.remove('\n');
    QStringList ctxt = context(pos);
    if (ctxt.size())
        result.prepend(ctxt.first());
    return result;
}

bool TsStorage::isPlural(const DocPosition& pos) const
{
    QDomElement unit = unitForPos(pos.entry);

    return unit.hasAttribute(names[PluralTag]);
}

void TsStorage::setApproved(const DocPosition& pos, bool approved)
{
    targetInsert(pos, QString()); //adds <target> if needed
    QDomElement target = unitForPos(pos.entry).firstChildElement(names[TargetTag]); //asking directly to bypass plural state detection
    if (target.attribute(attrnames[TypeAttr]) == attrvalues[ObsoleteVal])
        return;
    if (approved)
        target.removeAttribute(attrnames[TypeAttr]);
    else
        target.setAttribute(attrnames[TypeAttr], QStringLiteral("unfinished"));
}

bool TsStorage::isApproved(const DocPosition& pos) const
{
    QDomElement target = unitForPos(pos.entry).firstChildElement(names[TargetTag]);
    return !target.hasAttribute(attrnames[TypeAttr]) || target.attribute(attrnames[TypeAttr]) == attrvalues[VanishedVal];
}

bool TsStorage::isObsolete(int entry) const
{
    QDomElement target = unitForPos(entry).firstChildElement(names[TargetTag]);
    QString v = target.attribute(attrnames[TypeAttr]);
    return v == attrvalues[ObsoleteVal] || v == attrvalues[VanishedVal];
}

bool TsStorage::isEmpty(const DocPosition& pos) const
{
    TsContentEditingData data(TsContentEditingData::CheckLength);
    return content(targetForPos(pos), &data).isEmpty();
}

bool TsStorage::isEquivTrans(const DocPosition& pos) const
{
    Q_UNUSED(pos)
    return true;//targetForPos(pos.entry).attribute("equiv-trans")!="no";
}

void TsStorage::setEquivTrans(const DocPosition& pos, bool equivTrans)
{
    Q_UNUSED(pos)
    Q_UNUSED(equivTrans)
    //targetForPos(pos.entry).setAttribute("equiv-trans",noyes[equivTrans]);
}

QDomElement TsStorage::unitForPos(int pos) const
{
    return entries.at(pos).toElement();
}

QDomElement TsStorage::targetForPos(DocPosition pos) const
{
    QDomElement unit = unitForPos(pos.entry);
    QDomElement translation = unit.firstChildElement(names[TargetTag]);
    if (!unit.hasAttribute(names[PluralTag]))
        return translation;

    if (pos.form == -1) pos.form = 0;

    QDomNodeList forms = translation.elementsByTagName(QStringLiteral("numerusform"));
    while (pos.form >= forms.size())
        translation.appendChild(unit.ownerDocument().createElement(QStringLiteral("numerusform")));
    return forms.at(pos.form).toElement();
}

QDomElement TsStorage::sourceForPos(int pos) const
{
    return unitForPos(pos).firstChildElement(names[SourceTag]);
}

void TsStorage::setTargetLangCode(const QString& langCode)
{
    m_targetLangCode = langCode;

    QDomElement file = m_doc.elementsByTagName(QStringLiteral("TS")).at(0).toElement();
    if (m_targetLangCode != file.attribute(QStringLiteral("language")).replace('-', '_')) {
        QString l = langCode;
        file.setAttribute(QStringLiteral("language"), l.replace('_', '-'));
    }
}


//END STORAGE TRANSLATION


