/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2009 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

*/

#ifndef TBXPARSER_H
#define TBXPARSER_H

#include <QXmlDefaultHandler>

#include "glossary.h"

namespace GlossaryNS
{

/**
 * loads only data we need to store in memory
 * e.g. skips entries for languages other than en
 * and current project's target language
 *
 * @short TBX glossary parser
 * @author Nick Shaforostoff <shafff@ukr.net>
 */
class TbxParser : public QXmlDefaultHandler
{
    enum State { //localstate for getting chars into right place
        null = 0,
//        termGrp,
        term,
        descripDefinition,
        descripSubjectField
    };

    enum Lang {
        langNull = 0,
        langEn,
        langTarget
    };

public:
    explicit TbxParser(Glossary* glossary)
        : QXmlDefaultHandler()
        , m_glossary(glossary)
    {}

    ~TbxParser() {}

    bool startDocument();
    bool startElement(const QString&, const QString&, const QString&, const QXmlAttributes&);
    bool endElement(const QString&, const QString&, const QString&);
    bool characters(const QString&);

private:
//    bool inTermTag:1;
    State m_state: 8;
    Lang m_lang: 8;
    QString m_termEn;
    QString m_termOther;
    TermEntry m_entry;
    QString m_subjectField;
    Glossary* m_glossary;

};
}
#endif
