/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2009 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef CATALOGCAPABILITIES_H
#define CATALOGCAPABILITIES_H

enum CatalogCapabilities {
    KeepsNoteAuthors = 1,
    MultipleNotes = 2,
    Phases = 4,
    ExtendedStates = 8,
    Tags = 16,
};

enum CatalogType {
    Gettext,
    Xliff,
    Ts,
};

#endif
