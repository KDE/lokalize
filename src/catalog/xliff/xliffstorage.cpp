/*
  SPDX-FileCopyrightText: 2008-2009 Nick Shaforostoff <shaforostoff@kde.ru>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "xliffstorage.h"

#include "lokalize_debug.h"

#include "gettextheader.h"
#include "prefs_lokalize.h"
#include "project.h"
#include "version.h"

#include <QElapsedTimer>
#include <QList>
#include <QPair>
#include <QString>
#include <QStringBuilder>
#include <QXmlStreamReader>

static const QString noyes[] = {QStringLiteral("no"), QStringLiteral("yes")};
static const QString bintargettarget[] = {QStringLiteral("bin-target"), QStringLiteral("target")};
static const QString binsourcesource[] = {QStringLiteral("bin-source"), QStringLiteral("source")};
static const QString NOTE = QStringLiteral("note");

int XliffStorage::capabilities() const
{
    return KeepsNoteAuthors | MultipleNotes | Phases | ExtendedStates | Tags;
}

// BEGIN OPEN/SAVE

int XliffStorage::load(QIODevice *device)
{
    QElapsedTimer chrono;
    chrono.start();

    QXmlStreamReader reader(device);
    reader.setNamespaceProcessing(false);

    QString errorMsg;
    int errorLine{}; //+errorColumn;
    bool success = m_doc.setContent(&reader, false, &errorMsg, &errorLine /*,errorColumn*/);

    QString FILE = QStringLiteral("file");
    if (!success || m_doc.elementsByTagName(FILE).isEmpty()) {
        qCWarning(LOKALIZE_LOG) << errorMsg;
        return errorLine + 1;
    }

    QDomElement file = m_doc.elementsByTagName(FILE).at(0).toElement();
    m_sourceLangCode = file.attribute(QStringLiteral("source-language")).replace(u'-', u'_');
    m_targetLangCode = file.attribute(QStringLiteral("target-language")).replace(u'-', u'_');
    m_numberOfPluralForms = numberOfPluralFormsForLangCode(m_targetLangCode);

    // Create entry mapping.
    // Along the way: for langs with more than 2 forms
    // we create any form-entries additionally needed

    entries = m_doc.elementsByTagName(QStringLiteral("trans-unit"));
    int nodeListSize = entries.size();
    m_map.clear();
    m_map.reserve(nodeListSize);
    for (int i = 0; i < nodeListSize; ++i) {
        QDomElement parentElement = entries.at(i).parentNode().toElement();
        // if (Q_UNLIKELY( e.isNull() ))//sanity
        //       continue;
        m_map << i;
        m_unitsById[entries.at(i).toElement().attribute(QStringLiteral("id"))] = i;

        if (parentElement.tagName() == QLatin1String("group") && parentElement.attribute(QStringLiteral("restype")) == QLatin1String("x-gettext-plurals")) {
            m_plurals.insert(i);
            int localPluralNum = m_numberOfPluralForms;
            while (--localPluralNum > 0 && (++i) < nodeListSize) {
                QDomElement p = entries.at(i).parentNode().toElement();
                if (p.tagName() == QLatin1String("group") && p.attribute(QStringLiteral("restype")) == QLatin1String("x-gettext-plurals"))
                    continue;

                parentElement.appendChild(entries.at(m_map.last()).cloneNode());
            }
        }
    }

    binEntries = m_doc.elementsByTagName(QStringLiteral("bin-unit"));
    nodeListSize = binEntries.size();
    int offset = m_map.size();
    for (int i = 0; i < nodeListSize; ++i)
        m_unitsById[binEntries.at(i).toElement().attribute(QStringLiteral("id"))] = offset + i;

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

    QDomElement header = file.firstChildElement(QStringLiteral("header"));
    if (header.isNull())
        header = file.insertBefore(m_doc.createElement(QStringLiteral("header")), QDomElement()).toElement();
    QDomElement toolElem = header.firstChildElement(QStringLiteral("tool"));
    while (!toolElem.isNull() && toolElem.attribute(QStringLiteral("tool-id")) != QLatin1String("lokalize-" LOKALIZE_VERSION))
        toolElem = toolElem.nextSiblingElement(QStringLiteral("tool"));

    if (toolElem.isNull()) {
        toolElem = header.appendChild(m_doc.createElement(QStringLiteral("tool"))).toElement();
        toolElem.setAttribute(QStringLiteral("tool-id"), QStringLiteral("lokalize-" LOKALIZE_VERSION));
        toolElem.setAttribute(QStringLiteral("tool-name"), QStringLiteral("Lokalize"));
        toolElem.setAttribute(QStringLiteral("tool-version"), QStringLiteral(LOKALIZE_VERSION));
    }

    qCWarning(LOKALIZE_LOG) << chrono.elapsed();
    return 0;
}

