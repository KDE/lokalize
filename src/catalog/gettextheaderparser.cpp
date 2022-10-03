/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2008-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "gettextheaderparser.h"
#include <QStringList>
#include <QRegExp>

const QString BACKSLASH_N = QStringLiteral("\\n");

void GetTextHeaderParser::updateLastTranslator(QStringList &headerList, const QString &authorName, const QString &authorEmail)
{
    QString outputString;
    if (!authorEmail.isEmpty()) {
        outputString = QString("Last-Translator: %1 <%2>" + BACKSLASH_N).arg(authorName, authorEmail);
    } else {
        outputString = QString("Last-Translator: %1" + BACKSLASH_N).arg(authorName);
    }

    QRegExp regexp(QStringLiteral("^ *Last-Translator:.*"));
    auto needle = std::find_if(headerList.begin(), headerList.end(), [regexp](const QString &line){
        return line.contains(regexp);
    });
    if (Q_LIKELY(needle != headerList.end())) {
        *needle = outputString;
    } else {
        headerList.append(outputString);
    }
}
