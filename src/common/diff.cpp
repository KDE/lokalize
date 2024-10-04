/* **************************************************************************
  This file is part of Lokalize

  wordDiff algorithm adoption and further refinement:
  SPDX-FileCopyrightText: 2007 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>
  (based on Markus Stengel's GPL implementation of LCS-Delta algorithm as it is described in "Introduction to Algorithms", MIT Press, 2001, Second Edition, written by Thomas H. Cormen et. al. It uses dynamic programming to solve the Longest Common Subsequence (LCS) problem.)

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

************************************************************************** */

#include "diff.h"

#include "lokalize_debug.h"

// #include "project.h"
#include "prefs_lokalize.h"

#include <QRegularExpression>
#include <QVector>
#include <QStringList>
#include <QStringMatcher>
#include <QStringBuilder>

#include <list>


typedef enum {
    NOTHING       = 0,
    ARROW_UP      = 1,
    ARROW_LEFT    = 2,
    ARROW_UP_LEFT = 3,
    FINAL         = 4
} LCSMarker;

static const QString addMarkerStart = QStringLiteral("<KBABELADD>");
static const QString addMarkerEnd = QStringLiteral("</KBABELADD>");
static const QString delMarkerStart = QStringLiteral("<KBABELDEL>");
static const QString delMarkerEnd = QStringLiteral("</KBABELDEL>");

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
    ~LCSprinter() = default;

    void printLCS(uint index);
    inline QStringList operator()();

private:
    QStringList s1, s2;
    std::list<QString> resultString;
    QStringList s1Space, s2Space;
    QStringList::const_iterator it1, it2;
    QStringList::const_iterator it1Space, it2Space;
    uint nT: 31; //we're using 1d vector as 2d
    bool haveSpaces: 1; //"word: sfdfs" space is ": "
    QVector<LCSMarker> *b;
    //QStringList::iterator it1Space, it2Space;
};

inline
QStringList LCSprinter::operator()()
{
    QStringList result;
    for (const QString& str : resultString)
        result << str;

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
    haveSpaces = !s1Space_.isEmpty();
    printLCS(index);
}


static QStringList prepareForInternalDiff(const QString& str)
{
    QStringList result;
    int i = str.size();
    while (--i >= 0)
        result.prepend(QString(str.at(i)));
    result.prepend(QString());
    return result;
}