bool XliffStorage::save(QIODevice *device, bool belongsToProject)
{
    Q_UNUSED(belongsToProject)
    QTextStream stream(device);
    m_doc.save(stream, 2);
    return true;
}
// END OPEN/SAVE

// BEGIN STORAGE TRANSLATION

int XliffStorage::size() const
{
    return m_map.size();
}

void XliffStorage::setTargetLangCode(const QString &langCode)
{
    m_targetLangCode = langCode;

    QDomElement file = m_doc.elementsByTagName(QStringLiteral("file")).at(0).toElement();
    if (m_targetLangCode != file.attribute(QStringLiteral("target-language")).replace(u'-', u'_')) {
        QString l = langCode;
        file.setAttribute(QStringLiteral("target-language"), l.replace(u'_', u'-'));
    }
}

/**
 * helper structure used during XLIFF XML walk-through
 */
struct ContentEditingData {
    enum ActionType {
        Get,
        DeleteText,
        InsertText,
        DeleteTag,
        InsertTag,
        CheckLength,
    };

    QList<InlineTag> tags;
    QString stringToInsert;
    int pos{-1};
    int lengthOfStringToRemove{-1};
    ActionType actionType;

    /// Get
    explicit ContentEditingData(ActionType type = Get)
        : actionType(type)
    {
    }

    /// DeleteText
    ContentEditingData(int p, int l)
        : pos(p)
        , lengthOfStringToRemove(l)
        , actionType(DeleteText)
    {
    }

    /// InsertText
    ContentEditingData(int p, const QString &s)
        : stringToInsert(s)
        , pos(p)
        , lengthOfStringToRemove(-1)
        , actionType(InsertText)
    {
    }

    /// InsertTag
    ContentEditingData(int p, const InlineTag &range)
        : pos(p)
        , lengthOfStringToRemove(-1)
        , actionType(InsertTag)
    {
        tags.append(range);
    }

    /// DeleteTag
    explicit ContentEditingData(int p)
        : pos(p)
        , actionType(DeleteTag)
    {
    }
};

static QString doContent(QDomElement elem, int startingPos, ContentEditingData *data);

/**
 * walks through XLIFF XML and performs actions depending on ContentEditingData:
 * - reads content
 * - deletes content, or
 * - inserts content
 */
static QString content(QDomElement elem, ContentEditingData *data = nullptr)
{
    return doContent(elem, 0, data);
}

