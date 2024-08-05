/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2009 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "syntaxhighlighter.h"

#include "lokalize_debug.h"

#include "project.h"
#include "prefs_lokalize.h"
#include "prefs.h"

#include <kcolorscheme.h>

#include <QTextEdit>
#include <QApplication>
#include <QStringBuilder>

#define STATE_NORMAL 0
#define STATE_TAG 1



#define NUM_OF_RULES 5

SyntaxHighlighter::SyntaxHighlighter(QTextEdit *parent)
    : Sonnet::Highlighter(parent)
{
    highlightingRules.reserve(NUM_OF_RULES);
    HighlightingRule rule;
    //rule.format.setFontItalic(true);
//     tagFormat.setForeground(tagBrush.brush(QApplication::palette()));
    setAutomatic(false);

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
    rule.pattern = QRegularExpression(QStringLiteral("(&[A-Za-z_:][A-Za-z0-9_\\.:-]*;)"));
    highlightingRules.append(rule);

    QString accel = Project::instance()->accel();
    if (!accel.isEmpty()) {
        rule.format.setForeground(Qt::darkMagenta);
        rule.pattern = QRegularExpression(accel);
        highlightingRules.append(rule);
    }

    //\n \t \"
    rule.format.setForeground(Qt::darkGreen);
    rule.pattern = QRegularExpression(QStringLiteral("(\\\\[abfnrtv'\?\\\\])|(\\\\\\d+)|(\\\\x[\\dabcdef]+)"));
    highlightingRules.append(rule);

    //spaces
    settingsChanged();
    connect(SettingsController::instance(), &SettingsController::generalSettingsChanged, this, &SyntaxHighlighter::settingsChanged);

}

void SyntaxHighlighter::settingsChanged()
{
    const QRegularExpression re(QLatin1String(" +$|^ +"));
    if (Settings::highlightSpaces() && highlightingRules.last().pattern != re) {
        HighlightingRule rule;
        rule.format.clearForeground();
        
        // Note that the foreground and background colours have been switched in
        // the rules for formatting spaces (nbsp and leading/trailing spaces).
        // This makes the spaces easier to see.

        KColorScheme colorScheme(QPalette::Normal);
        // nbsp (non-breaking spaces)
        rule.format.setForeground(colorScheme.background(KColorScheme::AlternateBackground));
        rule.format.setBackground(colorScheme.foreground(KColorScheme::InactiveText));
        rule.format.setFontLetterSpacing(200);
        rule.pattern = QRegularExpression(QRegularExpression::escape(QChar(0x00a0U)));
        highlightingRules.append(rule);
        
        // soft hyphens
        rule.format.setForeground(colorScheme.foreground(KColorScheme::VisitedText));
        rule.format.setBackground(colorScheme.background(KColorScheme::VisitedBackground));
        rule.format.setFontLetterSpacing(100);
        rule.pattern = QRegularExpression(QLatin1String(".?") + QChar(0x0000AD) + QLatin1String(".?"));
        highlightingRules.append(rule);

        // spaces at the beginning/end of line
        rule.format.setForeground(colorScheme.background(KColorScheme::VisitedBackground));
        rule.format.setBackground(colorScheme.foreground(KColorScheme::VisitedText));
        rule.pattern = re;
        highlightingRules.append(rule);
        rehighlight();
    } else if (!Settings::highlightSpaces() && highlightingRules.last().pattern == re) {
        highlightingRules.resize(highlightingRules.size() - 3);
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
            startIndex = text.indexOf(QLatin1Char('<'));

        while (startIndex >= 0) {
            int endIndex = text.indexOf(QLatin1Char('>'), startIndex);
            int commentLength;
            if (endIndex == -1) {
                currentBlockState = STATE_TAG;
                commentLength = text.length() - startIndex;
            } else {
                commentLength = endIndex - startIndex
                                + 1/*+ commentEndExpression.matchedLength()*/;
            }
            setFormat(startIndex, commentLength, tagFormat);
            startIndex = text.indexOf(QLatin1Char('<'), startIndex + commentLength);
        }
    }

    for (const HighlightingRule &rule : std::as_const(highlightingRules)) {
        auto match = rule.pattern.match(text);
        while (match.hasMatch()) {
            const auto index = match.capturedStart();
            int length = match.capturedLength();
            QTextCharFormat f = rule.format;
            f.setFontItalic(!m_approved);
            setFormat(index, length, f);
            match = rule.pattern.match(text, index + length);
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
    for (int i = 0; i < count; ++i)
        underLines[i] = format(start + i).fontUnderline();

    setFormat(start, count, f);

    f.setFontUnderline(true);
    int prevStart = -1;
    bool isPrevUnderLined = false;
    for (int i = 0; i < count; ++i) {
        if (!underLines.at(i) && prevStart != -1)
            setFormat(start + isPrevUnderLined, i - prevStart, f);
        else if (underLines.at(i) && !isPrevUnderLined)
            prevStart = i;

        isPrevUnderLined = underLines.at(i);
    }
}
#endif

void SyntaxHighlighter::setMisspelled(int start, int count)
{
    const Project& project = *Project::instance();

    const QString text = currentBlock().text();
    QString word = text.mid(start, count);
    if (m_sourceString.contains(word)
        && QStringView(project.targetLangCode()).left(2) != QStringView(project.sourceLangCode()).left(2))
        return;

    const QString accel = project.accel();

    if (!isWordMisspelled(word.remove(accel)))
        return;
    count = word.length(); //safety

    bool smthPreceeding = (start > 0) &&
                          (accel.endsWith(text.at(start - 1))
                           || text.at(start - 1) == QChar(0x0000AD) //soft hyphen
                          );

    //HACK. Needs Sonnet API redesign (KDE 5)
    if (smthPreceeding) {
        qCWarning(LOKALIZE_LOG) << "ampersand is in the way. word len:" << count;
        const QRegularExpression regExp(QStringLiteral("\\b"));
        int realStart = text.lastIndexOf(regExp, start - 2);
        if (realStart == -1)
            realStart = 0;
        QString t = text.mid(realStart, count + start - realStart);
        t.remove(accel);
        t.remove(QChar(0x0000AD));
        if (!isWordMisspelled(t))
            return;
    }

    bool smthAfter = (start + count + 1 < text.size()) &&
                     (accel.startsWith(text.at(start + count))
                      || text.at(start + count) == QChar(0x0000AD) //soft hyphen
                     );
    if (smthAfter) {
        qCWarning(LOKALIZE_LOG) << "smthAfter. ampersand is in the way. word len:" << count;
        const QRegularExpression regExp(QStringLiteral("\\b"));
        int realEnd;
        if (const auto match = regExp.match(text, start + count + 2); match.hasMatch())
            realEnd = match.capturedStart();
        else
            realEnd = text.size();
        QString t = text.mid(start, realEnd - start);
        t.remove(accel);
        t.remove(QChar(0x0000AD));
        if (!isWordMisspelled(t))
            return;
    }

    if (count && format(start) == tagFormat)
        return;

    for (int i = 0; i < count; ++i) {
        QTextCharFormat f(format(start + i));
        f.setFontUnderline(true);
        f.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
        f.setUnderlineColor(Qt::red);
        setFormat(start + i, 1, f);
    }
}

void SyntaxHighlighter::unsetMisspelled(int start, int count)
{
    for (int i = 0; i < count; ++i) {
        QTextCharFormat f(format(start + i));
        f.setFontUnderline(false);
        setFormat(start + i, 1, f);
    }
}

#include "moc_syntaxhighlighter.cpp"
