/*
    XLIFF translation file analyzer

    Copyright (C) 2011 Albert Astals Cid <aacid@kde.org>
    Copyright (C) 2015 Nick Shaforostoff <shaforostoff@gmail.com>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
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