static QString doContent(QDomElement elem, int startingPos, ContentEditingData *data)
{
    // actually startingPos is current pos

    QString result;

    if (elem.isNull())
        return QString();

    bool seenCharacterDataAfterElement = false;

    QDomNode n = elem.firstChild();
    while (!n.isNull()) {
        if (n.isCharacterData()) {
            seenCharacterDataAfterElement = true;

            QDomCharacterData c = n.toCharacterData();
            QString cData = c.data();

            if (data && data->pos != -1 && data->pos >= startingPos && data->pos <= startingPos + cData.size()) {
                // time to do some action! ;)
                int localStartPos = data->pos - startingPos;

                // BEGIN DELETE TEXT
                if (data->actionType == ContentEditingData::DeleteText) { //(data->lengthOfStringToRemove!=-1)
                    if (localStartPos + data->lengthOfStringToRemove > cData.size()) {
                        // text is fragmented into several QDomCharacterData
                        int localDelLen = cData.size() - localStartPos;
                        // qCWarning(LOKALIZE_LOG)<<"text is fragmented into several QDomCharacterData. localDelLen:"<<localDelLen<<"cData:"<<cData;
                        c.deleteData(localStartPos, localDelLen);
                        // setup data for future iterations
                        data->lengthOfStringToRemove = data->lengthOfStringToRemove - localDelLen;
                        // data->pos=startingPos;
                        // qCWarning(LOKALIZE_LOG)<<"\tsetup:"<<data->pos<<data->lengthOfStringToRemove;
                    } else {
                        // qCWarning(LOKALIZE_LOG)<<"simple delete"<<localStartPos<<data->lengthOfStringToRemove;
                        c.deleteData(localStartPos, data->lengthOfStringToRemove);
                        data->actionType = ContentEditingData::CheckLength;
                        return QStringLiteral("a"); // so it exits 100%
                    }
                }
                // END DELETE TEXT
                // INSERT
                else if (data->actionType == ContentEditingData::InsertText) {
                    c.insertData(localStartPos, data->stringToInsert);
                    data->actionType = ContentEditingData::CheckLength;
                    return QStringLiteral("a"); // so it exits 100%
                }
                // BEGIN INSERT TAG
                else if (data->actionType == ContentEditingData::InsertTag) {
                    const InlineTag &tag = data->tags.first();
                    QString mid = cData.mid(localStartPos);
                    qCDebug(LOKALIZE_LOG) << "inserting tag" << tag.name() << tag.id << tag.start << tag.end << mid << data->pos << startingPos;
                    if (mid.size())
                        c.deleteData(localStartPos, mid.size());
                    QDomElement newNode = elem.insertAfter(elem.ownerDocument().createElement(QLatin1String(tag.getElementName())), n).toElement();
                    newNode.setAttribute(QStringLiteral("id"), tag.id);
                    if (!tag.xid.isEmpty())
                        newNode.setAttribute(QStringLiteral("xid"), tag.xid);

                    if (tag.isPaired() && tag.end > (tag.start + 1)) {
                        // qCWarning(LOKALIZE_LOG)<<"isPaired";
                        int len = tag.end - tag.start - 1; //-image symbol
                        int localLen = qMin(len, mid.size());
                        if (localLen) { // appending text
                            // qCWarning(LOKALIZE_LOG)<<"localLen. appending"<<localLen<<mid.left(localLen);
                            newNode.appendChild(elem.ownerDocument().createTextNode(mid.left(localLen)));
                            mid = mid.mid(localLen);
                        }
                        if (len - localLen) { // need to eat more (strings or elements) into newNode
                            int missingLen = len - localLen;
                            // qCWarning(LOKALIZE_LOG)<<"len-localLen";
                            // iterate over siblings until we get childrenCumulativeLen>missingLen (or siblings end)
                            int childrenCumulativeLen = 0;
                            QDomNode sibling = newNode.nextSibling();
                            while (!sibling.isNull()) { //&&(childrenCumulativeLen<missingLen))
                                QDomNode tmp = sibling;
                                sibling = sibling.nextSibling();
                                if (tmp.isAttr())
                                    continue;
                                ContentEditingData subData(ContentEditingData::Get);
                                if (tmp.isElement()) {
                                    childrenCumulativeLen++;
                                    childrenCumulativeLen += InlineTag::isPaired(InlineTag::getElementType(tmp.toElement().tagName().toUtf8()));
                                    qCWarning(LOKALIZE_LOG) << "calling sub";
                                    QString subContent = doContent(tmp.toElement(), /*we don't care about position*/ 0, &subData);
                                    qCWarning(LOKALIZE_LOG) << "called sub";
                                    childrenCumulativeLen += subContent.size();
                                } else if (tmp.isCharacterData())
                                    childrenCumulativeLen += tmp.toCharacterData().data().size();
                                // qCWarning(LOKALIZE_LOG)<<"brbr"<<tmp.nodeName()<<tmp.nodeValue()
                                //<<childrenCumulativeLen<<missingLen;

                                if (childrenCumulativeLen > missingLen) {
                                    if (tmp.isCharacterData()) {
                                        // divide the last string
                                        const QString &endData = tmp.toCharacterData().data();
                                        QString last = endData.left(endData.size() - (childrenCumulativeLen - missingLen));
                                        newNode.appendChild(elem.ownerDocument().createTextNode(last));
                                        tmp.toCharacterData().deleteData(0, last.size());
                                        // qCWarning(LOKALIZE_LOG)<<"end of add"<<last;
                                    }
                                    break;
                                }
                                newNode.appendChild(tmp);
                            }
                        }
                        if (!newNode.lastChild().isCharacterData())
                            newNode.appendChild(elem.ownerDocument().createTextNode(QString()));
                    }
                    if (!mid.isEmpty())
                        elem.insertAfter(elem.ownerDocument().createTextNode(mid), newNode);
                    else if (!newNode.nextSibling().isCharacterData()) // keep our DOM in a nice state
                        elem.insertAfter(elem.ownerDocument().createTextNode(QString()), newNode);

                    data->actionType = ContentEditingData::CheckLength;
                    return QStringLiteral("a"); // we're done here
                }
                // END INSERT TAG
                cData = c.data();
            }
            // else
            //     if (data&&data->pos!=-1/*&& n.nextSibling().isNull()*/)
            //         qCWarning(LOKALIZE_LOG)<<"arg!"<<startingPos<<"data->pos"<<data->pos;

            result += cData;
            startingPos += cData.size();
        } else if (n.isElement()) {
            QDomElement el = n.toElement();
            // BEGIN DELETE TAG
            if (data && data->actionType == ContentEditingData::DeleteTag && data->pos == startingPos) {
                // qCWarning(LOKALIZE_LOG)<<"start deleting tag";
                data->tags.append(InlineTag(startingPos,
                                            -1,
                                            InlineTag::getElementType(el.tagName().toUtf8()),
                                            el.attribute(QStringLiteral("id")),
                                            el.attribute(QStringLiteral("xid"))));
                if (data->tags.first().isPaired()) {
                    // get end position
                    ContentEditingData subData(ContentEditingData::Get);
                    QString subContent = doContent(el, startingPos, &subData);
                    data->tags[0].end = 1 + startingPos + subContent.size(); // tagsymbol+text
                    // qCWarning(LOKALIZE_LOG)<<"get end position"<<startingPos<<subContent.size();

                    // move children upper
                    QDomNode local = n.firstChild();
                    QDomNode refNode = n;
                    while (!local.isNull()) {
                        QDomNode tmp = local;
                        local = local.nextSibling();
                        if (!tmp.isAttr()) {
                            // qCWarning(LOKALIZE_LOG)<<"here is another child"<<tmp.nodeType()<<tmp.nodeName()<<tmp.nodeValue();
                            refNode = elem.insertAfter(tmp, refNode);
                        }
                    }
                }
                QDomNode temp = n;
                n = n.nextSibling();
                elem.removeChild(temp);
                data->actionType = ContentEditingData::CheckLength;
                return QStringLiteral("a"); // we're done here
            }
            // END DELETE TAG

            if (!seenCharacterDataAfterElement) // add empty charData child so that user could add some text
                elem.insertBefore(elem.ownerDocument().createTextNode(QString()), n);
            seenCharacterDataAfterElement = false;

            if (data) {
                result += QChar(TAGRANGE_IMAGE_SYMBOL);
                ++startingPos;
            }
            int oldStartingPos = startingPos;

            // detect type of the tag
            InlineTag::InlineElement i = InlineTag::getElementType(el.tagName().toUtf8());

            // 1 or 2 images to represent it?
            // 2 = there may be content inside
            if (InlineTag::isPaired(i)) {
                QString recursiveContent = doContent(el, startingPos, data);
                if (!recursiveContent.isEmpty()) {
                    result += recursiveContent;
                    startingPos += recursiveContent.size();
                }
                if (data) {
                    result += QChar(TAGRANGE_IMAGE_SYMBOL);
                    ++startingPos;
                }
            }

            if (data && data->actionType == ContentEditingData::Get) {
                QString id = el.attribute(QStringLiteral("id"));
                if (i == InlineTag::mrk) // TODO attr map
                    id = el.attribute(QStringLiteral("mtype"));

                // qCWarning(LOKALIZE_LOG)<<"tagName"<<el.tagName()<<"id"<<id<<"start"<<oldStartingPos-1<<startingPos-1;
                data->tags.append(InlineTag(oldStartingPos - 1, startingPos - 1, i, id, el.attribute(QStringLiteral("xid"))));
            }
        }
        n = n.nextSibling();
    }
    if (!seenCharacterDataAfterElement) {
        // add empty charData child so that user could add some text
        elem.appendChild(elem.ownerDocument().createTextNode(QString()));
    }

    return result;
}

