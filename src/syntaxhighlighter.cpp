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

#include "syntaxhighlighter.h"

#include "project.h"
#include "prefs_lokalize.h"
#include "prefs.h"

#include <KDebug>
#include <KColorScheme>

#include <QTextEdit>
#include <QApplication>

#define STATE_NORMAL 0
#define STATE_TAG 1



#define NUM_OF_RULES 5

SyntaxHighlighter::SyntaxHighlighter(QTextEdit *parent)
    : Sonnet::Highlighter(parent)
    , tagBrush(KColorScheme::View,KColorScheme::VisitedText)
    , m_approved(true)
//     , fuzzyState(false)
//     , fromDocbook(docbook)
{
    setAutomatic(false);

    highlightingRules.reserve(NUM_OF_RULES);
    HighlightingRule rule;
    //rule.format.setFontItalic(true);
//     tagFormat.setForeground(tagBrush.brush(QApplication::palette()));
    tagFormat.setForeground(tagBrush.brush(QApplication::palette()));
    //QTextCharFormat format;
    //tagFormat.setForeground(Qt::darkBlue);
//     if (!docbook) //support multiline tags
//     {
//         rule.format = tagFormat;
//         rule.pattern = QRegExp("<.+>");
//         rule.pattern.setMinimal(true);
//         highlightingRules.append(rule);
//     }

    //entity
    rule.format.setForeground(Qt::darkMagenta);
    rule.pattern = QRegExp("(&[A-Za-z_:][A-Za-z0-9_\\.:-]*;)");
    highlightingRules.append(rule);

    QString accel=Project::instance()->accel();
    if (!accel.isEmpty())
    {
        rule.format.setForeground(Qt::darkMagenta);
        rule.pattern = QRegExp(accel);
        highlightingRules.append(rule);
    }

    //\n \t \"
    rule.format.setForeground(Qt::darkGreen);
    rule.pattern = QRegExp("(\\\\[abfnrtv'\?\\\\])|(\\\\\\d+)|(\\\\x[\\dabcdef]+)");
    highlightingRules.append(rule);





    //spaces
    settingsChanged();
    connect(SettingsController::instance(),SIGNAL(generalSettingsChanged()),this, SLOT(settingsChanged()));

}

void SyntaxHighlighter::settingsChanged()
{
    QRegExp re(QString(" +$|^ +|.?")+QChar(0x0000AD)+".?"); //soft hyphen
    if (Settings::highlightSpaces() && highlightingRules.last().pattern!=re)
    {
        KColorScheme colorScheme(QPalette::Normal);
        HighlightingRule rule;
        rule.format.clearForeground();

        //nbsp
        rule.format.setBackground(colorScheme.background(KColorScheme::AlternateBackground));
        rule.pattern = QRegExp(QChar(0x00a0U));
        highlightingRules.append(rule);

        //usual spaces at the end
        rule.format.setBackground(colorScheme.background(KColorScheme::ActiveBackground));
        rule.pattern = re;
        highlightingRules.append(rule);
        rehighlight();
    }
    else if (!Settings::highlightSpaces() && highlightingRules.last().pattern==re)
    {
        highlightingRules.resize(highlightingRules.size()-2);
        rehighlight();
    }
}

/*
void SyntaxHighlighter::setFuzzyState(bool fuzzy)
{
    return;
    int i=NUM_OF_RULES;
    while(--i>=0)
        highlightingRules[i].format.setFontItalic(fuzzy);

    tagFormat.setFontItalic(fuzzy);
}*/

