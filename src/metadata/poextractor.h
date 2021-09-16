/*
    Gettext translation file analyzer

    SPDX-FileCopyrightText: 2007 Montel Laurent <montel@kde.org>
    SPDX-FileCopyrightText: 2009 Jos van den Oever <jos@vandenoever.info>
    SPDX-FileCopyrightText: 2014 Nick Shaforostoff <shaforostoff@gmail.com>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/


#ifndef POEXTRACTOR_H
#define POEXTRACTOR_H

#include "filemetadata.h"

class POExtractor
{

public:
    POExtractor();
    FileMetaData extract(const QString& filePath);

private:
    void endMessage();
    void handleComment(const char* data, uint32_t length);
    void handleLine(const char* data, uint32_t length);


    enum PoState {COMMENT, MSGCTXT, MSGID, MSGID_PLURAL, MSGSTR, MSGSTR_PLURAL,
                  WHITESPACE, ERROR
                 };
    PoState state;
    int messages;
    int untranslated;
    int fuzzy;
    bool isFuzzy, isTranslated;
};

#endif // PLAINTEXTEXTRACTOR_H