// flat-model interface (ignores XLIFF grouping)

CatalogString XliffStorage::catalogString(QDomElement unit, DocPosition::Part part) const
{
    static const QString names[] = {QStringLiteral("source"), QStringLiteral("target"), QStringLiteral("seg-source")};
    CatalogString catalogString;
    ContentEditingData data(ContentEditingData::Get);
    int nameIndex = part == DocPosition::Target;
    if (nameIndex == 0 && !unit.firstChildElement(names[2]).isNull())
        nameIndex = 2;
    catalogString.string = content(unit.firstChildElement(names[nameIndex]), &data);
    catalogString.tags = data.tags;
    return catalogString;
}

CatalogString XliffStorage::catalogString(const DocPosition &pos) const
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
    return nonbin ? content(elem) : elem.firstChildElement(QStringLiteral("external-file")).attribute(QStringLiteral("href"));
}
QString XliffStorage::source(const DocPosition &pos) const
{
    return genericContent(sourceForPos(pos.entry), pos.entry < size());
}
QString XliffStorage::target(const DocPosition &pos) const
{
    return genericContent(targetForPos(pos.entry), pos.entry < size());
}
QString XliffStorage::sourceWithPlurals(const DocPosition &pos, bool truncateFirstLine) const
{
    QString str = source(pos);
    if (truncateFirstLine) {
        int truncatePos = str.indexOf(QLatin1Char('\n'));
        if (truncatePos != -1)
            str.truncate(truncatePos);
    }
    return str;
}
QString XliffStorage::targetWithPlurals(const DocPosition &pos, bool truncateFirstLine) const
{
    QString str = target(pos);
    if (truncateFirstLine) {
        int truncatePos = str.indexOf(QLatin1Char('\n'));
        if (truncatePos != -1)
            str.truncate(truncatePos);
    }
    return str;
}

void XliffStorage::targetDelete(const DocPosition &pos, int count)
{
    if (pos.entry < size()) {
        ContentEditingData data(pos.offset, count);
        content(targetForPos(pos.entry), &data);
    } else {
        // only bulk delete requests are generated
        targetForPos(pos.entry).firstChildElement(QStringLiteral("external-file")).setAttribute(QStringLiteral("href"), QString());
    }
}

