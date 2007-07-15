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

#include <glossary.h>

#include "tbxparser.h"
#include "project.h"

#include <kdebug.h>

#include <QFile>
#include <QTextStream>
#include <QXmlSimpleReader>
#include <QXmlStreamReader>

void Glossary::load(const QString& p)
{
    clear();
    path=p;

    TbxParser parser(this);
    QXmlSimpleReader reader;
    reader.setContentHandler(&parser);

    QFile file(p);
    if (!file.open(QFile::ReadOnly | QFile::Text))
         return;
    QXmlInputSource xmlInputSource(&file);
    if (!reader.parse(xmlInputSource))
    {
         kWarning() << "failed to load "<< path<< endl;
    }


}

void Glossary::add(const TermEntry& entry)
{
    QFile stream(path);
    if (!stream.open(QFile::ReadWrite | QFile::Text))
         return;

    int id=0;
    if (stream.size()==0)
    {
        stream.write(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<!DOCTYPE martif PUBLIC \"ISO 12200:1999A//DTD MARTIF core (DXFcdV04)//EN\" \"TBXcdv04.dtd\">\n"
        "<martif type=\"TBX\" xml:lang=\"en\">\n"
        "    <martifHeader>\n"
        "        <fileDesc>\n"
        "            <titleStmt>\n"
        "                <title>Your Team Glossary</title>\n"
        "            </titleStmt>\n"
        "        </fileDesc>\n"
        "        <encodingDesc>\n"
        "            <p type=\"DCSName\">SYSTEM &quot;TBXDCSv05b.xml&quot;</p>\n"
        "        </encodingDesc>\n"
        "    </martifHeader>\n"
        "    <text>\n"
        "        <body>\n"
                    );
    }
    else
    {

//     QTextStream stream(&file);
//     stream.setCodec("UTF-8");
//     stream.setAutoDetectUnicode(true);
        QByteArray line(stream.readLine());
        QList<int> idlist;
        QRegExp rx("<termEntry id=\"([0-9]*)\">");
        while(!stream.atEnd()&&!line.contains("</body>"))
    //    while(!stream.atEnd()&&!stream.pos()+20>stream.size())
        {
            if (rx.indexIn(line)!=-1);
            {
                idlist<<rx.cap(1).toInt();
            }
            line=stream.readLine();
        }
    //    stream.seek(stream.pos());
        if (!idlist.isEmpty())
        {
            qSort(idlist);
            while (idlist.contains(id))
                ++id;
        }
        stream.seek(stream.pos()-line.size());
    }

    QByteArray out;
    out.reserve(256);
    out+="            ";
    QXmlStreamWriter xmlOut(&out);
    xmlOut.setAutoFormatting(true);
    xmlOut.writeStartElement("termEntry");
    xmlOut.writeAttribute("id",QString("%1").arg(id));
    if (entry.subjectField!=-1)
    {
        xmlOut.writeStartElement("descrip");
        xmlOut.writeAttribute("type","subjectField");
        xmlOut.writeCharacters(subjectFields.at(entry.subjectField));
        xmlOut.writeEndElement();
    }

    if (!entry.definition.isEmpty())
    {
        xmlOut.writeStartElement("descrip");
        xmlOut.writeAttribute("type","definition");
        xmlOut.writeCharacters(entry.definition);
        xmlOut.writeEndElement();
    }

    int i=0;
    for (;i<entry.english.size();++i)
    {
        if (entry.english.at(i).isEmpty())
            continue;

        xmlOut.writeStartElement("langSet");
        //xmlOut.writeAttribute("xml","lang","en");
        xmlOut.writeAttribute("xml:lang","en");
        xmlOut.writeStartElement("ntig");
        xmlOut.writeStartElement("termGrp");
        xmlOut.writeTextElement("term",entry.english.at(i));
        xmlOut.writeEndElement();
        xmlOut.writeEndElement();
        xmlOut.writeEndElement();
    }
    for (i=0;i<entry.target.size();++i)
    {
        if (entry.target.at(i).isEmpty())
            continue;
        xmlOut.writeStartElement("langSet");
        //xmlOut.writeAttribute("xml","lang",langCode());
        xmlOut.writeAttribute("xml:lang",Project::instance()->langCode());
        xmlOut.writeStartElement("ntig");
        xmlOut.writeStartElement("termGrp");
        xmlOut.writeTextElement("term",entry.target.at(i));
        xmlOut.writeEndElement();
        xmlOut.writeEndElement();
        xmlOut.writeEndElement();
    }
    xmlOut.writeEndElement();
    out.replace("\n\n","\n");
    out.replace("\n","\n            ");

    out+=
"\n        </body>"
"\n    </text>"
"\n</martif>\n";
    stream.write(out);
//     int indent=3;
//     indent=writeLine(indent,QString("<termEntry id=\"%1\">").arg(id).toUtf8(),stream);
//     indent=writeLine(indent,"<langSet xml:lang=\"en\">",stream);
//     indent=writeLine(indent,"<ntig>",stream);
//     indent=writeLine(indent,"<termGrp>",stream);
//     indent=writeLine(indent,QString("<term>%1</term>").arg(term.english).toUtf8(),stream);
//     //indent=writeLine(indent,"<termNote type="partOfSpeech">noun</termNote>
//     indent=writeLine(indent,"</termGrp>",stream);
//     //indent=writeLine(indent,"<descrip type="context"></descrip>
//     indent=writeLine(indent,"</ntig>",stream);
//     indent=writeLine(indent,"</langSet>",stream);
//     indent=writeLine(indent,QString("<langSet xml:lang=\"%1\">").arg(langCode()).toUtf8(),stream);
//     indent=writeLine(indent,"<ntig>",stream);
//     indent=writeLine(indent,"<termGrp>",stream);
//     indent=writeLine(indent,QString("<term>%1</term>").arg(term.target).toUtf8(),stream);
// //     indent=writeLine(indent,"<termNote type="partOfSpeech">сущ.</termNote>",stream);
//     indent=writeLine(indent,"</termGrp>",stream);
//     indent=writeLine(indent,"</ntig>",stream);
//     indent=writeLine(indent,"</langSet>",stream);
//     indent=writeLine(indent,"</termEntry>",stream);
//     indent=writeLine(indent,"</body>",stream);
//     indent=writeLine(indent,"</text>",stream);
//     indent=writeLine(indent,"</martif>",stream);

}

