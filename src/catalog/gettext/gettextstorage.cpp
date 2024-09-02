/*
  SPDX-FileCopyrightText: 2008-2014 Nick Shaforostoff <shaforostoff@kde.ru>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "gettextstorage.h"

#include "lokalize_debug.h"

#include "gettextheader.h"
#include "catalogitem_private.h"
#include "gettextimport.h"
#include "gettextexport.h"

#include "project.h"

#include <QMutex>
#include <QMutexLocker>
#include <QRegularExpression>
#include <QString>
#include <QMap>

#include <klocalizedstring.h>

QMutex regExMutex;

// static QString GNUPluralForms(const QString& lang);

using namespace GettextCatalog;

//BEGIN OPEN/SAVE

int GettextStorage::load(QIODevice* device/*, bool readonly*/)
{
    //GettextImportPlugin importer=GettextImportPlugin(readonly?(new ExtraDataSaver()):(new ExtraDataListSaver()));
    GettextImportPlugin importer;
    ConversionStatus status = OK;
    int errorLine{};
    {
        QMutexLocker locker(&regExMutex);
        status = importer.open(device, this, &errorLine);
    }

    //for langs with more than 2 forms
    //we create any form-entries additionally needed
    uint i = 0;
    uint lim = size();
    while (i < lim) {
        CatalogItem& item = m_entries[i];
        if (item.isPlural()
            && item.msgstrPlural().count() < m_numberOfPluralForms
           ) {
            QVector<QString> msgstr(item.msgstrPlural());
            while (msgstr.count() < m_numberOfPluralForms)
                msgstr.append(QString());
            item.setMsgstr(msgstr);
        }
        ++i;

    }
    //qCompress(m_storage->m_catalogExtraData.join("\n\n").toUtf8(),9);

    return status == OK ? 0 : (errorLine + 1);
}

bool GettextStorage::save(QIODevice* device, bool belongsToProject)
{
    QString header = m_header.msgstr();
    QString comment = m_header.comment();
    QString catalogProjectId;//=m_url.fileName();
    //catalogProjectId=catalogProjectId.left(catalogProjectId.lastIndexOf('.'));
    {
        QMutexLocker locker(&regExMutex);
        updateHeader(header, comment,
                     m_targetLangCode,
                     m_numberOfPluralForms,
                     catalogProjectId,
                     m_generatedFromDocbook,
                     belongsToProject,
                     /*forSaving*/true,
                     QStringLiteral("utf-8")); // we unconditionally write out as UTF-8
    }
    m_header.setMsgstr(header);
    m_header.setComment(comment);

    GettextExportPlugin exporter(Project::instance()->wordWrap());

    ConversionStatus status = OK;
    status = exporter.save(device/*x-gettext-translation*/, this);

    return status == OK;
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
    static const QString altSepText(QStringLiteral(" | "));
    static const QString ctype = i18n("separator for different-length string alternatives");
    return InlineTag(i, i, InlineTag::x, QString::number(i), QString(), altSepText, ctype);
}

