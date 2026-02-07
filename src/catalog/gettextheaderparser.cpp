/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2008-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>
  SPDX-FileCopyrightText: 2022      Andreas Cord-Landwehr <cordlandwehr@kde.org>
  SPDX-FileCopyrightText: 2026      Varun Sajith Dass <varunsajithdas@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "gettextheaderparser.h"
#include "lokalize_debug.h"

#include <QDate>
#include <QDebug>
#include <QLocale>
#include <QRegularExpression>
#include <QStringList>

const QString GetTextHeaderParser::sCurrentYear = QLocale(QLocale::C).toString(QDate::currentDate(), QStringLiteral("yyyy"));

QRegularExpression copyrightRegExp()
{
    static auto regexp = QRegularExpression(QStringLiteral(
        "#[ ]*"
        "(SPDX-FileCopyrightText:|[cC]opyright(\\s*:?\\s+\\([cC]\\))|(?<![cC]opyright )\\([cC]\\)|[cC]opyright\\s+©|(?<![cC]opyright )©|[cC]opyright(\\s*:)?)?"
        "[, ]+"
        "(?<years>([0-9]+(-[0-9]+| - [0-9]+| to [0-9]+|,[ ]?[0-9]+)*|YEAR))?"
        "[, ]*"
        "([bB]y[ ]+)?"
        "(?<name>([\u00C0-\u017Fa-zA-Z\\-\\.]+( [\u00C0-\u017Fa-zA-Z\\-\\.]+)*))"
        "[, ]*"
        "(?<contact>[^\\s,]*)"
        "[, ]*"
        "(?<yearssuffix>([0-9]+(-[0-9]+| - [0-9]+| to [0-9]+|,[ ]?[0-9]+)*|YEAR))?"));
    return regexp;
}

QString GetTextHeaderParser::joinAuthor(const QString &authorName, const QString &authorEmail)
{
    QString outputString;
    if (!authorEmail.isEmpty()) {
        outputString = QString(QStringLiteral("%1 <%2>")).arg(authorName, authorEmail);
    } else {
        outputString = authorName;
    }
    return outputString;
}

QString GetTextHeaderParser::simplifyYearString(const QString &input)
{
    QList<int> years;
    // Handle both commas and existing separators like " - " or " to " if they exist in legacy headers
    const QStringList parts = input.split(QLatin1Char(','), Qt::SkipEmptyParts);

    for (const QString &part : parts) {
        QString cleanPart = part.trimmed();

        // Check for range (e.g., "2010-2012" or "2010 - 2012")
        if (cleanPart.contains(QLatin1Char('-'))) {
            const QStringList range = cleanPart.split(QLatin1Char('-'));
            if (range.size() >= 2) {
                bool startOk, endOk;
                int start = range.first().trimmed().toInt(&startOk);
                int end = range.last().trimmed().toInt(&endOk);

                if (startOk && endOk && start <= end) {
                    for (int y = start; y <= end; ++y) {
                        if (!years.contains(y))
                            years.append(y);
                    }
                }
            }
        } else {
            // Single year case
            bool ok;
            const int year = cleanPart.toInt(&ok);
            if (ok && !years.contains(year)) {
                years.append(year);
            }
        }
    }

    if (years.isEmpty()) {
        return input;
    }

    std::sort(years.begin(), years.end());

    QStringList resultParts;
    int rangeStart = years.first();
    int prev = years.first();

    for (int i = 1; i < years.size(); ++i) {
        const int current = years.at(i);
        if (current == prev + 1) {
            prev = current;
        } else {
            if (rangeStart == prev) {
                resultParts << QString::number(rangeStart);
            } else {
                resultParts << QStringLiteral("%1-%2").arg(rangeStart).arg(prev);
            }
            rangeStart = current;
            prev = current;
        }
    }

    if (rangeStart == prev) {
        resultParts << QString::number(rangeStart);
    } else {
        resultParts << QStringLiteral("%1-%2").arg(rangeStart).arg(prev);
    }

    return resultParts.join(QStringLiteral(", "));
}

