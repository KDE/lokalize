/* This file is part of the KDE project
 * Copyright (C) 2007 Montel Laurent <montel@kde.org>
 * Copyright (C) 2007 Nick Shaforostoff <shafff@ukr.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include <QFile>
#include <QByteArray>
#include <QRegExp>
#include <QString>
#include <QTime>

#define STRIGI_IMPORT_API
#include <strigi/streamendanalyzer.h>
#include <strigi/analyzerplugin.h>
#include <strigi/fieldtypes.h>
#include <strigi/analysisresult.h>

#include <iostream>

using namespace std;
using namespace Strigi;

class PoEndAnalyzerFactory;
class PoEndAnalyzer : public StreamEndAnalyzer {
    public:
        PoEndAnalyzer(const PoEndAnalyzerFactory* f):factory(f) {}
        const char* name() const {return "PoEndAnalyzer";}
        bool checkHeader(const char* header, int32_t headersize) const;
        char analyze(Strigi::AnalysisResult& idx,Strigi::InputStream* in);
    private:
        const PoEndAnalyzerFactory* factory;
};


class PoEndAnalyzerFactory : public StreamEndAnalyzerFactory {
friend class PoEndAnalyzer;
private:
    static const std::string messagesFieldName;
    static const std::string translatedFieldName;
    static const std::string untranslatedFieldName;
    static const std::string fuzzyFieldName;
    static const std::string lastTranslatorFieldName;
    static const std::string poRevisionDateFieldName;
    static const std::string potCreationDateFieldName;

    const RegisteredField* messagesField;
    const RegisteredField* translatedField;
    const RegisteredField* untranslatedField;
    const RegisteredField* fuzzyField;
    const RegisteredField* lastTranslatorField;
    const RegisteredField* poRevisionDateField;
    const RegisteredField* potCreationDateField;

    const char* name() const {return "PoEndAnalyzer";}
    StreamEndAnalyzer* newInstance() const {return new PoEndAnalyzer(this);}
    void registerFields(FieldRegister&);
};

const std::string PoEndAnalyzerFactory::messagesFieldName( "translation.total" );
const std::string PoEndAnalyzerFactory::translatedFieldName( "translation.translated");
const std::string PoEndAnalyzerFactory::untranslatedFieldName( "translation.untranslated");
const std::string PoEndAnalyzerFactory::fuzzyFieldName( "translation.fuzzy");
const std::string PoEndAnalyzerFactory::lastTranslatorFieldName("translation.last_translator");
const std::string PoEndAnalyzerFactory::poRevisionDateFieldName("translation.translation_date");
const std::string PoEndAnalyzerFactory::potCreationDateFieldName("translation.source_date");


void PoEndAnalyzerFactory::registerFields( FieldRegister& reg ) {
	messagesField = reg.registerField( messagesFieldName, FieldRegister::integerType, 1, 0 );
	translatedField = reg.registerField( translatedFieldName, FieldRegister::integerType, 1, 0 );
	untranslatedField = reg.registerField(untranslatedFieldName, FieldRegister::integerType, 1, 0 );
        fuzzyField = reg.registerField(fuzzyFieldName, FieldRegister::integerType, 1, 0 );
//	obsoleteField = reg.registerField(obsoleteFieldName, FieldRegister::stringType, 1, 0 );
	lastTranslatorField = reg.registerField(lastTranslatorFieldName, FieldRegister::stringType, 1, 0 );
	poRevisionDateField = reg.registerField(poRevisionDateFieldName, FieldRegister::stringType/*datetimeType*/, 1, 0 );
	potCreationDateField = reg.registerField(potCreationDateFieldName, FieldRegister::stringType/*datetimeType*/, 1, 0 );
}

bool PoEndAnalyzer::checkHeader(const char* header, int32_t headersize) const
{
    //I guess, this is sufficient
    QByteArray data(QByteArray::fromRawData(header,headersize));
    if (!data.contains("\nmsgid \"\"\n") || !data.contains("PO-Revision-Date"))
        return false;

    return true;

}