void SyntaxHighlighter::highlightBlock(const QString &text)
{
    int currentBlockState = STATE_NORMAL;
    QTextCharFormat f;
    f.setFontItalic(!m_approved);
    setFormat(0, text.length(), f);

    tagFormat.setFontItalic(!m_approved);
    //if (fromDocbook)
    {
        int startIndex = STATE_NORMAL;
        if (previousBlockState() != STATE_TAG)
            startIndex = text.indexOf('<');

        while (startIndex >= 0)
        {
            int endIndex = text.indexOf('>', startIndex);
            int commentLength;
            if (endIndex == -1)
            {
                currentBlockState = STATE_TAG;
                commentLength = text.length() - startIndex;
            }
            else
            {
                commentLength = endIndex - startIndex
                                +1/*+ commentEndExpression.matchedLength()*/;
            }
            setFormat(startIndex, commentLength, tagFormat);
            startIndex = text.indexOf('<', startIndex + commentLength);
        }
    }

    foreach (const HighlightingRule &rule, highlightingRules)
    {
        QRegExp expression(rule.pattern);
        int index = expression.indexIn(text);
        while (index >= 0)
        {
            int length = expression.matchedLength();
            QTextCharFormat f=rule.format;
            f.setFontItalic(!m_approved);
            setFormat(index, length, f);
            index = expression.indexIn(text, index + length);
        }
    }

    if (spellCheckerFound())
        Sonnet::Highlighter::highlightBlock(text); // Resets current block state

    setCurrentBlockState(currentBlockState);
}

#if 0
void SyntaxHighlighter::setFormatRetainingUnderlines(int start, int count, QTextCharFormat f)
{
    QVector<bool> underLines(count);
    for (int i=0;i<count;++i)
        underLines[i]=format(start+i).fontUnderline();

    setFormat(start, count, f);

    f.setFontUnderline(true);
    int prevStart=-1;
    bool isPrevUnderLined=false;
    for (int i=0;i<count;++i)
    {
        if (!underLines.at(i) && prevStart!=-1)
            setFormat(start+isPrevUnderLined, i-prevStart, f);
        else if (underLines.at(i)&&!isPrevUnderLined)
            prevStart=i;

        isPrevUnderLined=underLines.at(i);
    }
}
#endif

void SyntaxHighlighter::setMisspelled(int start, int count)
{
    QString text=currentBlock().text();
    QString word=text.mid(start,count);
    if (m_sourceString.contains(word))
        return;

    QString accel=Project::instance()->accel();

    if (!isWordMisspelled(word.remove(accel)))
        return;
    count=word.length();//safety
    
    bool smthPreceeding=(start>0) &&
            (accel.endsWith(text.at(start-1))
                || text.at(start-1)==QChar(0x0000AD) //soft hyphen
            );

    //HACK. Needs Sonnet API redesign (KDE 5)
    if (smthPreceeding)
    {
        kWarning()<<"ampersand is in the way. word len:"<<count;
        int realStart=text.lastIndexOf(QRegExp("\\b"),start-2);
        if (realStart==-1)
            realStart=0;
        QString t=text.mid(realStart, count+start-realStart);
        t.remove(accel);
        t.remove(QChar(0x0000AD));
        if (!isWordMisspelled(t))
            return;
    }

    bool smthAfter=(start+count+1<text.size()) &&
            (accel.startsWith(text.at(start+count))
                || text.at(start+count)==QChar(0x0000AD) //soft hyphen
            );
    if (smthAfter)
    {
        kWarning()<<"smthAfter. ampersand is in the way. word len:"<<count;
        int realEnd=text.indexOf(QRegExp("\\b"),start+count+2);
        if (realEnd==-1)
            realEnd=text.size();
        QString t=text.mid(start, realEnd-start);
        t.remove(accel);
        t.remove(QChar(0x0000AD));
        if (!isWordMisspelled(t))
            return;
    }

    if (count && format(start)==tagFormat)
        return;

    for (int i=0;i<count;++i)
    {
        QTextCharFormat f(format(start+i));
        f.setFontUnderline(true);
        f.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
        f.setUnderlineColor(Qt::red);
        setFormat(start+i, 1, f);
    }
}

void SyntaxHighlighter::unsetMisspelled(int start, int count)
{
    for (int i=0;i<count;++i)
    {
        QTextCharFormat f(format(start+i));
        f.setFontUnderline(false);
        setFormat(start+i, 1, f);
    }
}



#include "syntaxhighlighter.moc"

