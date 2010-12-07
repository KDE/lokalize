/* **************************************************************************
  This file is part of Lokalize

  wordDiff algorithm adoption and further refinement:
        2007 (C) Nick Shaforostoff <shafff@ukr.net>
  (based on Markus Stengel's GPL implementation of LCS-Delta algorithm as it is described in "Introduction to Algorithms", MIT Press, 2001, Second Edition, written by Thomas H. Cormen et. al. It uses dynamic programming to solve the Longest Common Subsequence (LCS) problem. - http://www.markusstengel.de/text/en/i_4_1_5_3.html)

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

************************************************************************** */

#include "diff.h"

// #include "project.h"
#include "prefs_lokalize.h"

#include <QVector>
#include <QStringList>
#include <QStringMatcher>
#include <QStringBuilder>
#include <QLinkedList>

#include <kdebug.h>


typedef enum
{
    NOTHING       = 0,
    ARROW_UP      = 1,
    ARROW_LEFT    = 2,
    ARROW_UP_LEFT = 3,
    FINAL         = 4
} LCSMarker;

static QString addMarkerStart="<KBABELADD>";
static QString addMarkerEnd="</KBABELADD>";
static QString delMarkerStart="<KBABELDEL>";
static QString delMarkerEnd="</KBABELDEL>";

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
        inline QStringList operator()();

    private:
        QStringList s1, s2;
        QLinkedList<QString> resultString;
        QStringList s1Space, s2Space;
        QStringList::const_iterator it1, it2;
        QStringList::const_iterator it1Space, it2Space;
        uint nT:31;//we're using 1d vector as 2d
        bool haveSpaces:1;//"word: sfdfs" space is ": "
        QVector<LCSMarker> *b;
        //QStringList::iterator it1Space, it2Space;
};

inline
QStringList LCSprinter::operator()()
{
    QStringList result;
    foreach(const QString& str, resultString)
        result<<str;

    return result;
}


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
    , s1Space(s1Space_)
    , s2Space(s2Space_)
    , it1(s1.constBegin())
    , it2(s2.constBegin())
    , it1Space(s1Space.constBegin())
    , it2Space(s2Space.constBegin())
    , nT(nT_)
    , b(b_)

{
    haveSpaces=!s1Space_.isEmpty();
    printLCS(index);
}


static QStringList prepareForInternalDiff(const QString& str)
{
    QStringList result;
    int i=str.size();
    while(--i>=0)
        result.prepend(QString(str.at(i)));
    result.prepend(QString());
    return result;
}

void LCSprinter::printLCS(uint index)
{
    //fprintf(stderr,"%2d. %2d. %2d. %2d\n",(uint)(*b)[index],nT,index%nT, index);
    if (index % nT == 0 || index < nT)
    {
    // original LCS algo does not have to deal with ins before first common
        uint bound = index%nT;
        for (index=0; index<bound; ++index)
        {
            resultString.append(addMarkerStart);
            resultString.append(*it2);
            ++it2;
            if (haveSpaces)
            {
                resultString.append(*it2Space);
                ++it2Space;
            }
            resultString.append(addMarkerEnd);
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
                if((*it1)==(*it2))//case and accels
                    resultString.append(*it1);
                else
                {
                    QStringList word1=prepareForInternalDiff(*it1);
                    QStringList word2=prepareForInternalDiff(*it2);

                    QStringList empty;
                    resultString.append(calcLCS(word1,word2,empty,empty).join(QString()));
                }

                if((*it1Space)==(*it2Space))
                    resultString.append(*it1Space);
                else
                {
                    QStringList word1=prepareForInternalDiff(*it1Space);
                    QStringList word2=prepareForInternalDiff(*it2Space);

                    QStringList empty;
                    //empty=calcLCS(word1,word2,empty,empty);
//???this is not really good if we use diff result in autosubst

                    empty=calcLCS(word2,word1,empty,empty);
                    empty.replaceInStrings("KBABELADD>","KBABELTMP>");
                    empty.replaceInStrings("KBABELDEL>","KBABELADD>");
                    empty.replaceInStrings("KBABELTMP>","KBABELDEL>");

                    resultString.append(empty.join(QString()));
                }
                ++it1Space;
                ++it2Space;
                //kWarning() << " common " << *it1;
            }
            else
                resultString.append(*it1);//we may guess that this is a batch job, i.e. TM search
            ++it1;
            ++it2;
        }
    }
    else if (ARROW_UP == b->at(index))
    {
        printLCS(index-nT);
//         if (it1!=s1.end())
        {
            //kWarning()<<"APPENDDEL "<<*it1;
            //kWarning()<<"APPENDDEL "<<*it1Space;
            resultString.append(delMarkerStart);
            resultString.append(*it1);
            ++it1;
            if (haveSpaces)
            {
                resultString.append(*it1Space);
                ++it1Space;
            }
            resultString.append(delMarkerEnd);
        }
    }
    else
    {
        printLCS(index-1);
        resultString.append(addMarkerStart);
        resultString.append(*it2);
        ++it2;
        if (haveSpaces)
        {
            //kWarning() << "add2 " << *it2;
            resultString.append(*it2Space);
            ++it2Space;
        }
        resultString.append(addMarkerEnd);
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

    return printer();

}