void XliffStorage::targetInsert(const DocPosition &pos, const QString &arg)
{
    // qCWarning(LOKALIZE_LOG)<<"targetinsert"<<pos.entry<<arg;
    QDomElement targetEl = targetForPos(pos.entry);
    // BEGIN add <*target>
    if (targetEl.isNull()) {
        QDomNode unitEl = unitForPos(pos.entry);
        QDomNode refNode = unitEl.firstChildElement(QStringLiteral("seg-source")); // obey standard
        if (refNode.isNull())
            refNode = unitEl.firstChildElement(binsourcesource[pos.entry < size()]);
        targetEl = unitEl.insertAfter(m_doc.createElement(bintargettarget[pos.entry < size()]), refNode).toElement();
        targetEl.setAttribute(QStringLiteral("state"), QStringLiteral("new"));

        if (pos.entry < size()) {
            targetEl.appendChild(m_doc.createTextNode(arg)); // i bet that pos.offset is 0 ;)
            return;
        }
    }
    // END add <*target>
    if (arg.isEmpty())
        return; // means we were called just to add <taget> tag

    if (pos.entry >= size()) {
        QDomElement ef = targetEl.firstChildElement(QStringLiteral("external-file"));
        if (ef.isNull())
            ef = targetEl.appendChild(m_doc.createElement(QStringLiteral("external-file"))).toElement();
        ef.setAttribute(QStringLiteral("href"), arg);
        return;
    }

    ContentEditingData data(pos.offset, arg);
    content(targetEl, &data);
}

void XliffStorage::targetInsertTag(const DocPosition &pos, const InlineTag &tag)
{
    targetInsert(pos, QString()); // adds <taget> if needed
    ContentEditingData data(tag.start, tag);
    content(targetForPos(pos.entry), &data);
}

InlineTag XliffStorage::targetDeleteTag(const DocPosition &pos)
{
    ContentEditingData data(pos.offset);
    content(targetForPos(pos.entry), &data);
    if (data.tags[0].end == -1)
        data.tags[0].end = data.tags[0].start;
    return data.tags.first();
}

void XliffStorage::setTarget(const DocPosition &pos, const QString &arg)
{
    Q_UNUSED(pos);
    Q_UNUSED(arg);
    // TODO
}

QVector<AltTrans> XliffStorage::altTrans(const DocPosition &pos) const
{
    QVector<AltTrans> result;

    QDomElement elem = unitForPos(pos.entry).firstChildElement(QStringLiteral("alt-trans"));
    while (!elem.isNull()) {
        AltTrans aTrans;
        aTrans.context = CatalogString();
        aTrans.source = catalogString(elem, DocPosition::Source);
        aTrans.target = catalogString(elem, DocPosition::Target);
        aTrans.phase = elem.attribute(QStringLiteral("phase-name"));
        aTrans.origin = elem.attribute(QStringLiteral("origin"));
        aTrans.score = elem.attribute(QStringLiteral("match-quality")).toInt();
        aTrans.lang = elem.firstChildElement(QStringLiteral("target")).attribute(QStringLiteral("xml:lang"));

        const char *const types[] = {"proposal", "previous-version", "rejected", "reference", "accepted"};
        QString typeStr = elem.attribute(QStringLiteral("alttranstype"));
        int i = -1;
        while (++i < int(sizeof(types) / sizeof(char *)) && QLatin1String(types[i]) != typeStr)
            ;
        aTrans.type = AltTrans::Type(i);

        result << aTrans;

        elem = elem.nextSiblingElement(QStringLiteral("alt-trans"));
    }
    return result;
}

static QDomElement phaseElement(QDomDocument m_doc, const QString &name, QDomElement &phasegroup)
{
    QDomElement file = m_doc.elementsByTagName(QStringLiteral("file")).at(0).toElement();
    QDomElement header = file.firstChildElement(QStringLiteral("header"));
    phasegroup = header.firstChildElement(QStringLiteral("phase-group"));
    if (phasegroup.isNull()) {
        phasegroup = m_doc.createElement(QStringLiteral("phase-group"));
        // order following XLIFF spec
        QDomElement skl = header.firstChildElement(QStringLiteral("skl"));
        if (!skl.isNull())
            header.insertAfter(phasegroup, skl);
        else
            header.insertBefore(phasegroup, header.firstChildElement());
    }
    QDomElement phaseElem = phasegroup.firstChildElement(QStringLiteral("phase"));
    while (!phaseElem.isNull() && phaseElem.attribute(QStringLiteral("phase-name")) != name)
        phaseElem = phaseElem.nextSiblingElement(QStringLiteral("phase"));

    return phaseElem;
}

