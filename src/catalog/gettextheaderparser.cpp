/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2008-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "gettextheaderparser.h"
#include <QStringList>
#include <QRegExp>
#include <QLocale>
#include <QDate>
#include <QDebug>

const QString BACKSLASH_N = QStringLiteral("\\n");

QString GetTextHeaderParser::joinAuthor(const QString &authorName, const QString &authorEmail)
{
    QString outputString;
    if (!authorEmail.isEmpty()) {
        outputString = QString("%1 <%2>").arg(authorName, authorEmail);
    } else {
        outputString = authorName;
    }
    return outputString;
}

void GetTextHeaderParser::updateLastTranslator(QStringList &headerList, const QString &authorName, const QString &authorEmail)
{
    const QString outputString = QStringLiteral("Last-Translator: ") + joinAuthor(authorName, authorEmail) + BACKSLASH_N;

    const QRegExp regex(QStringLiteral("^ *Last-Translator:.*"));
    auto needle = std::find_if(headerList.begin(), headerList.end(), [regex](const QString &line){
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
    const QRegExp regex(QStringLiteral("^# *Copyright (\\(C\\)|\\x00a9).*This file is copyright:"));
    auto needle = std::find_if(commentList.begin(), commentList.end(), [regex](const QString &line){
        return line.contains(regex);
    });
    if (needle != commentList.end()) {
        needle->replace(QStringLiteral("YEAR"), cLocale.toString(QDate::currentDate(), QStringLiteral("yyyy")));
    }
}

void GetTextHeaderParser::updateAuthors(QStringList &commentList, const QString &authorName, const QString &authorEmail)
{
    QStringList foundAuthors;
    const QLocale cLocale(QLocale::C);
    const QString authorNameEmail = GetTextHeaderParser::joinAuthor(authorName, authorEmail);

    QString temp = QStringLiteral("# ") + authorNameEmail + QStringLiteral(", ") + cLocale.toString(QDate::currentDate(), QStringLiteral("yyyy")) + '.';

    // ### TODO: it would be nice if the entry could start with "COPYRIGHT" and have the "(C)" symbol (both not mandatory)
    QRegExp regexpAuthorYear(QStringLiteral("^#.*(<.+@.+>)?,\\s*([\\d]+[\\d\\-, ]*|YEAR)"));
    QRegExp regexpYearAlone(QStringLiteral("^# , \\d{4}.?\\s*$"));
    if (commentList.isEmpty()) {
        commentList.append(temp);
        commentList.append(QString());
    } else {
        auto it = commentList.begin();
        while (it != commentList.end()) {
            bool deleteItem = false;
            if (it->indexOf(QLatin1String("copyright"), 0, Qt::CaseInsensitive) != -1) {
                // We have a line with a copyright. It should not be moved.
            } else if (it->contains(QRegExp(QStringLiteral("#, *fuzzy"))))
                deleteItem = true;
            else if (it->contains(regexpYearAlone)) {
                // We have found a year number that is preceded by a comma.
                // That is typical of KBabel 1.10 (and earlier?) when there is neither an author name nor an email
                // Remove the entry
                deleteItem = true;
            } else if (it->contains(QLatin1String("# FIRST AUTHOR <EMAIL@ADDRESS>, YEAR."))) {
                deleteItem = true;
            } else if (it->contains(QLatin1String("# SOME DESCRIPTIVE TITLE")))
                deleteItem = true;
            else if (it->contains(regexpAuthorYear)) {   // email address followed by year
                if (!foundAuthors.contains((*it))) {
                    // The author line is new (and not a duplicate), so add it to the author line list
                    foundAuthors.append((*it));
                }
                // Delete also non-duplicated entry, as now all what is needed will be processed in foundAuthors
                deleteItem = true;
            }

            if (deleteItem)
                it = commentList.erase(it);
            else
                ++it;
        }

        if (!foundAuthors.isEmpty()) {
            bool found = false;
            bool foundAuthor = false;

            const QString cy = cLocale.toString(QDate::currentDate(), QStringLiteral("yyyy"));

            auto ait = foundAuthors.end();
            for (it = foundAuthors.begin() ; it != foundAuthors.end(); ++it) {
                if (it->contains(authorName) || it->contains(authorEmail)) {
                    foundAuthor = true;
                    if (it->contains(cy))
                        found = true;
                    else
                        ait = it;
                }
            }
            if (!found) {
                if (!foundAuthor) {
                    foundAuthors.append(temp);
                } else if (ait != foundAuthors.end()) {
                    //update years
                    const int index = (*ait).lastIndexOf(QRegExp(QStringLiteral("[\\d]+[\\d\\-, ]*")));
                    if (index == -1) {
                        (*ait) += QStringLiteral(", ") + cy;
                    } else {
                        ait->insert(index + 1, QStringLiteral(", ") + cy);
                    }
                } else {
                    qDebug() << "INTERNAL ERROR: author found but iterator dangling!";
                }
            }

        } else {
            foundAuthors.append(temp);
        }

        for (QString author : qAsConst(foundAuthors)) {
            // ensure dot at the end of copyright
            if (!author.endsWith(QLatin1Char('.'))) {
                author += QLatin1Char('.');
            }
            commentList.append(author);
        }
    }
}
