/* **************************************************************************
  This file is part of KAider

  wordDiff algorithm adoption (C) Nick Shaforostoff <shafff@ukr.net>
  (based on Markus Stengel's GPL implementation of LCS-Delta algorithm as it is described in "Introduction to Algorithms", MIT Press, 2001, Second Edition, written by Thomas H. Cormen et. al.
  It uses dynamic programming to solve the Longest Common Subsequence (LCS) problem. - http://www.markusstengel.de/text/en/i_4_1_5_3.html)

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

************************************************************************** */

// #include "diff.h"

// #include "project.h"
#include "prefs_kaider.h"

#include <QVector>
#include <QStringList>
#include <QStringMatcher>

#include <kdebug.h>

typedef enum
{
    NOTHING       = 0,
    ARROW_UP      = 1,
    ARROW_LEFT    = 2,
    ARROW_UP_LEFT = 3,
    FINAL         = 4
} LCSMarker;

QStringList calcLCS(const QStringList& s1Words,
                    const QStringList& s2Words,
                    const QStringList& s1Space,
                    const QStringList& s2Space
                   );


/**
     * The class is used for keeping "global" params of recursive function
     *
     * @short Class for keeping "global" params of recursive function
     * @author Nick Shaforostoff <shafff@ukr.net>
 */
    class LCSprinter
{
    public:
        LCSprinter(const QStringList &s_1,
                   const QStringList& s_2,
                   QVector<LCSMarker> *b_,
                   const uint nT_,
                   uint index,
                   const QStringList& s1Space_,
                   const QStringList& s2Space_
                  );
        ~LCSprinter() {};
        void printLCS(uint index);
        inline const QStringList& getResult();

    private:
        QStringList s1, s2, resultString;
        QStringList s1Space, s2Space;
        uint nT:31;//we're using 1d vector as 2d
        bool haveSpaces:1;//"word: sfdfs" space is ": "
        QVector<LCSMarker> *b;
        QStringList::const_iterator it1, it2;
        QStringList::const_iterator it1Space, it2Space;
        //QStringList::iterator it1Space, it2Space;
};

inline
const QStringList& LCSprinter::getResult()
{
    return resultString;
}

// inline
// QString LCSprinter::getString()
// {
//     return resultString.join("");
// }

inline
LCSprinter::LCSprinter(const QStringList& s_1,
                       const QStringList& s_2,
                       QVector<LCSMarker> *b_,
                       const uint nT_,
                       uint index,
                       const QStringList& s1Space_,
                       const QStringList& s2Space_
                      )
    : s1(s_1)
    , s2(s_2)
    , nT(nT_)
    , b(b_)
{
    if (!s1Space_.isEmpty())
    {
        haveSpaces=true;
        s1Space=s1Space_;
        s2Space=s2Space_;

        it1Space=s1Space.constBegin();
        it2Space=s2Space.constBegin();
    }
    else
        haveSpaces=false;

    it1=s1.constBegin();
    it2=s2.constBegin();
    printLCS(index);
}


