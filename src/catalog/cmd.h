/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2009 by Nick Shaforostoff <shafff@ukr.net>

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


#ifndef CMD_H
#define CMD_H

#include <QUndoCommand>

#include "pos.h"
#include "note.h"
#include "catalogstring.h"
class Catalog;

enum Commands { Insert,Delete,
                ToggleApprovement,
                InsertTag, DeleteTag,
                SetNote
                };

/**
 * how undo system works:
 * undo() and redo() functions call appropriate private method of Catalog to change catalog contents,
 * then set DocPosition (posBuffer var in Catalog), which is used to navigate editor to appr. place
 * @short Do insert text
 */
class InsTextCmd: public QUndoCommand
{
public:
    InsTextCmd(Catalog *catalog, const DocPosition& pos, const QString& str);
    ~InsTextCmd(){};
    int id () const {return Insert;}
    bool mergeWith(const QUndoCommand *other);
    void undo();
    void redo();
private:
    Catalog* _catalog;
    QString _str;
    DocPosition _pos;
    bool _firstModificationForThisEntry;
};

/// @see InsTextCmd
class DelTextCmd: public QUndoCommand
{
public:
    DelTextCmd(Catalog *catalog, const DocPosition& pos ,const QString& str);
    ~DelTextCmd(){};
    int id () const {return Delete;}
    bool mergeWith(const QUndoCommand *other);
    void redo();
    void undo();
private:
    Catalog* _catalog;
    QString _str;
    DocPosition _pos;
    bool _firstModificationForThisEntry;
};


/**
 * you should care not to new it w/ aint no need
 */
class ToggleApprovementCmd: public QUndoCommand
{
public:
    ToggleApprovementCmd(Catalog *catalog, uint index, bool approved);
    ~ToggleApprovementCmd(){};
    int id () const {return ToggleApprovement;}
    void redo();
    void undo();
private:
    void setJumpingPos();

    Catalog* _catalog;
    short _index:16;
    bool _approved:8;
    bool _firstModificationForThisEntry:8;

};


/**
 * @short Do insert tag
 */
class InsTagCmd: public QUndoCommand
{
public:
    /// offset is taken from @a tag and not from @a pos
    InsTagCmd(Catalog *catalog, const DocPosition& pos, const TagRange& tag);
    ~InsTagCmd(){};
    int id () const {return InsertTag;}
    void undo();
    void redo();
private:
    Catalog* _catalog;
    TagRange _tag;
    DocPosition _pos;
    bool _firstModificationForThisEntry;
};

/**
 * TagRange is filled from document
 *
 * @short Do delete tag
 */
class DelTagCmd: public QUndoCommand
{
public:
    DelTagCmd(Catalog *catalog, const DocPosition& pos);
    ~DelTagCmd(){};
    int id () const {return DeleteTag;}
    void undo();
    void redo();
    TagRange tag()const{return _tag;}//used to get proprties of deleted tag
private:
    Catalog* _catalog;
    TagRange _tag;
    DocPosition _pos;
    bool _firstModificationForThisEntry;
};


#endif // CMD_H


/**
 * @short Insert or remove (if content is empty) a note
 */
class SetNoteCmd: public QUndoCommand
{
public:
    /// @a pos.form is note number
    SetNoteCmd(Catalog *catalog, const DocPosition& pos, const Note& note);
    ~SetNoteCmd(){};
    int id () const {return SetNote;}
    void undo();
    void redo();
private:
    Catalog* _catalog;
    Note _note;
    Note _oldNote;
    DocPosition _pos;
    bool _firstModificationForThisEntry;
};