static Phase phaseFromElement(QDomElement phaseElem)
{
    Phase phase;
    phase.name = phaseElem.attribute(QStringLiteral("phase-name"));
    phase.process = phaseElem.attribute(QStringLiteral("process-name"));
    phase.company = phaseElem.attribute(QStringLiteral("company-name"));
    phase.contact = phaseElem.attribute(QStringLiteral("contact-name"));
    phase.email = phaseElem.attribute(QStringLiteral("contact-email"));
    phase.phone = phaseElem.attribute(QStringLiteral("contact-phone"));
    phase.tool = phaseElem.attribute(QStringLiteral("tool-id"));
    phase.date = QDate::fromString(phaseElem.attribute(QStringLiteral("date")), Qt::ISODate);
    return phase;
}

Phase XliffStorage::updatePhase(const Phase &phase)
{
    QDomElement phasegroup;
    QDomElement phaseElem = phaseElement(m_doc, phase.name, phasegroup);
    Phase prev = phaseFromElement(phaseElem);

    if (phaseElem.isNull() && !phase.name.isEmpty()) {
        phaseElem = phasegroup.appendChild(m_doc.createElement(QStringLiteral("phase"))).toElement();
        phaseElem.setAttribute(QStringLiteral("phase-name"), phase.name);
    }

    phaseElem.setAttribute(QStringLiteral("process-name"), phase.process);
    if (!phase.company.isEmpty())
        phaseElem.setAttribute(QStringLiteral("company-name"), phase.company);
    phaseElem.setAttribute(QStringLiteral("contact-name"), phase.contact);
    phaseElem.setAttribute(QStringLiteral("contact-email"), phase.email);
    // Q_ASSERT(phase.contact.length()); //is empty when exiting w/o saving
    if (!phase.phone.isEmpty())
        phaseElem.setAttribute(QLatin1String("contact-phone"), phase.phone);
    phaseElem.setAttribute(QStringLiteral("tool-id"), phase.tool);
    if (phase.date.isValid())
        phaseElem.setAttribute(QStringLiteral("date"), phase.date.toString(Qt::ISODate));
    return prev;
}

QList<Phase> XliffStorage::allPhases() const
{
    QList<Phase> result;
    QDomElement file = m_doc.elementsByTagName(QStringLiteral("file")).at(0).toElement();
    QDomElement header = file.firstChildElement(QStringLiteral("header"));
    QDomElement phasegroup = header.firstChildElement(QStringLiteral("phase-group"));
    QDomElement phaseElem = phasegroup.firstChildElement(QStringLiteral("phase"));
    while (!phaseElem.isNull()) {
        result.append(phaseFromElement(phaseElem));
        phaseElem = phaseElem.nextSiblingElement(QStringLiteral("phase"));
    }
    return result;
}

Phase XliffStorage::phase(const QString &name) const
{
    QDomElement phasegroup;
    QDomElement phaseElem = phaseElement(m_doc, name, phasegroup);

    return phaseFromElement(phaseElem);
}

QMap<QString, Tool> XliffStorage::allTools() const
{
    QMap<QString, Tool> result;
    QDomElement file = m_doc.elementsByTagName(QStringLiteral("file")).at(0).toElement();
    QDomElement header = file.firstChildElement(QStringLiteral("header"));
    QDomElement toolElem = header.firstChildElement(QStringLiteral("tool"));
    while (!toolElem.isNull()) {
        Tool tool;
        tool.tool = toolElem.attribute(QStringLiteral("tool-id"));
        tool.name = toolElem.attribute(QStringLiteral("tool-name"));
        tool.version = toolElem.attribute(QStringLiteral("tool-version"));
        tool.company = toolElem.attribute(QStringLiteral("tool-company"));

        result.insert(tool.tool, tool);
        toolElem = toolElem.nextSiblingElement(QStringLiteral("tool"));
    }
    return result;
}

QStringList XliffStorage::sourceFiles(const DocPosition &pos) const
{
    QStringList result;

    QDomElement elem = unitForPos(pos.entry).firstChildElement(QStringLiteral("context-group"));
    while (!elem.isNull()) {
        if (elem.attribute(QStringLiteral("purpose")).contains(QLatin1String("location"))) {
            QDomElement contextElem = elem.firstChildElement(QStringLiteral("context"));
            while (!contextElem.isNull()) {
                QString sourcefile;
                QString linenumber;
                const QString contextType = contextElem.attribute(QStringLiteral("context-type"));
                if (contextType == QLatin1String("sourcefile"))
                    sourcefile = contextElem.text();
                else if (contextType == QLatin1String("linenumber"))
                    linenumber = contextElem.text();
                if (!(sourcefile.isEmpty() && linenumber.isEmpty()))
                    result.append(sourcefile + QLatin1Char(':') + linenumber);

                contextElem = contextElem.nextSiblingElement(QStringLiteral("context"));
            }
        }

        elem = elem.nextSiblingElement(QStringLiteral("context-group"));
    }
    // qSort(result);

    return result;
}

