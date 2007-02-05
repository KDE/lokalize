#ifndef POS_H
#define POS_H


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
    Part part;
    short form;
    int entry;
    uint offset;

    DocPosition(): offset(0), part(Msgstr), entry(-1), form(0) {}
};

#endif
