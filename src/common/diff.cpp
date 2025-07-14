/*
  This file is part of Lokalize

  wordDiff algorithm adoption and further refinement:
  SPDX-FileCopyrightText: 2007 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>
  SPDX-FileCopyrightText: 2024 Finley Watson <fin-w@tutanota.com>
  (based on Markus Stengel's GPL implementation of LCS-Delta algorithm as it is described in "Introduction to Algorithms", MIT Press, 2001, Second Edition,
  written by Thomas H. Cormen et. al. It uses dynamic programming to solve the Longest Common Subsequence (LCS) problem.)

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "diff.h"
#include "lokalize_debug.h"

#include <list>

#include <KColorScheme>

#include <QRegularExpression>
#include <QStringBuilder>
#include <QStringList>
#include <QStringMatcher>
#include <QTextFormat>
#include <QVector>

typedef enum {
    NOTHING = 0,
    ARROW_UP = 1,
    ARROW_LEFT = 2,
    ARROW_UP_LEFT = 3,
    FINAL = 4,
} LCSMarker;

QStringList calcLCS(const QStringList &s1Words, const QStringList &s2Words, const QStringList &s1Space, const QStringList &s2Space);

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
               const QStringList &s_2,
               QVector<LCSMarker> *b_,
               const uint nT_,
               uint index,
               const QStringList &s1Space_,
               const QStringList &s2Space_);
    ~LCSprinter() = default;

    void printLCS(uint index);
    inline QStringList operator()();

private:
    QStringList s1, s2;
    std::list<QString> resultString;
    QStringList s1Space, s2Space;
    QStringList::const_iterator it1, it2;
    QStringList::const_iterator it1Space, it2Space;
    uint nT : 31; // we're using 1d vector as 2d
    bool haveSpaces : 1; //"word: sfdfs" space is ": "
    QVector<LCSMarker> *b;
};

inline QStringList LCSprinter::operator()()
{
    QStringList result;
    for (const QString &str : resultString)
        result << str;

    return result;
}

inline LCSprinter::LCSprinter(const QStringList &s_1,
                              const QStringList &s_2,
                              QVector<LCSMarker> *b_,
                              const uint nT_,
                              uint index,
                              const QStringList &s1Space_,
                              const QStringList &s2Space_)
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

static QStringList prepareForInternalDiff(const QString &str)
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
            if (haveSpaces) {
                if ((*it1) == (*it2)) // case and accels
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
                    //???this is not really good if we use diff result in autosubst

                    empty = calcLCS(word2, word1, empty, empty);
                    empty.replaceInStrings(addMarkerStart, QStringLiteral("{LokalizeTmp}"));
                    empty.replaceInStrings(addMarkerEnd, QStringLiteral("{/LokalizeTmp}"));
                    empty.replaceInStrings(delMarkerStart, addMarkerStart);
                    empty.replaceInStrings(delMarkerEnd, addMarkerEnd);
                    empty.replaceInStrings(QStringLiteral("{LokalizeTmp}"), delMarkerStart);
                    empty.replaceInStrings(QStringLiteral("{/LokalizeTmp}"), delMarkerEnd);

                    resultString.push_back(empty.join(QString()));
                }
                ++it1Space;
                ++it2Space;
            } else
                resultString.push_back(*it1); // we may guess that this is a batch job, i.e. TM search
            ++it1;
            ++it2;
        }
    } else if (ARROW_UP == b->at(index)) {
        printLCS(index - nT);
        resultString.push_back(delMarkerStart);
        resultString.push_back(*it1);
        ++it1;
        if (haveSpaces) {
            resultString.push_back(*it1Space);
            ++it1Space;
        }
        resultString.push_back(delMarkerEnd);
    } else {
        printLCS(index - 1);
        resultString.push_back(addMarkerStart);
        resultString.push_back(*it2);
        ++it2;
        if (haveSpaces) {
            resultString.push_back(*it2Space);
            ++it2Space;
        }
        resultString.push_back(addMarkerEnd);
    }
}

// calculate the LCS
QStringList calcLCS(const QStringList &s1Words, const QStringList &s2Words, const QStringList &s1Space, const QStringList &s2Space)
{
    uint i;
    uint j;

    uint mX = s1Words.count();
    uint nY = s2Words.count();

    // create lowered lists for matching,
    // and use original ones for printing (but only for non-batch)
    QStringList s1(s1Words);
    QStringList s2(s2Words);

    if (!s1Space.isEmpty()) {
        // accels are only removed by batch jobs
        // and this is not the one
        // also, lower things a bit :)

        for (i = 0; i < mX; ++i)
            s1[i] = s1.at(i).toLower();
        for (i = 0; i < nY; ++i)
            s2[i] = s2.at(i).toLower();
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
    result.remove(addMarkerEnd + addMarkerStart);
    result.remove(delMarkerEnd + delMarkerStart);

    return result;
}

// this also separates punctuation marks etc from words as _only_ they may have changed
static void prepareLists(QString str, QStringList &main, QStringList &space, const QString &accel, QString markup)
{
    Q_UNUSED(accel);
    int pos = 0;

    if (!markup.isEmpty())
        markup += QLatin1Char('|');
    const QRegularExpression rxSplit(QLatin1String("(\x08?") + markup + QLatin1String("\\W+|\\d+)+"), QRegularExpression::UseUnicodePropertiesOption);

    main = str.split(rxSplit, Qt::SkipEmptyParts);
    main.prepend(QStringLiteral("\t")); // little hack

    // ensure the string always begins with the space part
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
    space.append(QString()); // so we don't have to worry about list boundaries
    space.append(QString()); // so we don't have to worry about list boundaries
}

QString userVisibleWordDiff(const QString &str1ForMatching, const QString &str2ForMatching, const QString &accel, const QString &markup, int options)
{
    QString res;
    if (str1ForMatching.isEmpty() && str2ForMatching.isEmpty()) {
        return res;
    } else if (!str1ForMatching.isEmpty() && str2ForMatching.isEmpty()) {
        res = delMarkerStart + str1ForMatching + delMarkerEnd;
    } else if (str1ForMatching.isEmpty() && !str2ForMatching.isEmpty()) {
        res = addMarkerStart + str2ForMatching + addMarkerEnd;
    } else {
        QStringList s1, s2;
        QStringList s1Space, s2Space;

        prepareLists(str1ForMatching, s1, s1Space, accel, markup);
        prepareLists(str2ForMatching, s2, s2Space, accel, markup);

        QStringList result(calcLCS(s1, s2, s1Space, s2Space));
        result.removeFirst(); //\t
        result.first().remove(0, 1); //\b
        result.replaceInStrings(delMarkerStart + delMarkerEnd, QString());
        result.replaceInStrings(addMarkerStart + addMarkerEnd, QString());

        if (options & Html) {
            result.replaceInStrings(QStringLiteral("&"), QStringLiteral("&amp;"));
            result.replaceInStrings(QStringLiteral("<"), QStringLiteral("&lt;"));
            result.replaceInStrings(QStringLiteral(">"), QStringLiteral("&gt;"));
        }
        res = result.join(QString());
        res.remove(addMarkerEnd + addMarkerStart);
        res.remove(delMarkerEnd + delMarkerStart);
    }

    // Convert from curly brace diff tags to HTML coloured with inline CSS
    if (options & Html)
        res = diffToHtmlDiff(res);

    return res;
}

QString diffToHtmlDiff(const QString &diff)
{
    // TODO: To improve this code, the hex colour generation could
    // be moved out of this function so that it is not calculated
    // every time this function is called. Bear in mind that it
    // should be updated when the user changes colour scheme:
    // perhaps reading how syntaxhighlighter.cpp works would be
    // useful as reference, and looking at using KStatefulBrush?
    QString coloredHtmlDiff;

    QString addDiffColorSpan;
    QString delDiffColorSpan;
    QTextCharFormat addDiffColor;
    QTextCharFormat delDiffColor;

    coloredHtmlDiff = diff;

    KColorScheme colorScheme(QPalette::Normal);
    addDiffColor.setForeground(colorScheme.foreground(KColorScheme::PositiveText));
    addDiffColor.setBackground(colorScheme.background(KColorScheme::PositiveBackground));
    delDiffColor.setForeground(colorScheme.foreground(KColorScheme::NegativeText));
    delDiffColor.setBackground(colorScheme.background(KColorScheme::NegativeBackground));
    // Note: the span classes below are used by tmview.cpp TM::targetAdapted() regex, but not elsewhere
    addDiffColorSpan = QLatin1String("<span class=\"lokalizeAddDiffColorSpan\" style=\"background-color:") + addDiffColor.background().color().name()
        + QLatin1String(";color:") + addDiffColor.foreground().color().name() + QLatin1String(";\">");
    delDiffColorSpan = QLatin1String("<span class=\"lokalizeDelDiffColorSpan\" style=\"background-color:") + delDiffColor.background().color().name()
        + QLatin1String(";color:") + delDiffColor.foreground().color().name() + QLatin1String(";\">");

    coloredHtmlDiff.replace(addMarkerStart, addDiffColorSpan);
    coloredHtmlDiff.replace(addMarkerEnd, QLatin1String("</span>"));
    coloredHtmlDiff.replace(delMarkerStart, delDiffColorSpan);
    coloredHtmlDiff.replace(delMarkerEnd, QLatin1String("</span>"));
    // Newline chars that are manual line breaks in the translation file are converted to HTML
    coloredHtmlDiff.replace(QLatin1String("\n"), QLatin1String("<br>"));

    return coloredHtmlDiff;
}
