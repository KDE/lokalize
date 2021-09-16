/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2009 Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

*/

#ifndef ALTTRANS_H
#define ALTTRANS_H

#include "catalogstring.h"
#include "tmentry.h"

struct AltTrans {
    ///@see https://docs.oasis-open.org/xliff/v1.2/os/xliff-core.html#alttranstype
    enum Type {Proposal, PreviousVersion, Rejected, Reference, Accepted, Other};
    Type type;

    CatalogString source;
    CatalogString target;

    short score;

    QString lang;
    QString origin;
    QString phase;

    AltTrans(const CatalogString& s = CatalogString(), const QString& o = QString()): type(Other), source(s), score(0), origin(o) {}
};


#endif
