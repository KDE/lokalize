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

#include "glossary.h"
#include "tbxparser.h"
#include "project.h"
#include <kdebug.h>

TbxParser::TbxParser(Glossary* glossary)
        : QXmlDefaultHandler()
        , m_glossary(glossary)
{}


TbxParser::~TbxParser()
{}


bool TbxParser::startDocument()
{
    m_state=null;
    m_lang=langNull;
    inTermTag=false;
    return true;
}



bool TbxParser::startElement( const QString&, const QString&,
                                    const QString& qName,
                                    const QXmlAttributes& attr)
{
    if (qName=="langSet")
    {
        if (attr.value("xml:lang")=="en")
            m_lang=langEn;
//         else
//             m_lang=langOther;
        else if (attr.value("xml:lang")==Project::instance()->langCode())
            m_lang=langOther;
        else
            m_lang=langNull;

    }
    else if (qName=="term")
    {
        inTermTag=true;
        //m_state=term;
        //m_termEn.clear();
    }
    else if (qName=="termEntry")
    {
        m_termEn.clear();
        m_termOther.clear();
        m_termOtherList.clear();
    }
    return true;
}

bool TbxParser::endElement(const QString&,const QString&,const QString& qName)
{
    if (qName=="term")
    {
        inTermTag=false;
        //m_state=null;
        if (m_lang==langOther)
        {
            m_termOtherList << m_termOther;
            m_termOther.clear();
        }
//         else if (m_lang==langOther)
//         {
//             
//         }

    }
    else if (qName=="termEntry")
    {
        //sanity check --maybe this entry is for another language?
        if (m_termOtherList.isEmpty()||m_termEn.isEmpty())
            return true;

        QStringList words(m_termEn.split(" ",QString::SkipEmptyParts));
        int i=0;
        for (;i<words.size();++i)
        {
            m_glossary->wordHash.insert(words.at(i).toLower(),m_glossary->termList.size());
        }
        m_termOtherList.prepend(m_termEn);
        m_glossary->termList.append(m_termOtherList);
        m_termOtherList.clear();

    }
    return true;
}



bool TbxParser::characters ( const QString & ch )
{
    if(inTermTag)
    {
//         kWarning() << "O " << ch << endl;
        if (m_lang==langEn)
            m_termEn+=ch;
        else if (m_lang==langOther)
            m_termOther+=ch;
//         kWarning() << "O m_termEn " << m_termEn << endl;
//         kWarning() << "O m_termO " << m_termOther << endl;
    }

    return true;
}

