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

#include "tbxparser.h"

#include "glossary.h"
#include "project.h"
#include <kdebug.h>

using namespace GlossaryNS;

bool TbxParser::startDocument()
{
    m_state=null;
    m_lang=langNull;
    return true;
}



bool TbxParser::startElement( const QString&, const QString&,
                                    const QString& qName,
                                    const QXmlAttributes& attr)
{
    if (qName=="langSet")
    {
        if (attr.value("xml:lang").startsWith("en"))
            m_lang=langEn;
        else if (attr.value("xml:lang")==Project::instance()->langCode())
            m_lang=langTarget;
        else
            m_lang=langNull;
    }
    else if (qName=="term")
    {
        m_state=term;
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
            m_state=descripDefinition;
        else if (attr.value("type")=="subjectField")
            m_state=descripSubjectField;
    }
    return true;
}

bool TbxParser::endElement(const QString&,const QString&,const QString& qName)
{
    if (qName=="term")
    {
        if (m_lang==langEn)
        {
            m_entry.english << m_termEn;
            m_termEn.clear();
            m_entry.english.last().squeeze();
        }
        else if (m_lang==langTarget)
        {
            m_entry.target << m_termOther;
            m_termOther.clear();
            m_entry.target.last().squeeze();
        }

    }
    else if (qName=="descrip")
    {
        if (m_state==descripSubjectField && !m_subjectField.isEmpty())
        {
            m_entry.subjectField=Project::instance()->glossary()->subjectFields.indexOf(m_subjectField);
            if (m_entry.subjectField==-1)//got this field value for the first time
            {
                m_entry.subjectField=Project::instance()->glossary()->subjectFields.size();
                Project::instance()->glossary()->subjectFields << m_subjectField;
            }
            m_subjectField.clear();
        }

    }
    else if (qName=="termEntry")
    {
        //sanity check --maybe this entry is only for another language?
        if (m_entry.target.isEmpty()||m_entry.english.isEmpty())
            return true;

        int index=m_glossary->termList.count();
        m_glossary->termList.append(m_entry);
        m_glossary->hashTermEntry(index);

        m_entry.clear();
    }
    m_state=null;
    return true;
}



bool TbxParser::characters ( const QString & ch )
{
    if(m_state==term)
    {
        if (m_lang==langEn)
            m_termEn+=ch.toLower();//this is important
        else if (m_lang==langTarget)
            m_termOther+=ch;
    }
    else if (m_state==descripDefinition)
        m_entry.definition+=ch;
    else if (m_state==descripSubjectField)
        m_subjectField+=ch;


    return true;
}

