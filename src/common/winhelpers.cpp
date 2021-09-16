/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2017 Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

*/

#include <QString>
#include <windows.h>
#define SECURITY_WIN32
#include <security.h>

QString fullUserName()
{
    ushort name[100];
    unsigned long size = 99;
    GetUserNameW((LPWSTR)name, &size);
    return QString::fromUtf16(name);
}
