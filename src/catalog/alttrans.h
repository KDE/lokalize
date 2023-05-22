/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2009 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef ALTTRANS_H
#define ALTTRANS_H

#include "catalogstring.h"
#include "tmentry.h"

struct AltTrans {
    ///@see https://docs.oasis-open.org/xliff/v1.2/os/xliff-core.html#alttranstype
    enum Type {Proposal, PreviousVersion, Rejected, Reference, Accepted, Other};
    Type type{Other};

    CatalogString source;
    CatalogString target;

    short score{0};

    QString lang;
    QString origin;
    QString phase;

    explicit AltTrans(const CatalogString& s = CatalogString(), const QString& o = QString())
        : source(s)
        , origin(o)
    {}
};


#endif
