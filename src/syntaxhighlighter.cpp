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


#include "syntaxhighlighter.h"

#include "project.h"
#include "prefs_lokalize.h"
#include "prefs.h"

#include <KDebug>
#include <KColorScheme>

#include <QApplication>

#define STATE_NORMAL 0
#define STATE_TAG 1


#define NUM_OF_RULES 5

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent/*, bool docbook*/)
    : QSyntaxHighlighter(parent)
    , tagBrush(KColorScheme::View,KColorScheme::VisitedText)
//     , fuzzyState(false)
//     , fromDocbook(docbook)
{
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

    rule.format.setForeground(Qt::darkMagenta);
    rule.pattern = QRegExp(Project::instance()->accel());
    //rule.pattern = QRegExp("&[^;]*;");
/*    QString accelRx=Project::instance()->accel();
    int pos=accelRx.indexOf('(')+1;
    rule.pattern = QRegExp(  accelRx.mid( pos,accelRx.indexOf(')',pos)-1 )  );*/
    highlightingRules.append(rule);

    //\n \t \"
    rule.format.setForeground(Qt::darkGreen);
    rule.pattern = QRegExp("(\\\\[abfnrtv'\"\?\\\\])|(\\\\\\d+)|(\\\\x[\\dabcdef]+)");
    highlightingRules.append(rule);





    //spaces
    settingsChanged();
    connect(SettingsController::instance(),SIGNAL(generalSettingsChanged()),this, SLOT(settingsChanged()));

}

void SyntaxHighlighter::settingsChanged()
{
    QRegExp re(" +$|^ +");
    if (Settings::highlightSpaces() && highlightingRules.last().pattern!=re)
    {
        HighlightingRule rule;
        KColorScheme colorScheme(QPalette::Normal);
        rule.format.clearForeground();
        rule.format.setBackground(colorScheme.background(KColorScheme::ActiveBackground));
        rule.pattern = re;
        highlightingRules.append(rule);
        rehighlight();
    }
    else if (!Settings::highlightSpaces() && highlightingRules.last().pattern==re)
    {
        highlightingRules.resize(highlightingRules.size()-1);
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
//     if (fromDocbook)
    {
        setCurrentBlockState(STATE_NORMAL);

        int startIndex = STATE_NORMAL;
        if (previousBlockState() != STATE_TAG)
            startIndex = text.indexOf('<');

        while (startIndex >= 0)
        {
            int endIndex = text.indexOf('>', startIndex);
            int commentLength;
            if (endIndex == -1)
            {
                setCurrentBlockState(STATE_TAG);
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
        int index = text.indexOf(expression);
        while (index >= 0)
        {
            int length = expression.matchedLength();
            setFormat(index, length, rule.format);
            index = text.indexOf(expression, index + length);
        }
    }
}


#include "syntaxhighlighter.moc"

