/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2014      Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include <QRegularExpression>
#include <QString>
#include <QStringBuilder>

QString escapeWithLinks(const QString &text)
{
    QString html;
    static QRegularExpression urlDetector(QStringLiteral("(https?|ftp)://[^\\s/$.?#].[^\\s]*"));
    QStringList parts = text.split(urlDetector);
    if (parts.size())
        html += parts.takeFirst().toHtmlEscaped();

    QRegularExpressionMatchIterator i = urlDetector.globalMatch(text);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString word = match.captured(0);
        html += QStringLiteral("<a href=\"") + word.toHtmlEscaped() + QStringLiteral("\">") + word.toHtmlEscaped() + QStringLiteral("</a>");
        if (parts.size())
            html += parts.takeFirst().toHtmlEscaped();
    }
    return html;
}