static void initNoteFromElement(Note &note, QDomElement elem)
{
    note.content = elem.text();
    note.from = elem.attribute(QStringLiteral("from"));
    note.lang = elem.attribute(QStringLiteral("xml:lang"));
    if (elem.attribute(QStringLiteral("annotates")) == QLatin1String("source"))
        note.annotates = Note::Source;
    else if (elem.attribute(QStringLiteral("annotates")) == QLatin1String("target"))
        note.annotates = Note::Target;
    bool ok;
    note.priority = elem.attribute(QStringLiteral("priority")).toInt(&ok);
    if (!ok)
        note.priority = 0;
}

QVector<Note> XliffStorage::notes(const DocPosition &pos) const
{
    QList<Note> result;

    QDomElement elem = entries.at(m_map.at(pos.entry)).firstChildElement(NOTE);
    while (!elem.isNull()) {
        Note note;
        initNoteFromElement(note, elem);
        result.append(note);
        elem = elem.nextSiblingElement(NOTE);
    }
    std::sort(result.begin(), result.end());
    return result.toVector();
}

QVector<Note> XliffStorage::developerNotes(const DocPosition &pos) const
{
    Q_UNUSED(pos);
    // TODO
    return QVector<Note>();
}

Note XliffStorage::setNote(DocPosition pos, const Note &note)
{
    // qCWarning(LOKALIZE_LOG)<<int(pos.form)<<note.content;
    QDomElement unit = unitForPos(pos.entry);
    QDomElement elem;
    Note oldNote;
    if (pos.form == -1 && !note.content.isEmpty()) {
        QDomElement ref = unit.lastChildElement(NOTE);
        elem = unit.insertAfter(m_doc.createElement(NOTE), ref).toElement();
        elem.appendChild(m_doc.createTextNode(QString()));
    } else {
        QDomNodeList list = unit.elementsByTagName(NOTE);
        // if (pos.form==-1) pos.form=list.size()-1;
        if (pos.form < list.size()) {
            elem = unit.elementsByTagName(NOTE).at(pos.form).toElement();
            initNoteFromElement(oldNote, elem);
        }
    }

    if (elem.isNull())
        return oldNote;

    if (!elem.text().isEmpty()) {
        ContentEditingData data(0, elem.text().size());
        content(elem, &data);
    }

    if (!note.content.isEmpty()) {
        ContentEditingData data(0, note.content);
        content(elem, &data);
        if (!note.from.isEmpty())
            elem.setAttribute(QStringLiteral("from"), note.from);
        if (note.priority)
            elem.setAttribute(QStringLiteral("priority"), note.priority);
    } else
        unit.removeChild(elem);

    return oldNote;
}

QStringList XliffStorage::noteAuthors() const
{
    QSet<QString> result;
    QDomNodeList notesNodes = m_doc.elementsByTagName(NOTE);
    int i = notesNodes.size();
    while (--i >= 0) {
        QString from = notesNodes.at(i).toElement().attribute(QStringLiteral("from"));
        if (!from.isEmpty())
            result.insert(from);
    }
    return result.values();
}

QVector<Note> phaseNotes(QDomDocument m_doc, const QString &phasename, bool remove = false)
{
    QVector<Note> result;

    QDomElement phasegroup;
    QDomElement phaseElem = phaseElement(m_doc, phasename, phasegroup);

    QDomElement noteElem = phaseElem.firstChildElement(NOTE);
    while (!noteElem.isNull()) {
        Note note;
        initNoteFromElement(note, noteElem);
        result.append(note);
        QDomElement old = noteElem;
        noteElem = noteElem.nextSiblingElement(NOTE);
        if (remove)
            phaseElem.removeChild(old);
    }
    return result;
}

QVector<Note> XliffStorage::phaseNotes(const QString &phasename) const
{
    return ::phaseNotes(m_doc, phasename, false);
}

QVector<Note> XliffStorage::setPhaseNotes(const QString &phasename, QVector<Note> notes)
{
    QVector<Note> result = ::phaseNotes(m_doc, phasename, true);

    QDomElement phasegroup;
    QDomElement phaseElem = phaseElement(m_doc, phasename, phasegroup);

    for (const Note &note : std::as_const(notes)) {
        QDomElement elem = phaseElem.appendChild(m_doc.createElement(NOTE)).toElement();
        elem.appendChild(m_doc.createTextNode(note.content));
        if (!note.from.isEmpty())
            elem.setAttribute(QStringLiteral("from"), note.from);
        if (note.priority)
            elem.setAttribute(QStringLiteral("priority"), note.priority);
    }

    return result;
}