void LCSprinter::printLCS(uint index)
{
    //fprintf(stderr,"%2d. %2d. %2d. %2d\n",(uint)(*b)[index],nT,index%nT, index);
    if (index % nT == 0 || index < nT)
    {
    //original LCS algo doesnt have to deal with ins before first common
        uint bound = index%nT;
        for (index=0; index<bound; ++index)
        {
            resultString.append("<KBABELADD>");
//            kDebug() << "add-------- ";
            resultString.append(*it2);
            ++it2;
            if (haveSpaces)
            {
//                 kWarning() << "add1 " << *it2;
                resultString.append(*it2Space);
                ++it2Space;
//                 kWarning() << " add1 " << *it2;
            }
            resultString.append("</KBABELADD>");
        }

        return;
    }

    if (ARROW_UP_LEFT == b->at(index))
    {
        printLCS(index-nT-1);
        if (it1!=s1.constEnd())
        {
            //kWarning() << "upleft '" << *it1 <<"'";
            //kWarning() << "upleft 1s" << *it1Space;
            //kWarning() << "upleft 2s" << *it2Space;
            if (haveSpaces)
            {
//                     kWarning()<<"!!!!!! '"<<*it1<<"' '"<<*it2;
                if((*it1)==(*it2))//case and accels
                    resultString.append(*it1);
                else
                {
                    QStringList word1;
                    QStringList word2;
                    int i=it1->size();
                    while(--i>=0)
                        word1.prepend(QString(it1->at(i)));
                    word1.prepend("");
                    i=it2->size();
                    while(--i>=0)
                        word2.prepend(QString(it2->at(i)));
                    word2.prepend("");

                    QStringList empty;
                    resultString.append(calcLCS(word1,word2,empty,empty).join(""));
                }

//                 kWarning() << "common " << *it1;
                if((*it1Space)==(*it2Space))
                    resultString.append(*it1Space);
                else
                {
                    QStringList word1;
                    QStringList word2;
                    int i=it1Space->size();
                    while(--i>=0)
                        word1.prepend(QString(it1Space->at(i)));
                    word1.prepend("");
                    i=it2Space->size();
                    while(--i>=0)
                        word2.prepend(QString(it2Space->at(i)));
                    word2.prepend("");

                    QStringList empty;
//                     empty=calcLCS(word1,word2,empty,empty);
//???this is not really good if we use diff result in autosubst
                    empty=calcLCS(word2,word1,empty,empty);
                    empty.replaceInStrings("KBABELADD>","KBABELTMP>");
                    empty.replaceInStrings("KBABELDEL>","KBABELADD>");
                    empty.replaceInStrings("KBABELTMP>","KBABELDEL>");
                    resultString.append(empty.join(""));
//                     kWarning()<<"!!!!!! '"<<*it1Space<<"' '"<<*it2Space<<"' '"<<resultString.last()<<"' '";

//                     resultString.append("<KBABELADD>");
//                     resultString.append(*it2Space);
//                     resultString.append("</KBABELADD>");
//                     resultString.append("<KBABELDEL>");
//                     resultString.append(*it1Space);
//                     resultString.append("</KBABELDEL>");
                }
                ++it1Space;
                ++it2Space;
//                 kWarning() << " common " << *it1;
            }
            else
                resultString.append(*it1);//we may guess that this is a batch job, i.e. TM search
            ++it1;
            ++it2;
        }
        return;
    }
    else if (ARROW_UP == b->at(index))
    {
        printLCS(index-nT);
//         if (it1!=s1.end())
        {
            //kWarning()<<"APPENDDEL "<<*it1;
            //kWarning()<<"APPENDDEL "<<*it1Space;
            resultString.append("<KBABELDEL>");
            resultString.append(*it1);
            ++it1;
            if (haveSpaces)
            {
//                 kWarning() << "del " << *it1;
                resultString.append(*it1Space);
                ++it1Space;
//                 kWarning() << " del " << *it1;
            }
            resultString.append("</KBABELDEL>");
        }
        return;
    }
    else
    {
        printLCS(index-1);
        //kWarning()<<"APPENDADD "<<*it2;
        //kWarning()<<"APPENDADD "<<*it2Space;
        resultString.append("<KBABELADD>");
        resultString.append(*it2);
        ++it2;
        if (haveSpaces)
        {
//             kWarning() << "add2 " << *it2;
            resultString.append(*it2Space);
            ++it2Space;
//             kWarning() << " add2 " << *it2;
        }
        resultString.append("</KBABELADD>");
        return;
    }

}




// calculate the LCS
QStringList calcLCS(const QStringList& s1Words,
                    const QStringList& s2Words,
                    const QStringList& s1Space,
                    const QStringList& s2Space
                   )
{

    uint i;
    uint j;

    uint mX = s1Words.count();
    uint nY = s2Words.count();

    //create lowered lists for matching,
    //and use original ones for printing (but only for non-batch)
    QStringList s1(s1Words);
    QStringList s2(s2Words);

    if (!s1Space.isEmpty())
    {
        //accels are only removed by batch jobs
        //and this is not the one
        //also, lower things a bit :)

        for (i=0;i<mX;++i)
            s1[i]=s1.at(i).toLower();
        for (i=0;i<nY;++i)
            s2[i]=s2.at(i).toLower();
#if 0 //i'm too lazy...
        QString accel(Project::instance()->accel());
        i=mX;
        while(--i>0)
        {
            if ((s1Space.at(i)==accel))
            {
                s1[i]+=s1[i+1];
                s1.removeAt(i+1);
                s1Space.removeAt(i);
                s1Words[i]+=s1[i+1];
                s1Words.removeAt(i+1);
                --mX;
                --nY;
            }
        }
#endif
    }


    uint mT = mX+1;
    uint nT = nY+1;

    QVector<LCSMarker> b(mT*nT, NOTHING);
    QVector<uint> c(mT*nT, 0);



    b[0] = FINAL;
    uint index_cache = 0;
    QStringList::const_iterator it1, it2;

    for (i=1, it1 = s1.constBegin(); i<mT; ++i, ++it1)
    {
        for (j=1, it2 = s2.constBegin(); j<nT; ++j, ++it2)
        {
            index_cache = i*nT+j;
            if ((*it1)==(*it2))
            {
                c[index_cache] = c.at(index_cache-nT-1) + 1;
                b[index_cache] = ARROW_UP_LEFT;
            }
            else if (c.at(index_cache-nT) >= c.at(index_cache-1))
            {
                c[index_cache] = c.at(index_cache-nT);
                b[index_cache] = ARROW_UP;
            }
            else
            {
                c[index_cache] = c.at(index_cache-1);
                b[index_cache] = ARROW_LEFT;
            }
        }
    }

    c.clear();

    LCSprinter printer(s1Words, s2Words, &b, nT, index_cache, s1Space, s2Space);

    return printer.getResult();

}

QString wordDiff(const QStringList& s1,const QStringList& s2)
{
    QStringList empty;
    empty=calcLCS(s1,s2,empty,empty);
    //return calcLCS(s1,s2,empty,empty).join("");
    return empty.join("");
}