char PoEndAnalyzer::analyze(AnalysisResult& idx, InputStream* in)
{
    if (idx.extension()=="svn-base")
        return Ok;

    QTime time;time.start();
    const char* array;
    int32_t n = in->read(array, in->size(), in->size());
    QByteArray tmp(QByteArray::fromRawData(array,n));
    QByteArray data(QByteArray::fromRawData(array,tmp.lastIndexOf('\n')));


    const char* a =idx.path().c_str();
    a+=7;

//     bool debug=QString(a).endsWith("kstars.po");
//     cout<<"-----+----- "<<n<<" "<<debug<<endl;
    QFile f(a);
    if (in->size()==-1)
    {

        if (!f.open(QIODevice::ReadOnly))
            return Error;

        data = f.readAll()+'\n';
    }
    else
        data+="\n\n\n";


    QString str=QString::fromUtf8(data.left(1024).data(),1024);
    QRegExp rx("\\n\"POT-Creation-Date: ([^\\\\]*)\\\\n");
    int cursor=rx.indexIn(str);
    if (cursor==-1)
        return Error;
    QString potCreationDate(rx.cap(1));

    rx.setPattern("\\n\"PO-Revision-Date: ([^\\\\]*)\\\\n");
    cursor=rx.indexIn(str,cursor);
    if (cursor==-1)
        return Error;
    QString poRevisionDate(rx.cap(1));

    rx.setPattern("\\n\"Last-Translator: ([^\\\\]*)\\\\n");
    cursor=rx.indexIn(str,cursor);
    if (cursor==-1)
        return Error;
    QString lastTranslator(rx.cap(1));


    int messages      = 0;
    int untranslated  = 0;
    int fuzzy         = 0;

/*
    QLatin1String msgstr("msgstr");
    QLatin1String "msgid \""("msgid \"");
    QChar '"'('"');
    QChar '\n'('\n');
*/
    QByteArray msg;

    bool possiblyUntranslated=false;
    bool possiblyFuzzy=false;
    bool plural=false;
    QByteArray line;
    cursor =data.indexOf('\n',cursor+1)+1;
    int end=data.indexOf('\n',cursor+1);
    while (end!=-1)
    {
        line=data.mid(cursor,end-cursor);

/*        if (debug)
            cout<<endl<<"line: "<<QString(line).toLocal8Bit().data()<<" ";*/
        if (possiblyUntranslated)
        {
            possiblyUntranslated=false;
            if (!line.startsWith('"'))
                ++untranslated;

            if (plural)
            {
                plural=false;
                //fast forward to the next entry
                do
                {
                    cursor=end+1;
                    end=data.indexOf('\n',cursor);
                    line=data.mid(cursor,end-cursor);
                } while (line.startsWith("msgstr")||line.startsWith('"'));
            }

//             if (debug)
//                 cout<<"++untranslated "/*<<endl*/;
        }

        if (possiblyFuzzy)
        {
            possiblyFuzzy=false;
            if (!line.startsWith("#~"))
                ++fuzzy;
        }



        if (line.startsWith("msgid \""))
        {
            ++messages;
/*            msg=line.mid(7).chop(1);
            idx.addText(msg.constData(),msg.length());*/
        }
        else if (line.startsWith("#,"))
        {
            if (line.contains("fuzzy"))
                possiblyFuzzy=true;
        }
        else if (line.startsWith("msgstr"))
        {
            const int& len=line.size();
            if (line.endsWith("\"\"")&&((len<3)||(line.at(len-3)!='\\')))
            {
                possiblyUntranslated=true;
                if (len>6&&line.at(6)=='[')
                    plural=true;
            }
        }
//         else if (debug)
//             cout<<"else"<<endl;

        if (line.endsWith('"'))
        {
            msg=line.mid(line.indexOf('"')+1);
            msg.chop(1);

            idx.addText(msg.constData(),msg.length());
        }


        cursor=end+1;
        end=data.indexOf('\n',cursor);

    }

    idx.addValue( factory->messagesField,messages);
    idx.addValue( factory->translatedField,(messages-untranslated-fuzzy));
    idx.addValue( factory->untranslatedField,untranslated);
    idx.addValue( factory->fuzzyField,fuzzy);

    idx.addValue( factory->lastTranslatorField,(const char*)lastTranslator.toUtf8());
    idx.addValue( factory->poRevisionDateField,(const char*)poRevisionDate.toUtf8());
    idx.addValue( factory->potCreationDateField,(const char*)potCreationDate.toUtf8());

//     if (QString(a).endsWith("kstars.po"))
//         cout<<"!!!"<<endl<<"elapsed: "<<time.elapsed()<<endl;

    //in->reset(0);
    //return in;
    return Ok;

}

class Factory : public AnalyzerFactoryFactory {
public:
    std::list<StreamEndAnalyzerFactory*>
    streamEndAnalyzerFactories() const {
        std::list<StreamEndAnalyzerFactory*> af;
        af.push_back(new PoEndAnalyzerFactory());
        return af;
    }
};



STRIGI_ANALYZER_FACTORY(Factory)


