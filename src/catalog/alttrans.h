/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2009 by Nick Shaforostoff <shafff@ukr.net>

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

#ifndef ALTTRANS_H
#define ALTTRANS_H

#include "catalogstring.h"
#include "tmentry.h"

struct AltTrans
{
    ///@see @link http://docs.oasis-open.org/xliff/v1.2/os/xliff-core.html#alttranstype
    enum Type {Proposal, PreviousVersion, Rejected, Reference, Accepted, Other};
    Type type;

    CatalogString source;
    CatalogString target;

    short score;

    QString lang;
    QString origin;
    QString phase;

    AltTrans(CatalogString source_=CatalogString()):type(Other),source(source_){}
};


#endif
