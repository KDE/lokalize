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

#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <sonnet/highlighter.h>
#include <kcolorscheme.h>

#include <QHash>
#include <QTextCharFormat>


class QTextDocument;

class SyntaxHighlighter : public Sonnet::Highlighter
{
    Q_OBJECT

public:
    explicit SyntaxHighlighter(QTextEdit *parent);
    ~SyntaxHighlighter(){};

    void setApprovementState(bool a){m_approved=a;};
    void setSourceString(const QString& s){m_sourceString=s;}

protected:
    void highlightBlock(const QString &text);

    void setMisspelled(int start, int count);
    void unsetMisspelled(int start, int count);

private slots:
    void settingsChanged();

//    void setFormatRetainingUnderlines(int start, int count, QTextCharFormat format);
private:
    struct HighlightingRule
    {
        QRegExp pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

//     bool fromDocbook;
    QTextCharFormat tagFormat;
    KStatefulBrush tagBrush;
    bool m_approved;
    QString m_sourceString;
};

#endif
