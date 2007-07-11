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

// TbxParser::TbxParser(Glossary* glossary)
//         : QXmlDefaultHandler()
//         , m_glossary(glossary)
// {}
// 
// 
// TbxParser::~TbxParser()
// {}


bool TbxParser::startDocument()
{
    m_state=null;
    m_lang=langNull;
//     inTermTag=false;
    return true;
}



bool TbxParser::startElement( const QString&, const QString&,
                                    const QString& qName,
                                    const QXmlAttributes& attr)
{
//     kWarning() << qName << endl;
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
//         inTermTag=true;
        m_state=term;
        //m_termEn.clear();
    }
    else if (qName=="termEntry")
    {
        m_termEn.clear();
        m_termOther.clear();
        m_entry.clear();
        m_entry.id=attr.value("id");
    }
    else if (qName=="descrip")
    {
        if (attr.value("type")=="definition")
        {
            m_state=descripDefinition;
        }
        else if (attr.value("type")=="subjectField")
        {
            m_state=descripSubjectField;
        }
    }
    return true;
}

bool TbxParser::endElement(const QString&,const QString&,const QString& qName)
{
    if (qName=="term")
    {
//         inTermTag=false;
        if (m_lang==langEn)
        {
            m_entry.english << m_termEn;
            m_termEn.clear();
        }
        else if (m_lang==langOther)
        {
            m_entry.target << m_termOther;
            m_termOther.clear();
        }

    }
    else if (qName=="termEntry")
    {
        //sanity check --maybe this entry is only for another language?
        if (m_entry.target.isEmpty()||m_entry.english.isEmpty())
            return true;
//         kWarning() << m_entry.target.size() << " " << m_entry.english.size()<< endl;
        int i=0;
        int j;
        for (;i<m_entry.english.size();++i)
        {
            QStringList words(m_entry.english.at(i).split(' ',QString::SkipEmptyParts));
            for (j=0;j<words.size();++j)
            {
                m_glossary->wordHash.insert(words.at(i),m_glossary->termList.count());
            }

        }
        m_glossary->termList.append(m_entry);

        m_entry.clear();
    }
    else if (qName=="descrip")
    {
        if (m_state==descripSubjectField)
        {
            m_entry.subjectField=Project::instance()->glossary()->subjectFields.indexOf(m_subjectField);
            if (m_entry.subjectField==-1)
            {
                m_entry.subjectField=Project::instance()->glossary()->subjectFields.size();
                Project::instance()->glossary()->subjectFields << m_subjectField;
            }
            m_subjectField.clear();
        }

    }
    m_state=null;
    return true;
}



bool TbxParser::characters ( const QString & ch )
{
    if(m_state==term/*inTermTag*/)
    {
//         kWarning() << "O " << ch << endl;
        if (m_lang==langEn)
            m_termEn+=ch.toLower();//this is important
        else if (m_lang==langOther)
            m_termOther+=ch;
//         kWarning() << "O m_termEn " << m_termEn << endl;
//         kWarning() << "O m_termO " << m_termOther << endl;
    }
    else if (m_state==descripDefinition)
    {
        m_entry.definition+=ch;
    }
    else if (m_state==descripSubjectField)
    {
        m_subjectField+=ch;
    }


    return true;
}

