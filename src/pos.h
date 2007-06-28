#ifndef POS_H
#define POS_H

#include <QtCore>

enum Part {UndefPart, Msgid, Msgstr, Comment};

/**
* This struct represents a position in a catalog.
* A position is a tuple (index,pluralform,textoffset).
*
* @short Structure, that represents a position in a catalog.
* @author Matthias Kiefer <matthias.kiefer@gmx.de>
* @author Stanislav Visnovsky <visnovsky@kde.org>
*/
struct DocPosition
{
    Part part:8;
    uchar form:8;
    short entry:16;
    uint offset:32;

    DocPosition():
        part(Msgstr),
        form(0),
        entry(-1), 
        offset(0)
        {}
};

#endif