void LCSprinter::printLCS(uint index)
{
    //fprintf(stderr,"%2d. %2d. %2d. %2d\n",(uint)(*b)[index],nT,index%nT, index);
    if (index % nT == 0 || index < nT) {
        // original LCS algo does not have to deal with ins before first common
        uint bound = index % nT;
        for (index = 0; index < bound; ++index) {
            resultString.push_back(addMarkerStart);
            resultString.push_back(*it2);
            ++it2;
            if (haveSpaces) {
                resultString.push_back(*it2Space);
                ++it2Space;
            }
            resultString.push_back(addMarkerEnd);
        }

        return;
    }

    if (ARROW_UP_LEFT == b->at(index)) {
        printLCS(index - nT - 1);
        if (it1 != s1.constEnd()) {
            //qCWarning(LOKALIZE_LOG) << "upleft '" << *it1 <<"'";
            //qCWarning(LOKALIZE_LOG) << "upleft 1s" << *it1Space;
            //qCWarning(LOKALIZE_LOG) << "upleft 2s" << *it2Space;
            if (haveSpaces) {
                if ((*it1) == (*it2)) //case and accels
                    resultString.push_back(*it1);
                else {
                    QStringList word1 = prepareForInternalDiff(*it1);
                    QStringList word2 = prepareForInternalDiff(*it2);

                    QStringList empty;
                    resultString.push_back(calcLCS(word1, word2, empty, empty).join(QString()));
                }

                if ((*it1Space) == (*it2Space))
                    resultString.push_back(*it1Space);
                else {
                    QStringList word1 = prepareForInternalDiff(*it1Space);
                    QStringList word2 = prepareForInternalDiff(*it2Space);

                    QStringList empty;
                    //empty=calcLCS(word1,word2,empty,empty);
//???this is not really good if we use diff result in autosubst

                    empty = calcLCS(word2, word1, empty, empty);
                    empty.replaceInStrings(QStringLiteral("KBABELADD>"), QStringLiteral("KBABELTMP>"));
                    empty.replaceInStrings(QStringLiteral("KBABELDEL>"), QStringLiteral("KBABELADD>"));
                    empty.replaceInStrings(QStringLiteral("KBABELTMP>"), QStringLiteral("KBABELDEL>"));

                    resultString.push_back(empty.join(QString()));
                }
                ++it1Space;
                ++it2Space;
                //qCWarning(LOKALIZE_LOG) << " common " << *it1;
            } else
                resultString.push_back(*it1);//we may guess that this is a batch job, i.e. TM search
            ++it1;
            ++it2;
        }
    } else if (ARROW_UP == b->at(index)) {
        printLCS(index - nT);
//         if (it1!=s1.end())
        {
            //qCWarning(LOKALIZE_LOG)<<"APPENDDEL "<<*it1;
            //qCWarning(LOKALIZE_LOG)<<"APPENDDEL "<<*it1Space;
            resultString.push_back(delMarkerStart);
            resultString.push_back(*it1);
            ++it1;
            if (haveSpaces) {
                resultString.push_back(*it1Space);
                ++it1Space;
            }
            resultString.push_back(delMarkerEnd);
        }
    } else {
        printLCS(index - 1);
        resultString.push_back(addMarkerStart);
        resultString.push_back(*it2);
        ++it2;
        if (haveSpaces) {
            //qCWarning(LOKALIZE_LOG) << "add2 " << *it2;
            resultString.push_back(*it2Space);
            ++it2Space;
        }
        resultString.push_back(addMarkerEnd);
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

    if (!s1Space.isEmpty()) {
        //accels are only removed by batch jobs
        //and this is not the one
        //also, lower things a bit :)

        for (i = 0; i < mX; ++i)
            s1[i] = s1.at(i).toLower();
        for (i = 0; i < nY; ++i)
            s2[i] = s2.at(i).toLower();
#if 0 //i'm too lazy...
        QString accel(Project::instance()->accel());
        i = mX;
        while (--i > 0) {
            if ((s1Space.at(i) == accel)) {
                s1[i] += s1[i + 1];
                s1.removeAt(i + 1);
                s1Space.removeAt(i);
                s1Words[i] += s1[i + 1];
                s1Words.removeAt(i + 1);
                --mX;
                --nY;
            }
        }
#endif
    }


    uint mT = mX + 1;
    uint nT = nY + 1;

    QVector<LCSMarker> b(mT * nT, NOTHING);
    QVector<uint> c(mT * nT, 0);



    b[0] = FINAL;
    uint index_cache = 0;
    QStringList::const_iterator it1, it2;

    for (i = 1, it1 = s1.constBegin(); i < mT; ++i, ++it1) {
        for (j = 1, it2 = s2.constBegin(); j < nT; ++j, ++it2) {
            index_cache = i * nT + j;
            if ((*it1) == (*it2)) {
                c[index_cache] = c.at(index_cache - nT - 1) + 1;
                b[index_cache] = ARROW_UP_LEFT;
            } else if (c.at(index_cache - nT) >= c.at(index_cache - 1)) {
                c[index_cache] = c.at(index_cache - nT);
                b[index_cache] = ARROW_UP;
            } else {
                c[index_cache] = c.at(index_cache - 1);
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
    static const QString space(QStringLiteral(" "));
    s1.prepend(space);
    s2.prepend(space);

    static QStringList empty;
    QStringList list = calcLCS(s1, s2, empty, empty);
    bool r = list.first() == space;
    if (r)
        list.removeFirst();
    else
        qCDebug(LOKALIZE_LOG) << "first ' ' assumption is wrong" << list.first();

    QString result = list.join(QString());


    if (!r)
        result.remove(0, 1);
    result.remove(QStringLiteral("</KBABELADD><KBABELADD>"));
    result.remove(QStringLiteral("</KBABELDEL><KBABELDEL>"));

    return result;
}


//this also separates punctuation marks etc from words as _only_ they may have changed
static void prepareLists(QString str, QStringList& main, QStringList& space, const QString& accel, QString markup)
{
    Q_UNUSED(accel);
    int pos = 0;

    //accels are only removed by batch jobs
    //and this is not the one
#if 0
    QRegExp rxAccelInWord("[^\\W|\\d]" + accel + "[^\\W|\\d]");
    int accelLen = accel.size();
    while ((pos = rxAccelInWord.indexIn(str, pos)) != -1) {
        str.remove(rxAccelInWord.pos() + 1, accelLen);
        pos += 2; //two letters
    }
#endif

    //QRegExp rxSplit("\\W+|\\d+");
    //i tried that but it failed:
    if (!markup.isEmpty())
        markup += QLatin1Char('|');
    const QRegularExpression rxSplit(QLatin1String("(\x08?") + markup + QLatin1String("\\W+|\\d+)+"));

    main = str.split(rxSplit, Qt::SkipEmptyParts);
    main.prepend(QStringLiteral("\t"));//little hack


    //ensure the string always begins with the space part
    str.prepend(QStringLiteral("\b"));
    pos = 0;
    while (true) {
        const auto match = rxSplit.match(str, pos);
        if (!match.hasMatch()) {
            break;
        }
        pos = match.capturedStart();
        space.append(match.captured(0));
        pos += match.capturedLength();
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
    QString res;
    if (str1ForMatching.isEmpty() && str2ForMatching.isEmpty()) {
        return res;
    } else if (!str1ForMatching.isEmpty() && str2ForMatching.isEmpty()) {
        res = QLatin1String("{KBABELDEL}") + str1ForMatching + QLatin1String("{/KBABELDEL}");
    } else if (str1ForMatching.isEmpty() && !str2ForMatching.isEmpty()) {
        res = QLatin1String("{KBABELADD}") + str2ForMatching + QLatin1String("{/KBABELADD}");
    } else {
        QStringList s1, s2;
        QStringList s1Space, s2Space;

        prepareLists(str1ForMatching, s1, s1Space, accel, markup);
        prepareLists(str2ForMatching, s2, s2Space, accel, markup);

        QStringList result(calcLCS(s1, s2, s1Space, s2Space));
        result.removeFirst();//\t
        result.first().remove(0, 1); //\b
        result.replaceInStrings(QStringLiteral("<KBABELDEL></KBABELDEL>"), QString());
        result.replaceInStrings(QStringLiteral("<KBABELADD></KBABELADD>"), QString());

        result.replaceInStrings(QStringLiteral("<KBABELADD>"), QStringLiteral("{KBABELADD}"));
        result.replaceInStrings(QStringLiteral("</KBABELADD>"), QStringLiteral("{/KBABELADD}"));
        result.replaceInStrings(QStringLiteral("<KBABELDEL>"), QStringLiteral("{KBABELDEL}"));
        result.replaceInStrings(QStringLiteral("</KBABELDEL>"), QStringLiteral("{/KBABELDEL}"));

        if (options & Html) {
            result.replaceInStrings(QStringLiteral("&"), QStringLiteral("&amp;"));
            result.replaceInStrings(QStringLiteral("<"), QStringLiteral("&lt;"));
            result.replaceInStrings(QStringLiteral(">"), QStringLiteral("&gt;"));
        }
        res = result.join(QString());
        res.remove(QStringLiteral("{/KBABELADD}{KBABELADD}"));
        res.remove(QStringLiteral("{/KBABELDEL}{KBABELDEL}"));
    }

    if (options & Html) {
        res.replace(QLatin1String("{KBABELADD}"), QLatin1String("<font style=\"background-color:") + Settings::addColor().name() + QLatin1String(";color:black\">"));
        res.replace(QLatin1String("{/KBABELADD}"), QLatin1String("</font>"));
        res.replace(QLatin1String("{KBABELDEL}"), QLatin1String("<font style=\"background-color:") + Settings::delColor().name() + QLatin1String(";color:black\">"));
        res.replace(QLatin1String("{/KBABELDEL}"), QLatin1String("</font>"));
        res.replace(QLatin1String("\\n"), QLatin1String("\\n<br>"));
    }

    return res;
}