QString GetTextHeaderParser::updateAuthorCopyrightLine(const QString &line)
{
    QRegularExpression regExp = copyrightRegExp();
    QString lineTrimmed = line.trimmed();
    if (lineTrimmed.endsWith(QStringLiteral("."))) { // remove tailing "." if exists
        lineTrimmed = lineTrimmed.left(lineTrimmed.length() - 1);
    }
    auto match = regExp.match(lineTrimmed);
    while (match.hasMatch()) {
        QString name = match.captured(QStringLiteral("name"));
        QString contact = match.captured(QStringLiteral("contact"));
        QString years = match.captured(QStringLiteral("years"));
        QString yearssuffix = match.captured(QStringLiteral("yearssuffix"));
        // handle traditional statements from Lokalize
        if (years.isEmpty() && !yearssuffix.isEmpty()) {
            years = yearssuffix;
        } else if (!years.isEmpty() && !yearssuffix.isEmpty()) {
            years = years + QStringLiteral(", ") + yearssuffix;
            qCDebug(LOKALIZE_LOG) << "Unexpected double copyright year statement detected, try to handle gracefully:" << line;
        }
        // handle year update
        if (years.isEmpty()) {
            years = sCurrentYear;
        }
        if (!years.contains(sCurrentYear)) {
            years.append(QStringLiteral(", ") + sCurrentYear);
        }

        // Simplify year string (e.g., convert "2010, 2011, 2012" to "2010-2012")
        years = simplifyYearString(years);

        return QString(QStringLiteral("# SPDX-FileCopyrightText: %1 %2 %3")).arg(years, name, contact).trimmed();
    }
    qCDebug(LOKALIZE_LOG) << "Cannot parse copyright line" << line;
    return line;
}

void GetTextHeaderParser::updateLastTranslator(QStringList &headerList, const QString &authorName, const QString &authorEmail)
{
    const QString outputString = QStringLiteral("Last-Translator: ") + joinAuthor(authorName, authorEmail) + QStringLiteral("\\n");

    const QRegularExpression regex(QStringLiteral("^ *Last-Translator:.*"));
    auto needle = std::find_if(headerList.begin(), headerList.end(), [regex](const QString &line) {
        return line.contains(regex);
    });
    if (Q_LIKELY(needle != headerList.end())) {
        *needle = outputString;
    } else {
        headerList.append(outputString);
    }
}

void GetTextHeaderParser::updateGeneralCopyrightYear(QStringList &commentList)
{
    const QLocale cLocale(QLocale::C);
    // U+00A9 is the Copyright sign
    const QRegularExpression regex(QStringLiteral("^# *Copyright (\\(C\\)|\\x00a9).*This file is copyright:"));
    auto needle = std::find_if(commentList.begin(), commentList.end(), [regex](const QString &line) {
        return line.contains(regex);
    });
    if (needle != commentList.end()) {
        needle->replace(QStringLiteral("YEAR"), cLocale.toString(QDate::currentDate(), QStringLiteral("yyyy")));
    }
}

void GetTextHeaderParser::updateAuthors(QStringList &commentList, const QString &authorName, const QString &authorEmail)
{
    // cleanup template parameters and obsolete information
    commentList.erase(std::remove_if(commentList.begin(),
                                     commentList.end(),
                                     [](const QString &line) {
                                         const QRegularExpression regexpYearAlone(QStringLiteral("^# , \\d{4}.?\\s*$"));
                                         if (line.contains(QRegularExpression(QStringLiteral("#, *fuzzy")))
                                             // We have found a year number that is preceded by a comma.
                                             // That is typical of KBabel 1.10 (and earlier?) when there is neither an author name nor an email
                                             // Remove the entry
                                             || line.contains(regexpYearAlone) || line.contains(QStringLiteral("# FIRST AUTHOR <EMAIL@ADDRESS>, YEAR."))
                                             || line.contains(QStringLiteral("# SOME DESCRIPTIVE TITLE"))) {
                                             qCDebug(LOKALIZE_LOG) << "Removing line:" << line;
                                             return true;
                                         }
                                         return false;
                                     }),
                      commentList.end());

    // update author or add as new
    auto authorLine = std::find_if(commentList.begin(), commentList.end(), [authorName, authorEmail](const QString &line) {
        static const QRegularExpression regExp = copyrightRegExp();
        QString lineTrimmed = line.trimmed();
        if (lineTrimmed.endsWith(QStringLiteral("."))) { // remove tailing "." if exists
            lineTrimmed = lineTrimmed.left(lineTrimmed.length() - 1);
        }
        const auto match = regExp.match(lineTrimmed);
        if (match.hasMatch() && (line.contains(authorName) || line.contains(authorEmail))) {
            return true;
        }
        return false;
    });

    if (authorLine == commentList.end()) { // author missing, add as new
        commentList << QStringLiteral("# SPDX-FileCopyrightText: ") + sCurrentYear + QStringLiteral(" ") + joinAuthor(authorName, authorEmail);
    } else { // update years
        *authorLine = updateAuthorCopyrightLine(*authorLine);
    }
}
