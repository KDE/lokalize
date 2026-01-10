/*
  This file is part of Lokalize

  Gettext translation file analyzer

  SPDX-FileCopyrightText: 2007      Montel Laurent <montel@kde.org>
  SPDX-FileCopyrightText: 2009      Jos van den Oever <jos@vandenoever.info>
  SPDX-FileCopyrightText: 2014      Nick Shaforostoff <shaforostoff@gmail.com>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: LGPL-2.1-or-later
*/

#ifndef POEXTRACTOR_H
#define POEXTRACTOR_H

#include "filemetadata.h"

class POExtractor
{
public:
    POExtractor() = default;
    FileMetaData extract(const QString &filePath);

private:
    void endMessage();
    void handleComment(const char *data, uint32_t length);
    void handleLine(const char *data, uint32_t length);

    enum PoState {
        COMMENT,
        MSGCTXT,
        MSGID,
        MSGID_PLURAL,
        MSGSTR,
        MSGSTR_PLURAL,
        WHITESPACE,
        ERROR,
    };
    PoState state{WHITESPACE};
    int messages{0};
    int untranslated{0};
    int fuzzy{0};
    bool isFuzzy{false};
    bool isTranslated{false};
};

#endif // PLAINTEXTEXTRACTOR_H
