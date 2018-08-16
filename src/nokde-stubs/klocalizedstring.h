/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2014 by Nick Shaforostoff <shafff@ukr.net>

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

#ifndef KLOCALIZEDSTRING_H
#define KLOCALIZEDSTRING_H

#include <QObject>

static inline QString i18nc(const char* y, const char* x)
{
    return QObject::tr(x, y);
}
static inline QString i18nc(const char* y, const char* x, int n)
{
    return QObject::tr(x, y, n);
}
static inline QString i18nc(const char* y, const char* x, const QString& s)
{
    return QObject::tr(x, y).arg(s);
}
static inline QString i18nc(const char* y, const char* x, const QString& s1, const QString& s2)
{
    return QObject::tr(x, y).arg(s1).arg(s2);
}
static inline QString i18nc(const char* y, const char* x, int n, int m)
{
    return QObject::tr(x, y).arg(n).arg(m);
}
static inline QString i18n(const char* x, int n, int m)
{
    return QObject::tr(x).arg(n).arg(m);
}
static inline QString i18n(const char* x, const QString& s1, const QString& s2)
{
    return QObject::tr(x).arg(s1).arg(s2);
}
static inline QString i18n(const char* x)
{
    return QObject::tr(x);
}

namespace KLocalizedString
{
void setApplicationDomain(const char*);
};

#if 0
QString i18nc(const char* y, const char* x);
QString i18nc(const char* y, const char* x, int n);
QString i18nc(const char* y, const char* x, const QString& s);
QString i18nc(const char* y, const char* x, const QString& s1, const QString& s2);
QString i18nc(const char* y, const char* x, int n, int m)
{
    return QObject::tr(x, y).arg(n).arg(m);
}
QString i18n(const char* x, int n, int m)
{
    return QObject::tr(x).arg(n).arg(m);
}
QString i18n(const char* x, const QString& s1, const QString& s2)
{
    return QObject::tr(x).arg(s1).arg(s2);
}
QString i18n(const char* x)
{
    return QObject::tr(x);
}
#endif

#define I18N_NOOP2(y, x) x
#define I18N_NOOP(x) x

#endif
