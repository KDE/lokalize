/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2011 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

*/

#include "domroutines.h"

void setText(QDomElement element, QString text)
{
    QDomNodeList children = element.childNodes();
    for (int i = 0; i < children.count(); i++) {
        if (children.at(i).isCharacterData()) {
            children.at(i).toCharacterData().setData(text);
            text.clear();
        }
    }

    if (!text.isEmpty())
        element.appendChild(element.ownerDocument().createTextNode(text));
}

