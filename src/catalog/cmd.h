/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2009 by Nick Shaforostoff <shafff@ukr.net>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

  In addition, as a special exception, the copyright holders give
  permission to link the code of this program with any edition of
  the Qt library by Trolltech AS, Norway (or with modified versions
  of Qt that use the same license as Qt), and distribute linked
  combinations including the two.  You must obey the GNU General
  Public License in all respects for all of the code used other than
  Qt. If you modify this file, you may extend this exception to
  your version of the file, but you are not obligated to do so.  If
  you do not wish to do so, delete this exception statement from
  your version.

**************************************************************************** */



#ifndef CMD_H
#define CMD_H

#include <QUndoCommand>

#include "pos.h"
#include "catalogstring.h"
class Catalog;

enum Commands { Insert, Delete, ToggleApprovement, InsertTag, DeleteTag };

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