QString wordDiff(QStringList s1, QStringList s2)
{
    static QString space(" ");
    s1.prepend(space);
    s2.prepend(space);

    static QStringList empty;
    QStringList list=calcLCS(s1,s2,empty,empty);
    bool r=list.first()==" ";
    if (r)
        list.removeFirst();
    else
        qDebug()<<"first ' ' assumption is wrong"<<list.first();

    QString result=list.join(QString());


    if (!r)
        result.remove(0,1);
    result.remove("</KBABELADD><KBABELADD>");
    result.remove("</KBABELDEL><KBABELDEL>");

    return result;
}


//this also separates punctuation marks etc from words as _only_ they may have changed
static void prepareLists(QString str, QStringList& main, QStringList& space, const QString& accel, QString markup)
{
    Q_UNUSED(accel);
    int pos=0;

    //accels are only removed by batch jobs
    //and this is not the one
#if 0
    QRegExp rxAccelInWord("[^\\W|\\d]"+accel+"[^\\W|\\d]");
    int accelLen=accel.size();
    while ((pos=rxAccelInWord.indexIn(str,pos))!=-1)
    {
        str.remove(rxAccelInWord.pos()+1,accelLen);
        pos+=2;//two letters
    }
#endif

    //QRegExp rxSplit("\\W+|\\d+");
    //i tried that but it failed:
    if (!markup.isEmpty())
        markup+='|';
    QRegExp rxSplit('('%markup%"\\W+|\\d+)+");

    main=str.split(rxSplit,QString::SkipEmptyParts);
    main.prepend("\t");//little hack


    //ensure the string always begins with the space part
    str.prepend('\b');
    pos=0;
    while ((pos=rxSplit.indexIn(str,pos))!=-1)
    {
        space.append(rxSplit.cap(0));
        pos+=rxSplit.matchedLength();
    }
    space.append(QString());//so we don't have to worry about list boundaries
    space.append(QString());//so we don't have to worry about list boundaries
}

QString userVisibleWordDiff(const QString& str1ForMatching,
                            const QString& str2ForMatching,
                            const QString& accel,
                            const QString& markup,
                            int options)
{
    QStringList s1, s2;
    QStringList s1Space, s2Space;


    prepareLists(str1ForMatching, s1, s1Space, accel, markup);
    prepareLists(str2ForMatching, s2, s2Space, accel, markup);

    //QRegExp rxSpace("[^(\\W+|\\d+)]");
    //i tried that but it failed:
    //QRegExp rxSpace("[^("+Project::instance()->markup()+"|\\W+|\\d+)]");
    //QStringList s1Space(str1ForMatching.split(rxSpace,QString::SkipEmptyParts));
    //QStringList s2Space(str2ForMatching.split(rxSpace,QString::SkipEmptyParts));


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

    if (options&Html)
    {
        result.replaceInStrings("&","&amp;");
        result.replaceInStrings("<","&lt;");
        result.replaceInStrings(">","&gt;");
    }

    //result.last().chop(1);//\b
    //kWarning()<<"DIFF RESULT '" <<result<<"' '"<<result<<"'";

    QString res(result.join(QString()));
    res.remove("{/KBABELADD}{KBABELADD}");
    res.remove("{/KBABELDEL}{KBABELDEL}");

    if (options&Html)
    {
        res.replace("{KBABELADD}","<font style=\"background-color:"%Settings::addColor().name()%";color:black\">");
        res.replace("{/KBABELADD}","</font>");
        res.replace("{KBABELDEL}","<font style=\"background-color:"%Settings::delColor().name()%";color:black\">");
        res.replace("{/KBABELDEL}","</font>");
        res.replace("\\n","\\n<br>");
    }

    return res;
}

