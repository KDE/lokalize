/*
    XLIFF translation file analyzer

    Copyright (C) 2011 Albert Astals Cid <aacid@kde.org>
    Copyright (C) 2015 Nick Shaforostoff <shaforostoff@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "xliffextractor.h"
#include "catalog.h"
#include <QFile>
#include <QXmlInputSource>
#include <fstream>

XliffExtractor::XliffExtractor()
{
}

class XliffHandler: public QXmlDefaultHandler
{
public:
    XliffHandler()
     : charCount(0)
    {
        total = 0;
        untranslated = 0;
        fuzzy = 0;
        fuzzy_reviewer = 0;
        fuzzy_approver = 0;
        
    }
    bool startElement(const QString& namespaceURI, const QString& localName, const QString& qName, const QXmlAttributes& atts);
    bool endElement(const QString& namespaceURI, const QString& localName, const QString& qName);
    bool characters(const QString&);
    //void endAnalysis(bool complete);

    int total;
    int untranslated;
    int fuzzy;
    int fuzzy_reviewer;
    int fuzzy_approver;
    QDate lastDate;
    QString lastTranslator;
        
private:
    bool currentEntryFuzzy;
    bool currentEntryFuzzy_reviewer; 
    bool currentEntryFuzzy_approver;
    int charCount;
};

extern const QString xliff_states[];
TargetState stringToState(const QString& state);

bool XliffHandler::startElement(const QString&, const QString& localName, const QString&, const QXmlAttributes& atts)
{
    //if (fileType == Unknown) 
    //    fileType = strcmp(localname, "xliff") ? Other : XLF;
    
    if (localName==QLatin1String("source")) total++;
    if (localName==QLatin1String("target"))
    {
        charCount=0;

        currentEntryFuzzy = currentEntryFuzzy_reviewer = currentEntryFuzzy_approver = false;
        if (atts.value(QLatin1String("approved"))!=QLatin1String("yes"))
        {
            QString state = atts.value(QLatin1String("state"));
            if (state.length())
            {
                TargetState tstate=stringToState(state);
                currentEntryFuzzy = !::isApproved(tstate, ProjectLocal::Translator);
                currentEntryFuzzy_reviewer = !::isApproved(tstate, ProjectLocal::Reviewer);
                currentEntryFuzzy_approver = !::isApproved(tstate, ProjectLocal::Approver);
            }
        }
    }
    /*
    if (!strcmp(localname, "phase")) {
        string contactNameString, contactEmailString, dateString;
        for (int i = 0; i < nb_attributes; ++i) {
            if (!strcmp(attr[i*5], "contact-name")) {
                contactNameString = std::string(attr[i*5+3], (attr[i*5+4] - attr[i*5+3]));
            } else if (!strcmp(attr[i*5], "contact-email")) {
                contactEmailString = std::string(attr[i*5+3], (attr[i*5+4] - attr[i*5+3]));
            } else if (!strcmp(attr[i*5], "date")) {
                dateString = std::string(attr[i*5+3], (attr[i*5+4] - attr[i*5+3]));
            }
        }
        
        if (!dateString.empty()) {
            const QDate thisDate = QDate::fromString(dateString.c_str(), Qt::ISODate);
            if (lastDate.isNull() || thisDate >= lastDate) { // >= Assuming the last one in the file is the real last one
                lastDate = thisDate;
                if (!contactNameString.empty() && !contactEmailString.empty()) {
                    lastTranslator = contactNameString + " <" + contactEmailString + ">";
                } else if (!contactNameString.empty()) {
                    lastTranslator = contactNameString;
                } else if (!contactEmailString.empty()) {
                    lastTranslator = contactEmailString;
                } else {
                    lastTranslator = string();
                }
            }
        }
    }*/
    return true;
}

bool XliffHandler::endElement(const QString&, const QString& localName, const QString&)
{    
    if (localName==QLatin1String("target"))
    {
        if (!charCount)
        {
            ++untranslated;
        }
        else if (currentEntryFuzzy)
        {
            ++fuzzy;
            ++fuzzy_reviewer;
            ++fuzzy_approver;
        }
        else if (currentEntryFuzzy_reviewer)
        {
            ++fuzzy_reviewer;
            ++fuzzy_approver;
        }
        else if (currentEntryFuzzy_approver)
        {
            ++fuzzy_approver;
        }
    }
    return true;
}

bool XliffHandler::characters(const QString& ch)
{
    charCount+=ch.length();
    return true;
}


void XliffExtractor::extract(const QString& filePath, FileMetaData& m)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QXmlInputSource source(&file);
    QXmlSimpleReader xmlReader;
    XliffHandler handler;
    xmlReader.setContentHandler(&handler);

    bool ok = xmlReader.parse(source);

    if (!ok)
        qDebug() << "Parsing failed.";


    //TODO WordCount
    m.fuzzy      = handler.fuzzy;
    m.translated = handler.total-handler.untranslated-handler.fuzzy;
    m.untranslated=handler.untranslated;
    m.filePath = filePath;

    //qDebug()<<"parsed"<<filePath<<m.fuzzy<<m.translated<<m.untranslated<<handler.fuzzy_approver<<handler.fuzzy_reviewer;
    Q_ASSERT(m.fuzzy>=0 && m.untranslated>=0 && handler.total>=0);

    m.translated_approver=handler.total-handler.untranslated-handler.fuzzy_approver;
    m.translated_reviewer=handler.total-handler.untranslated-handler.fuzzy_reviewer;
    m.fuzzy_approver=handler.fuzzy_approver;
    m.fuzzy_reviewer=handler.fuzzy_reviewer;
}
