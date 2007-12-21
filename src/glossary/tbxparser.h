/* ****************************************************************************
  This file is part of KAider

  Copyright (C) 2007 by Nick Shaforostoff <shafff@ukr.net>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

  In addition, as a special exception, the copyright holders give
  permission to link the code of this program with any edition of
  the Qt library by Trolltech AS, Norway (or with modified versions
  of Qt that use the same license as Qt), and distribute linked
  combinations including the two.  You must obey the GNU General
  Public License in all respects for all of the code used other than
  Qt. If you modify this file, you may extend this exception to
  your version of the file, but you are not obligated to do so.  If
  you do not wish to do so, delete this exception statement from
  your version.

**************************************************************************** */

#ifndef TBXPARSER_H
#define TBXPARSER_H

#include <QXmlDefaultHandler>

#include "glossary.h"
//class Glossary;

/**
 * loads only data we need to store in memory
 * e.g. skips entries for languages other than en
 * and current project's target language
 *
 *	@author Nick Shaforostoff <shafff@ukr.net>
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
        langOther
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
    bool inTermTag:1;
    State m_state:8;
    Lang m_lang:8;
    QString m_termEn;
    QString m_termOther;
    TermEntry m_entry;
    QString m_subjectField;
    Glossary* m_glossary;

};

#endif
