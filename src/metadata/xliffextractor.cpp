/*
    XLIFF translation file analyzer

    SPDX-FileCopyrightText: 2011 Albert Astals Cid <aacid@kde.org>
    SPDX-FileCopyrightText: 2015 Nick Shaforostoff <shaforostoff@gmail.com>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "xliffextractor.h"

#include <QFile>
#include <QXmlInputSource>

#include "lokalize_debug.h"
#include "catalog/catalog.h"

class XliffHandler: public QXmlDefaultHandler
{
public:
    XliffHandler() = default;

    bool startElement(const QString& namespaceURI, const QString& localName, const QString& qName, const QXmlAttributes& atts) override;
    bool endElement(const QString& namespaceURI, const QString& localName, const QString& qName) override;
    bool characters(const QString&) override;
    //void endAnalysis(bool complete);

    int total{0};
    int untranslated{0};
    int fuzzy{0};
    int fuzzy_reviewer{0};
    int fuzzy_approver{0};
    QDate lastDate;
    QString lastTranslator;
    QString lastTranslator_fallback;
    QString lastDateString_fallback;

private:
    bool currentEntryFuzzy{false};
    bool currentEntryFuzzy_reviewer{false};
    bool currentEntryFuzzy_approver{false};
    int charCount{0};
};

extern const QString xliff_states[];
TargetState stringToState(const QString& state);

bool XliffHandler::startElement(const QString&, const QString& localName, const QString&, const QXmlAttributes& atts)
{
    //if (fileType == Unknown)
    //    fileType = strcmp(localname, "xliff") ? Other : XLF;

    if (localName == QLatin1String("source")) total++;
    else if (localName == QLatin1String("target")) {
        charCount = 0;

        currentEntryFuzzy = currentEntryFuzzy_reviewer = currentEntryFuzzy_approver = false;
        if (atts.value(QLatin1String("approved")) != QLatin1String("yes")) {
            QString state = atts.value(QLatin1String("state"));
            if (state.length()) {
                TargetState tstate = stringToState(state);
                currentEntryFuzzy = !::isApproved(tstate, ProjectLocal::Translator);
                currentEntryFuzzy_reviewer = !::isApproved(tstate, ProjectLocal::Reviewer);
                currentEntryFuzzy_approver = !::isApproved(tstate, ProjectLocal::Approver);
            }
        }
    } else if (localName == QLatin1String("phase")) {
        QString contactNameString  = atts.value(QLatin1String("contact-name"));
        QString contactEmailString = atts.value(QLatin1String("contact-email"));
        QString dateString         = atts.value(QLatin1String("date"));

        QString currentLastTranslator;
        if (contactNameString.length() && contactEmailString.length())
            currentLastTranslator = contactNameString + " <" + contactEmailString + ">";
        else if (contactNameString.length())
            currentLastTranslator = contactNameString;
        else if (contactEmailString.length())
            currentLastTranslator = contactEmailString;

        if (currentLastTranslator.length()) lastTranslator_fallback = currentLastTranslator;
        if (dateString.length()) {
            lastDateString_fallback = dateString;

            const QDate thisDate = QDate::fromString(dateString, Qt::ISODate);
            if (lastDate.isNull() || thisDate >= lastDate) { // >= Assuming the last one in the file is the real last one
                lastDate = thisDate;
                lastTranslator = currentLastTranslator;
            }
        }
    }
    return true;
}

bool XliffHandler::endElement(const QString&, const QString& localName, const QString&)
{
    if (localName == QLatin1String("target")) {
        if (!charCount) {
            ++untranslated;
        } else if (currentEntryFuzzy) {
            ++fuzzy;
            ++fuzzy_reviewer;
            ++fuzzy_approver;
        } else if (currentEntryFuzzy_reviewer) {
            ++fuzzy_reviewer;
            ++fuzzy_approver;
        } else if (currentEntryFuzzy_approver) {
            ++fuzzy_approver;
        }
    }
    return true;
}

bool XliffHandler::characters(const QString& ch)
{
    charCount += ch.length();
    return true;
}


FileMetaData XliffExtractor::extract(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }

    QXmlInputSource source(&file);
    QXmlSimpleReader xmlReader;
    XliffHandler handler;
    xmlReader.setContentHandler(&handler);

    bool ok = xmlReader.parse(source);

    if (!ok)
        qCDebug(LOKALIZE_LOG) << "Parsing failed.";


    //TODO WordCount
    FileMetaData m;
    m.fuzzy      = handler.fuzzy;
    m.translated = handler.total - handler.untranslated - handler.fuzzy;
    m.untranslated = handler.untranslated;
    m.filePath = filePath;

    //qCDebug(LOKALIZE_LOG)<<"parsed"<<filePath<<m.fuzzy<<m.translated<<m.untranslated<<handler.fuzzy_approver<<handler.fuzzy_reviewer;
    Q_ASSERT(m.fuzzy >= 0 && m.untranslated >= 0 && handler.total >= 0);

    m.translated_approver = handler.total - handler.untranslated - handler.fuzzy_approver;
    m.translated_reviewer = handler.total - handler.untranslated - handler.fuzzy_reviewer;
    m.fuzzy_approver = handler.fuzzy_approver;
    m.fuzzy_reviewer = handler.fuzzy_reviewer;

    m.lastTranslator = handler.lastTranslator.length() ? handler.lastTranslator : handler.lastTranslator_fallback;
    m.translationDate = handler.lastDate.isValid() ? handler.lastDate.toString(Qt::ISODate) : handler.lastDateString_fallback;

    return m;
}