QString XliffStorage::setPhase(const DocPosition &pos, const QString &phase)
{
    QString PHASENAME = QStringLiteral("phase-name");
    targetInsert(pos, QString()); // adds <taget> if needed

    QDomElement targetElem = targetForPos(pos.entry);
    QString result = targetElem.attribute(PHASENAME);
    if (phase.isEmpty())
        targetElem.removeAttribute(PHASENAME);
    else if (phase != result)
        targetElem.setAttribute(PHASENAME, phase);

    return result;
}

QString XliffStorage::phase(const DocPosition &pos) const
{
    QDomElement targetElem = targetForPos(pos.entry);
    return targetElem.attribute(QStringLiteral("phase-name"));
}

QStringList XliffStorage::context(const DocPosition &pos) const
{
    Q_UNUSED(pos);
    // TODO
    return QStringList(QString());
}

QStringList XliffStorage::matchData(const DocPosition &pos) const
{
    Q_UNUSED(pos);
    return QStringList();
}

QString XliffStorage::id(const DocPosition &pos) const
{
    return unitForPos(pos.entry).attribute(QStringLiteral("id"));
}

bool XliffStorage::isPlural(const DocPosition &pos) const
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

static const QString xliff_states[] = {QStringLiteral("new"),
                                       QStringLiteral("needs-translation"),
                                       QStringLiteral("needs-l10n"),
                                       QStringLiteral("needs-adaptation"),
                                       QStringLiteral("translated"),
                                       QStringLiteral("needs-review-translation"),
                                       QStringLiteral("needs-review-l10n"),
                                       QStringLiteral("needs-review-adaptation"),
                                       QStringLiteral("final"),
                                       QStringLiteral("signed-off")};

TargetState stringToState(const QString &state)
{
    int i = sizeof(xliff_states) / sizeof(QString);
    while (--i > 0 && state != xliff_states[i])
        ;
    return TargetState(i);
}

TargetState XliffStorage::setState(const DocPosition &pos, TargetState state)
{
    targetInsert(pos, QString()); // adds <taget> if needed
    QDomElement targetElem = targetForPos(pos.entry);
    TargetState prev = stringToState(targetElem.attribute(QStringLiteral("state")));
    targetElem.setAttribute(QStringLiteral("state"), xliff_states[state]);

    unitForPos(pos.entry).setAttribute(QStringLiteral("approved"), noyes[state == SignedOff]);
    return prev;
}

TargetState XliffStorage::state(const DocPosition &pos) const
{
    QDomElement targetElem = targetForPos(pos.entry);
    if (!targetElem.hasAttribute(QStringLiteral("state")) && unitForPos(pos.entry).attribute(QStringLiteral("approved")) == QLatin1String("yes"))
        return SignedOff;
    return stringToState(targetElem.attribute(QStringLiteral("state")));
}

bool XliffStorage::isEmpty(const DocPosition &pos) const
{
    ContentEditingData data(ContentEditingData::CheckLength);
    return content(targetForPos(pos.entry), &data).isEmpty();
}

bool XliffStorage::isEquivTrans(const DocPosition &pos) const
{
    return targetForPos(pos.entry).attribute(QStringLiteral("equiv-trans")) != QLatin1String("no");
}

void XliffStorage::setEquivTrans(const DocPosition &pos, bool equivTrans)
{
    targetForPos(pos.entry).setAttribute(QStringLiteral("equiv-trans"), noyes[equivTrans]);
}

QDomElement XliffStorage::unitForPos(int pos) const
{
    if (pos < size())
        return entries.at(m_map.at(pos)).toElement();

    return binEntries.at(pos - size()).toElement();
}

QDomElement XliffStorage::targetForPos(int pos) const
{
    return unitForPos(pos).firstChildElement(bintargettarget[pos < size()]);
}

QDomElement XliffStorage::sourceForPos(int pos) const
{
    return unitForPos(pos).firstChildElement(binsourcesource[pos < size()]);
}

int XliffStorage::binUnitsCount() const
{
    return binEntries.size();
}

int XliffStorage::unitById(const QString &id) const
{
    return m_unitsById.contains(id) ? m_unitsById.value(id) : -1;
}

QString XliffStorage::originalOdfFilePath()
{
    QDomElement file = m_doc.elementsByTagName(QStringLiteral("file")).at(0).toElement();
    return file.attribute(QStringLiteral("original"));
}

void XliffStorage::setOriginalOdfFilePath(const QString &odfFilePath)
{
    QDomElement file = m_doc.elementsByTagName(QStringLiteral("file")).at(0).toElement();
    return file.setAttribute(QStringLiteral("original"), odfFilePath);
}

// END STORAGE TRANSLATION