static CatalogString makeCatalogString(const QString& string)
{
    CatalogString result;
    result.string = string;

    int i = 0;

    while ((i = result.string.indexOf(altSep, i)) != -1) {
        result.string[i] = TAGRANGE_IMAGE_SYMBOL;
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
QString GettextStorage::sourceWithPlurals(const DocPosition& pos, bool truncateFirstLine) const
{
    if (m_entries.at(pos.entry).isPlural()) {
        const QVector<QString> plurals = m_entries.at(pos.entry).msgidPlural();
        QString pluralString;
        for (int i = 0; i < plurals.size(); i++) {
            QString str = plurals.at(i);
            if (truncateFirstLine) {
                const int truncatePos = str.indexOf(QLatin1Char('\n'));
                if (truncatePos != -1)
                    str.truncate(truncatePos);
            }
            pluralString += str;
            if (i != plurals.size() - 1) {
                pluralString += QLatin1Char('|');
            }
        }
        return pluralString;
    } else {
        QString str = m_entries.at(pos.entry).msgid(pos.form);
        if (truncateFirstLine) {
            const int truncatePos = str.indexOf(QLatin1Char('\n'));
            if (truncatePos != -1)
                str.truncate(truncatePos);
        }
        return str;
    }
}
QString GettextStorage::targetWithPlurals(const DocPosition& pos, bool truncateFirstLine) const
{
    if (m_entries.at(pos.entry).isPlural()) {
        const QVector<QString> plurals = m_entries.at(pos.entry).msgstrPlural();
        QString pluralString;
        for (int i = 0; i < plurals.size(); i++) {
            QString str = plurals.at(i);
            if (truncateFirstLine) {
                const int truncatePos = str.indexOf(QLatin1Char('\n'));
                if (truncatePos != -1)
                    str.truncate(truncatePos);
            }
            pluralString += str;
            if (i != plurals.size() - 1) {
                pluralString += QLatin1Char('|');
            }
        }
        return pluralString;
    } else {
        QString str = m_entries.at(pos.entry).msgstr(pos.form);
        if (truncateFirstLine) {
            const int truncatePos = str.indexOf(QLatin1Char('\n'));
            if (truncatePos != -1)
                str.truncate(truncatePos);
        }
        return str;
    }
}

void GettextStorage::targetDelete(const DocPosition& pos, int count)
{
    m_entries[pos.entry].d._msgstrPlural[pos.form].remove(pos.offset, count);
}
void GettextStorage::targetInsert(const DocPosition& pos, const QString& arg)
{
    m_entries[pos.entry].d._msgstrPlural[pos.form].insert(pos.offset, arg);
}
void GettextStorage::setTarget(const DocPosition& pos, const QString& arg)
{
    m_entries[pos.entry].d._msgstrPlural[pos.form] = arg;
}


void GettextStorage::targetInsertTag(const DocPosition& pos, const InlineTag& tag)
{
    Q_UNUSED(tag);
    targetInsert(pos, altSep);
}

InlineTag GettextStorage::targetDeleteTag(const DocPosition& pos)
{
    targetDelete(pos, 1);
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
    static const QRegularExpression alt_trans_mark_re(QStringLiteral("^#\\|"));
    QStringList prev = m_entries.at(pos.entry).comment().split(QLatin1Char('\n')).filter(alt_trans_mark_re);

    QString oldSingular;
    QString oldPlural;

    QString* cur = &oldSingular;
    QStringList::iterator it = prev.begin();
    static const QString msgid_plural_alt = QStringLiteral("#| msgid_plural \"");
    while (it != prev.end()) {
        if (it->startsWith(msgid_plural_alt))
            cur = &oldPlural;

        const int start = it->indexOf(QLatin1Char('\"')) + 1;
        const int end = it->lastIndexOf(QLatin1Char('\"'));
        if (start && end != -1) {
            if (!cur->isEmpty())
                (*cur) += QLatin1Char('\n');
            if (!(cur->isEmpty() && (end - start) == 0)) //for multiline msgs
                (*cur) += QStringView(*it).mid(start, end - start);
        }
        ++it;
    }
    if (pos.form == 0)
        cur = &oldSingular;

    cur->replace(QStringLiteral("\\\""), QStringLiteral("\""));

    QVector<AltTrans> result;
    if (!cur->isEmpty())
        result << AltTrans(CatalogString(*cur), i18n("Previous source value, saved by Gettext during transition to a newer POT template"));

    return result;
}

Note GettextStorage::setNote(DocPosition pos, const Note& note)
{
    //qCWarning(LOKALIZE_LOG)<<"s"<<m_entries.at(pos.entry).comment();
    Note oldNote;
    QVector<Note> l = notes(pos);
    if (l.size()) oldNote = l.first();

    QStringList comment = m_entries.at(pos.entry).comment().split(QLatin1Char('\n'));
    //remove previous comment;
    QStringList::iterator it = comment.begin();
    while (it != comment.end()) {
        if (it->startsWith(QLatin1String("# ")))
            it = comment.erase(it);
        else
            ++it;
    }
    if (note.content.size())
        comment.prepend(QStringLiteral("# ") + note.content.split(QLatin1Char('\n')).join(QStringLiteral("\n# ")));
    m_entries[pos.entry].setComment(comment.join(QLatin1Char('\n')));

    //qCWarning(LOKALIZE_LOG)<<"e"<<m_entries.at(pos.entry).comment();
    return oldNote;
}

QVector<Note> GettextStorage::notes(const DocPosition& docPosition, const QRegularExpression& re, int preLen) const
{
    QVector<Note> result;
    QString content;

    const QStringList note = m_entries.at(docPosition.entry).comment().split(QLatin1Char('\n')).filter(re);

    for (const QString &s : note) {
        if (s.size() >= preLen) {
            content += QStringView(s).mid(preLen);
            content += QLatin1Char('\n');
        }
    }

    if (!content.isEmpty()) {
        content.chop(1);
        result << Note(content);
    }
    return result;

//i18nc("@info PO comment parsing. contains filename","<i>Place:</i>");
//i18nc("@info PO comment parsing","<i>GUI place:</i>");
}

QVector<Note> GettextStorage::notes(const DocPosition& docPosition) const
{
    static const QRegularExpression nre(QStringLiteral("^# "));
    return notes(docPosition, nre, 2);
}

QVector<Note> GettextStorage::developerNotes(const DocPosition& docPosition) const
{
    static const QRegularExpression dnre(QStringLiteral("^#\\. (?!i18n: file:)"));
    return notes(docPosition, dnre, 3);
}

QStringList GettextStorage::sourceFiles(const DocPosition& pos) const
{
    QStringList result;
    QStringList commentLines = m_entries.at(pos.entry).comment().split(QLatin1Char('\n'));

    static const QRegularExpression i18n_file_re(QStringLiteral("^#. i18n: file: "));
    const auto uiLines = commentLines.filter(i18n_file_re);
    for (const QString &uiLine : uiLines) {
        const auto fileRefs = QStringView(uiLine).mid(15).split(QLatin1Char(' '));
        for (const auto &fileRef : fileRefs) {
            result << fileRef.toString();
        }
    }

    bool hasUi = !result.isEmpty();
    static const QRegularExpression cpp_re(QStringLiteral("^#: "));
    const auto cppLines = commentLines.filter(cpp_re);
    for (const QString &cppLine : cppLines) {
        const auto fileRefs = QStringView(cppLine).mid(3).split(QLatin1Char(' '));
        for (const auto &fileRef : fileRefs) {
            if (hasUi && fileRef.startsWith(QLatin1String("rc.cpp:"))) {
                continue;
            }
            result << fileRef.toString();
        }
    }
    return result;
}

QStringList GettextStorage::context(const DocPosition& pos) const
{
    return matchData(pos);
}

QStringList GettextStorage::matchData(const DocPosition& pos) const
{
    QString ctxt = m_entries.at(pos.entry).msgctxt();

    //KDE-specific
    //Splits @info:whatsthis and actual note
    /*    if (ctxt.startsWith('@') && ctxt.contains(' '))
        {
            QStringList result(ctxt.section(' ',0,0,Qt::SectionSkipEmpty));
            result<<ctxt.section(' ',1,-1,Qt::SectionSkipEmpty);
            return result;
        }*/
    return QStringList(ctxt);
}

QString GettextStorage::id(const DocPosition& pos) const
{
    //entries in gettext format may be non-unique
    //only if their msgctxts are different

    QString result = source(pos);
    result.remove(QLatin1Char('\n'));
    result.prepend(m_entries.at(pos.entry).msgctxt() + QLatin1String(":\n"));
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
    if (newHeader.isValid()) {
        // normalize the values - ensure every key:value pair is only on a single line
        QString values = newHeader.msgstr();
        values.replace(QStringLiteral("\\n"), QStringLiteral("\\n\n"));
//       qCDebug(LOKALIZE_LOG) << "Normalized header: " << values;
        QString comment = newHeader.comment();
        QString catalogProjectId;//=m_url.fileName(); FIXME m_url is always empty
        //catalogProjectId=catalogProjectId.left(catalogProjectId.lastIndexOf('.'));
        bool belongsToProject = m_url.contains(Project::instance()->poDir());

        updateHeader(values,
                     comment,
                     m_targetLangCode,
                     m_numberOfPluralForms,
                     catalogProjectId,
                     m_generatedFromDocbook,
                     belongsToProject,
                     /*forSaving*/true,
                     QString::fromLatin1(m_codec));
        m_header = newHeader;
        m_header.setComment(comment);
        m_header.setMsgstr(values);

//       setClean(false);
        //Q_EMIT signalHeaderChanged();

        return true;
    }
    qCWarning(LOKALIZE_LOG) << "header Not valid";
    return false;
}



