/*****************************************************************************
  This file is part of KAider

  Copyright (C) 2007	  by Nick Shaforostoff <shafff@ukr.net>

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

#include "project.h"
#include "projectmodel.h"

#include "tbxparser.h"

#include <QXmlSimpleReader>
#include <QXmlStreamReader>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <kurl.h>
#include <kdirlister.h>

// // // #include <kdialog.h>
#include <kross/core/action.h>
 #include <kross/core/actioncollection.h>
 #include <kross/core/manager.h>
 // // #include <kross/ui/view.h>
#include "webquerycontroller.h"
// #include "webquerythread.h"

using namespace Kross;

Project* Project::_instance=0;

Project* Project::instance()
{
    if (_instance==0)
        _instance=new Project();

    return _instance;
}

Project::Project(/*const QString &file*/)
    : ProjectBase()
    , m_model(0)
    , m_glossary(new Glossary)
//     , m_webQueryThread(new WebQueryThread)
{
//     m_webQueryThread->start();
}

Project::~Project()
{
    delete m_model;
    delete m_glossary;
//     delete m_webQueryThread;
    kWarning() << "--d "<< m_path << endl;
    writeConfig();
}

// void Project::save()
// {
// //     kWarning() << "--s "<< m_path << endl;
// //     setSharedConfig(KSharedConfig::openConfig(m_path, KConfig::NoGlobals));
// // 
// //     kWarning() << "--s "<< potBaseDir() << " " << poBaseDir()<< endl;
// //     QString aa(potBaseDir());
// //     readConfig();
// //     setPotBaseDir(aa);
//     writeConfig();
// }


void Project::load(const QString &file)
{
    setSharedConfig(KSharedConfig::openConfig(file, KConfig::NoGlobals));
    readConfig();
    m_path=file;
//     kWarning() << "--l "<< m_path << endl;
//put 'em into thread?
    QTimer::singleShot(500,this,SLOT(populateDirModel()));
    QTimer::singleShot(400,this,SLOT(populateGlossary()));
    QTimer::singleShot(1000,this,SLOT(populateWebQueryActions()));
    //void populateWebQueryActions(QStringList paths);
//     m_webQueryThread.populateWebQueryActions();//(webQueryScripts());
/*    QMetaObject::invokeMethod(m_webQueryThread,
                              SLOT(populateWebQueryActions(QStringList)),
                                   Qt::BlockingQueuedConnection,
                              Q_ARG(QStringList,webQueryScripts())
                                 );*/
/*    QStringList ssssssss(webQueryScripts());
        connect(this,SIGNAL(populateWebQueryActions(QString)),
               m_webQueryThread,SLOT(populateWebQueryActions(QString)),Qt::QueuedConnection);
        emit populateWebQueryActions(ssssssss.first());
        disconnect(this,SIGNAL(populateWebQueryActions(QString)),
               m_webQueryThread,SLOT(populateWebQueryActions(QString)));
    kWarning()<<"loa "<<ssssssss.size()<<endl;*/
//     populateKrossActions();
    emit loaded();
}

QString Project::projectDir() const
{
    return KUrl(m_path).directory();
}

QStringList Project::webQueryScripts() const
{
    QStringList actionz(ProjectBase::webQueryScripts());
//     kWarning()<<"dd "<<actionz.size()<<endl;
    int i=0;
    for (;i<actionz.size();++i)
        actionz[i]=absolutePath(actionz.at(i));

//     kWarning()<<actionz.size()<<endl;
    return actionz;
}

void Project::populateWebQueryActions()
{
    QStringList a(webQueryScripts());
    int i=0;
    while(i<a.size())
    {
        WebQueryController* webQueryController=new WebQueryController(this);
                    //Manager::self().addObject(m_webQueryController, "WebQueryController",ChildrenInterface::AutoConnectSignals);
        Action* action = new Action(this,QUrl(a.at(i)));
        action->addObject(webQueryController, "WebQueryController",ChildrenInterface::AutoConnectSignals);
        Manager::self().actionCollection()->addAction(action);
        action->trigger();
        ++i;
    }

}


QString Project::absolutePath(const QString& possiblyRelPath) const
{
    if (KUrl::isRelativeUrl(possiblyRelPath))
    {
        KUrl url(m_path);
        url.setFileName(QString());
//         kWarning () << "1  " << url.path() << endl;
        //url.addPath(possiblyRelPath);
        url.cd(possiblyRelPath);
        //url.cleanPath();
//         kWarning () << "2  " << possiblyRelPath << " + "  << url.path() << endl;
        return url.path();
    }
    return possiblyRelPath;
}

