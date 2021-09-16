/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2009 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <sonnet/highlighter.h>
#include <sonnet/speller.h>
#include <kcolorscheme.h>

#include <QHash>
#include <QTextCharFormat>


class QTextEdit;

class SyntaxHighlighter : public Sonnet::Highlighter
{
    Q_OBJECT

public:
    explicit SyntaxHighlighter(QTextEdit *parent);
    ~SyntaxHighlighter() override = default;

    void setApprovementState(bool a)
    {
        m_approved = a;
    }
    void setSourceString(const QString& s)
    {
        m_sourceString = s;
    }

protected:
    void highlightBlock(const QString &text) override;

    void setMisspelled(int start, int count) override;
    void unsetMisspelled(int start, int count) override;

private Q_SLOTS:
    void settingsChanged();

//    void setFormatRetainingUnderlines(int start, int count, QTextCharFormat format);
private:
    struct HighlightingRule {
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
