/*
    XLIFF translation file analyzer

    SPDX-FileCopyrightText: 2011 Albert Astals Cid <aacid@kde.org>
    SPDX-FileCopyrightText: 2015 Nick Shaforostoff <shaforostoff@gmail.com>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#ifndef XLIFFEXTRACTOR_H
#define XLIFFEXTRACTOR_H

#include "filemetadata.h"

class XliffExtractor
{

public:
    XliffExtractor() = default;
    FileMetaData extract(const QString& filePath);
};

#endif // PLAINTEXTEXTRACTOR_H
