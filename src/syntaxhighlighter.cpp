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

#include <QtGui>

#define STATE_NORMAL 0
#define STATE_TAG 1


SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent, bool docbook)
    : QSyntaxHighlighter(parent), fromDocbook(docbook)
{
    HighlightingRule rule;
    //QTextCharFormat format;
    tagFormat.setForeground(Qt::darkBlue);
    if (!docbook) //support multiline tags
    {
        rule.format = tagFormat;
        rule.pattern = QRegExp("<[^>]*>");
        highlightingRules.append(rule);
    }

    //enity
    rule.format.setForeground(Qt::darkMagenta);
    //rule.pattern = QRegExp("&[^;]*;");
    rule.pattern = QRegExp("(&[A-Za-z_:][A-Za-z0-9_.:-]*;)");
    highlightingRules.append(rule);

    //\n \t \"
    rule.format.setForeground(Qt::darkGreen);
    //rule.pattern = QRegExp("&[^;]*;");
    rule.pattern = QRegExp("(\\\\[abfnrtv'\"\?\\\\])|(\\\\\\d+)|(\\\\x[\\dabcdef]+)");
    highlightingRules.append(rule);

/*
    //spaces
    rule.format.clearForeground();
    rule.format.setBackground(Qt::Dense6Pattern);
    rule.pattern = QRegExp(" ");
    highlightingRules.append(rule);
*/
//     commentStartExpression = QRegExp("/\\*");
//     commentEndExpression = QRegExp("\\*/");

}

void SyntaxHighlighter::highlightBlock(const QString &text)
{
    if (fromDocbook)
    {
        setCurrentBlockState(STATE_NORMAL);

        int startIndex = STATE_NORMAL;
        if (previousBlockState() != STATE_TAG)
            startIndex = text.indexOf("<");

        while (startIndex >= 0)
        {
            int endIndex = text.indexOf(">", startIndex);
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
            startIndex = text.indexOf("<",
                                                    startIndex + commentLength);
        }
    }

    foreach (HighlightingRule rule, highlightingRules)
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

