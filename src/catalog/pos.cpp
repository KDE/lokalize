/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007      Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/

#include "pos.h"
#include "catalog.h"
#include "config-lokalize.h"

bool switchPrev(const Catalog *&catalog, DocPosition &pos, int parts)
{
    bool switchEntry = false;
    bool switchCommentIndex = false;
    if (pos.part == DocPosition::Comment)
        switchCommentIndex = true;
    else if (pos.part == DocPosition::Target) {
        if (parts & DocPosition::Source)
            pos.part = DocPosition::Source;
        switchEntry = !(parts & DocPosition::Source);
    } else if (pos.part == DocPosition::Source)
        switchEntry = true;

    bool skipCommentThisTime = false;
    if (switchCommentIndex) {
        if (pos.form)
            pos.form--;
        switchEntry = pos.form; // pos.form is zero again
        skipCommentThisTime = pos.form;
    }

    if (!switchEntry)
        return true;

    if (Q_UNLIKELY(pos.form > 0 && catalog->isPlural(pos.entry)))
        pos.form--;
    else if (Q_UNLIKELY(pos.entry == 0))
        return false;
    else {
        pos.entry--;
        pos.form = catalog->isPlural(pos.entry) * (catalog->numberOfPluralForms() - 1);
    }
    pos.offset = 0;

    if (parts & DocPosition::Comment && !skipCommentThisTime && pos.form == 0 && catalog->notes(pos).size()) {
        pos.part = DocPosition::Comment;
        pos.form = catalog->notes(pos).size() - 1;
    } else
        pos.part = DocPosition::Target;

    return true;
}

bool switchNext(const Catalog *&catalog, DocPosition &pos, int parts)
{
    bool switchEntry = false;
    bool switchCommentIndex = false;
    if (pos.part == DocPosition::Source)
        pos.part = DocPosition::Target;
    else if (pos.part == DocPosition::Target) {
        if (parts & DocPosition::Comment && pos.form == 0 && catalog->notes(pos).size())
            pos.part = DocPosition::Comment;
        else
            switchEntry = true;
    } else if (pos.part == DocPosition::Comment)
        switchCommentIndex = true;

    if (switchCommentIndex) {
        pos.form++;
        if (catalog->notes(pos).size() == pos.form) {
            pos.form = 0;
            switchEntry = true;
        }
    }

    if (!switchEntry)
        return true;

    if (Q_UNLIKELY(pos.entry != -1 && pos.form + 1 < catalog->numberOfPluralForms() && catalog->isPlural(pos.entry)))
        pos.form++;
    else if (Q_UNLIKELY(pos.entry == catalog->numberOfEntries() - 1))
        return false;
    else {
        pos.entry++;
        pos.form = 0;
    }
    pos.offset = 0;

    pos.part = (parts & DocPosition::Source) ? DocPosition::Source : DocPosition::Target;

    return true;
}

#if HAVE_DBUS
#include <QDBusArgument>
const QDBusArgument &operator>>(const QDBusArgument &argument, DocPosition &pos)
{
    int entry;
    int form;
    uint offset;

    argument.beginStructure();
    argument >> entry >> form >> offset;
    argument.endStructure();

    pos.entry = entry;
    pos.form = form;
    pos.offset = offset;

    return argument;
}

QDBusArgument &operator<<(QDBusArgument &argument, const DocPosition &pos)
{
    int entry = pos.entry;
    int form = pos.form;
    uint offset = pos.offset;

    argument.beginStructure();
    argument << entry << form << offset;
    argument.endStructure();

    return argument;
}
#endif