QString wordDiff(const QString& str1,
                 const QString& str2,
                 const QString& accel,
                 const QString& markup)
{
    //separate punctuation marks etc from words as _only_ they may have changed
    QStringList s1("\t"), s2("\t");//little hack

    //accels are only removed by batch jobs
    //and this is not the one
    QString str1ForMatching(str1);
    QString str2ForMatching(str2);

    //move this to calcLCS?
    QRegExp rxAccelInWord("[^\\W|\\d]"+accel+"[^\\W|\\d]");
    int accelLen=accel.size();
    int pos=0;
    while ((pos=rxAccelInWord.indexIn(str1ForMatching,pos))!=-1)
    {
        str1ForMatching.remove(rxAccelInWord.pos()+1,accelLen);
        pos+=2;//two letters
    }
    pos=0;
    while ((pos=rxAccelInWord.indexIn(str2ForMatching,pos))!=-1)
    {
        str2ForMatching.remove(rxAccelInWord.pos()+1,accelLen);
        pos+=2;//two letters
    }

    //QRegExp rxSplit("\\W+|\\d+");
    //i tried that but it failed:
    QRegExp rxSplit('('+markup+"|\\W+|\\d+)+");

    s1+=str1ForMatching.split(rxSplit,QString::SkipEmptyParts);
    s2+=str2ForMatching.split(rxSplit,QString::SkipEmptyParts);

    //QRegExp rxSpace("[^(\\W+|\\d+)]");
    //i tried that but it failed:
    //QRegExp rxSpace("[^("+Project::instance()->markup()+"|\\W+|\\d+)]");
    //QStringList s1Space(str1ForMatching.split(rxSpace,QString::SkipEmptyParts));
    //QStringList s2Space(str2ForMatching.split(rxSpace,QString::SkipEmptyParts));

    //ensure the string always begins with the space part
    str1ForMatching.prepend('\b');
    str2ForMatching.prepend('\b');
    QStringList s1Space;
    QStringList s2Space;
    pos=0;
    while ((pos=rxSplit.indexIn(str1ForMatching,pos))!=-1)
    {
        s1Space.append(rxSplit.cap(0));
        pos+=rxSplit.matchedLength();
    }
    pos=0;
    while ((pos=rxSplit.indexIn(str2ForMatching,pos))!=-1)
    {
        s2Space.append(rxSplit.cap(0));
        pos+=rxSplit.matchedLength();
    }
    s1Space.append("");//so we don't have to worry about list boundaries
    s2Space.append("");
    s1Space.append("");//so we don't have to worry about list boundaries
    s2Space.append("");

    //return QString();
//     kWarning()<<endl<<endl<<"wordDiff 1 '" <<str1<<"' '"<<str2<<"'";
//     kWarning()<<s1.size()<<" "<<s2.size()<<" "<<s1Space.size()<<" "<<s2Space.size()<<" ";
    //kWarning()<<" '"<<s1Space.first()<<"' '"<<s2Space.first()<<"' ";
    QStringList result(calcLCS(s1,s2,s1Space,s2Space));
    result.removeFirst();//\t
    result.first().remove(0,1);//\b
//     kWarning()<<"wordDiff 1 '" <<result<<"'";
    result.replaceInStrings("<KBABELDEL></KBABELDEL>","");
    result.replaceInStrings("<KBABELADD></KBABELADD>","");

    result.replaceInStrings("<KBABELADD>","{KBABELADD}");
    result.replaceInStrings("</KBABELADD>","{/KBABELADD}");
    result.replaceInStrings("<KBABELDEL>","{KBABELDEL}");
    result.replaceInStrings("</KBABELDEL>","{/KBABELDEL}");

    result.replaceInStrings("<","&lt;");
    result.replaceInStrings(">","&gt;");

    result.replaceInStrings("{KBABELADD}","<font style=\"background-color:"+Settings::addColor().name()+"\">");
    result.replaceInStrings("{/KBABELADD}","</font>");
    result.replaceInStrings("{KBABELDEL}","<font style=\"background-color:"+Settings::delColor().name()+"\">");
    result.replaceInStrings("{/KBABELDEL}","</font>");

    //result.last().chop(1);//\b
    //kWarning()<<"DIFF RESULT '" <<result<<"' '"<<result<<"'";


//     result.remove("</KBABELADD><KBABELADD>");
//     result.remove("</KBABELDEL><KBABELDEL>");

//     QStringMatcher addMatcher("</KBABELADD>");
//     int pos=0;//TODO beginning
//     while ((pos=addMatcher.indexIn(result,pos))!=-1)
//     {
//         msg.remove(accel.pos(1),accel.cap(1).size());
//         pos=accel.pos(1);
//         kWarning()<<endl<<endl<<"valvalvalvalval " <<msg<<endl;
//     }


    QString res(result.join(""));
    res.remove("</KBABELADD><KBABELADD>");
    res.remove("</KBABELDEL><KBABELDEL>");

    return result.join("");

}



