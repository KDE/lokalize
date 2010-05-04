/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2009 by Nick Shaforostoff <shafff@ukr.net>

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

#ifndef TBXPARSER_H
#define TBXPARSER_H

#include <QXmlDefaultHandler>

#include "glossary.h"

namespace GlossaryNS {

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
    enum State //localstate for getting chars into right place
    {
        null=0,
//        termGrp,
        term,
        descripDefinition,
        descripSubjectField
    };

    enum Lang
    {
        langNull=0,
        langEn,
        langTarget
    };

public:
    TbxParser(Glossary* glossary)
        : QXmlDefaultHandler()
        , m_glossary(glossary)
    {}

    ~TbxParser(){}

    bool startDocument();
    bool startElement(const QString&,const QString&,const QString&,const QXmlAttributes&);
    bool endElement(const QString&,const QString&,const QString&);
    bool characters(const QString&);

private:
//    bool inTermTag:1;
    State m_state:8;
    Lang m_lang:8;
    QString m_termEn;
    QString m_termOther;
    TermEntry m_entry;
    QString m_subjectField;
    Glossary* m_glossary;

};
}
#endif
