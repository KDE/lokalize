/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2009 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>
  SPDX-FileCopyrightText: 2024      Karl Ove Hufthammer <karl@huftis.org>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include <KColorScheme>
#include <KStatefulBrush>
#include <Sonnet/Highlighter>
#include <Sonnet/Speller>

#include <QRegularExpression>
#include <QSyntaxHighlighter>

class QTextCharFormat;
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
    void setSourceString(const QString &s)
    {
        m_sourceString = s;
    }

protected:
    void highlightBlock(const QString &text) override;

    void setMisspelled(int start, int count) override;
    void unsetMisspelled(int start, int count) override;

private Q_SLOTS:
    void settingsChanged();

private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;
    QTextCharFormat tagFormat;
    KStatefulBrush tagBrush{KColorScheme::View, KColorScheme::VisitedText};
    KStatefulBrush escapeCharBrush{KColorScheme::View, KColorScheme::PositiveText};
    bool m_approved{true};
    QString m_sourceString;
};

#endif