void Glossary::change(const TermEntry& entry)
{
    QFile in(path);
    if (!in.open(QFile::ReadOnly | QFile::Text))
         return;

    QByteArray out;
    out.reserve(in.size()+256);
//     QFile out(glossaryPath()+".tmp");
//     if (!out.open(QFile::WriteOnly | QFile::Text))
//          return;


    QXmlStreamReader xmlIn(&in);
    QXmlStreamWriter xmlOut(&out);
    xmlOut.setAutoFormatting(true);
    while (!xmlIn.atEnd())
    {
        if (xmlIn.readNext()==QXmlStreamReader::StartElement
           &&xmlIn.name()=="termEntry"
            &&xmlIn.attributes().value("id")==entry.id           )
        {
            //we aint changing starting termEntry
            xmlOut.writeCurrentToken(xmlIn);
            //first, write _our_ meta data
// #if 0
            if (entry.subjectField!=-1)
            {
                xmlOut.writeStartElement("descrip");
                xmlOut.writeAttribute("type","subjectField");
                xmlOut.writeCharacters(subjectFields.at(entry.subjectField));
                xmlOut.writeEndElement();
            }

            if (!entry.definition.isEmpty())
            {
                xmlOut.writeStartElement("descrip");
                xmlOut.writeAttribute("type","definition");
                xmlOut.writeCharacters(entry.definition);
                xmlOut.writeEndElement();
            }
// #endif
            //write data from In, skipping data that we support
             do
//             while(!(   xmlIn.readNext()==QXmlStreamReader::EndElement
//                          &&xmlIn.name()=="termEntry"   ));
            {
                if (xmlIn.tokenType()==QXmlStreamReader::StartElement)
                {
                    if (xmlIn.name()=="descrip")
                    {
                        if (xmlIn.attributes().value("type")=="subjectField"
                           ||xmlIn.attributes().value("type")=="definition")
                        {
                             //skip this data in input stream
                             while (! (xmlIn.readNext()==QXmlStreamReader::EndElement
                                       &&xmlIn.name()=="descrip") )
                                ;
//                              xmlIn.readNext();
//                              continue;
                        }
                    }
                    else if (xmlIn.name()=="langSet"
                            &&(xmlIn.attributes().value("xml:lang")=="en"
                              ||xmlIn.attributes().value("xml:lang")==Project::instance()->langCode()))
                    {
                        while (! (xmlIn.readNext()==QXmlStreamReader::EndElement
                                   &&xmlIn.name()=="langSet") )
                            ;
//                                 kWarning() << "text  "<< xmlIn.text().toString()<< endl;;
//                         xmlIn.readNext();
//                         continue;
                    }
//                     else
//                         kWarning() << "text  "<< xmlIn.attributes().value("xml:lang").toString()<< endl;;
                }
//                 kWarning() << "ff  "<< xmlIn.tokenString()<< endl;
                if (xmlIn.readNext()==QXmlStreamReader::EndElement
                         &&xmlIn.name()=="termEntry")
                    break;
                xmlOut.writeCurrentToken(xmlIn);
//             }
            } while(!(   xmlIn.readNext()==QXmlStreamReader::EndElement
                         &&xmlIn.name()=="termEntry"   ));
//             xmlOut.wri   teCurrentToken(xmlIn);
            int i=0;
            for (;i<entry.english.size();++i)
            {
                if (entry.english.at(i).isEmpty())
                    continue;

                xmlOut.writeStartElement("langSet");
                //xmlOut.writeAttribute("xml","lang","en");
                xmlOut.writeAttribute("xml:lang","en");
                xmlOut.writeStartElement("ntig");
                xmlOut.writeStartElement("termGrp");
                xmlOut.writeTextElement("term",entry.english.at(i));
                xmlOut.writeEndElement();
                xmlOut.writeEndElement();
                xmlOut.writeEndElement();
            }
            for (i=0;i<entry.target.size();++i)
            {
                if (entry.target.at(i).isEmpty())
                    continue;
                xmlOut.writeStartElement("langSet");
                //xmlOut.writeAttribute("xml","lang",langCode());
                xmlOut.writeAttribute("xml:lang",Project::instance()->langCode());
                xmlOut.writeStartElement("ntig");
                xmlOut.writeStartElement("termGrp");
                xmlOut.writeTextElement("term",entry.target.at(i));
                xmlOut.writeEndElement();
                xmlOut.writeEndElement();
                xmlOut.writeEndElement();
            }

//             xmlIn.readNext();
        }

        xmlOut.writeCurrentToken(xmlIn);
    }

    if (!xmlIn.hasError())
    {
        in.close();
        if (!in.open(QFile::WriteOnly | QFile::Text))
            return;
        //HACK 
        out.replace("\n\n\n","\n");
        out.replace("\n\n","\n");
        out.replace("\n            <langSet","\n                <langSet");
        in.write(out);
    }

}
