/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2014 by Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

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

#include <QString>
#include <QStringBuilder>
#include <QRegularExpression>

QString escapeWithLinks(const QString& text)
{
    QString html;
    static QRegularExpression urlDetector(QStringLiteral("(https?|ftp)://[^\\s/$.?#].[^\\s]*"));
    QStringList parts = text.split(urlDetector);
    if (parts.size()) html += parts.takeFirst().toHtmlEscaped();

    QRegularExpressionMatchIterator i = urlDetector.globalMatch(text);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString word = match.captured(0);
        html += QStringLiteral("<a href=\"") + word.toHtmlEscaped() + QStringLiteral("\">") + word.toHtmlEscaped() + QStringLiteral("</a>");
        if (parts.size()) html += parts.takeFirst().toHtmlEscaped();
    }
    return html;
}