void Project::populateDirModel()
{
    if (!m_model || m_path.isEmpty())
        return;

/*    KUrl url(m_path);
    url.setFileName(QString());
    url.cd(poBaseDir());

    if (QFile::exists(url.path()))*/
    QString a(poDir());
    if (QFile::exists(a))
    {
        m_model->dirLister()->openUrl(a);
    }
}
#if 0
void Project::populateKrossActions()
{

    m_webQueryController=new WebQueryController(this);
    Manager::self().addObject(m_webQueryController, "WebQueryController",ChildrenInterface::AutoConnectSignals);
    //Action* action = new Action(this,"ss"/*,"/home/kde-devel/test.js"*/);
    Action* action = new Action(this,QUrl("/home/kde-devel/t.js"));
    Manager::self().actionCollection()->addAction(action);
    action->trigger();
    action->setEnabled(false);

    action = new Action(this,QUrl("/home/kde-devel/tt.js"));
    Manager::self().actionCollection()->addAction(action);

//     action->setInterpreter("javascript");
//     action->setFile("/home/kde-devel/t.js");
/* # Publish a QObject instance only for the Kross::Action instance.
 action->addChild(myqobject2, "MySecondQObject");*/
/* # Set the script file we like to execute.
 action->setFile("/home/myuser/mytest.py");
 # Execute the script file.*/
//      action->setInterpreter("python");

//  action->setCode("println( \"interval=\" );");
    //action->trigger();
//     QVariant result = action->callFunction("init", QVariantList()<<QString("Arg"));
//     kWarning() <<result << action->functionNames()<<endl;
// 
//     KDialog d;
//     new ActionCollectionEditor(action, d.mainWidget());
//     d.exec();
    //m_webQueryController->query();

}
#endif
void Project::populateGlossary()
{
    m_glossary->clear();

    TbxParser parser(m_glossary);
    QXmlSimpleReader reader;
    reader.setContentHandler(&parser);

    QFile file(glossaryPath());
    if (!file.open(QFile::ReadOnly | QFile::Text))
         return;
    QXmlInputSource xmlInputSource(&file);
    if (!reader.parse(xmlInputSource))
    {
         kWarning() << "failed to load "<< glossaryPath()<< endl;
    }
//     glossaryChange();

}

/////////////////////////////at most 1 tag pair on the line
//int writeLine(int indent,const QString& str,QFile& out)
int writeLine(int indent,const QByteArray& str,QFile& out)
{
    kWarning() << str << /*" a1 "<< indent<< */endl;
    int balance=str.count('<')-str.count("</")*2;
    if (balance<0)
        indent+=balance;

    out.write(QByteArray(indent*4,' ')+str+'\n');

    if (balance>0)
        indent+=balance;


//kWarning() << "a3 "<< indent<< endl;

    return indent;
}
void Project::glossaryAdd(const TermEntry& entry)
{
    QFile stream(glossaryPath());
    if (!stream.open(QFile::ReadWrite | QFile::Text))
         return;

//     QTextStream stream(&file);
//     stream.setCodec("UTF-8");
//     stream.setAutoDetectUnicode(true);
    QByteArray line(stream.readLine());
    int id=0;
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
        xmlOut.writeCharacters(glossary()->subjectFields.at(entry.subjectField));
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
        xmlOut.writeAttribute("xml:lang",langCode());
        xmlOut.writeStartElement("ntig");
        xmlOut.writeStartElement("termGrp");
        xmlOut.writeTextElement("term",entry.target.at(i));
        xmlOut.writeEndElement();
        xmlOut.writeEndElement();
        xmlOut.writeEndElement();
    }
    xmlOut.writeEndElement();
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

void Project::glossaryChange(const TermEntry& entry)
{
    QFile in(glossaryPath());
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
                xmlOut.writeCharacters(glossary()->subjectFields.at(entry.subjectField));
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
                              ||xmlIn.attributes().value("xml:lang")==langCode()))
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
                xmlOut.writeAttribute("xml:lang",langCode());
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

ProjectModel* Project::model()
{
    if (!m_model)
    {
        m_model=new ProjectModel;
        QTimer::singleShot(500,this,SLOT(populateDirModel()));
    }

    return m_model;

}

/*
Project::Project(const QString &file)
    : ProjectBase(KSharedConfig::openConfig(file, KConfig::NoGlobals))
{
    readConfig();
}
*/
// Project::~Project()
// {
// }



#include "project.moc"

