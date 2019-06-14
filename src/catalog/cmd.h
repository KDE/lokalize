/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2011 by Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

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
#include "phase.h"
#include "state.h"
#include "catalogstring.h"
class Catalog;

enum Commands {
    Insert, Delete,
    InsertTag, DeleteTag,
    ToggleApprovement, EquivTrans,
    SetNote, UpdatePhase
};

class LokalizeUnitCmd: public QUndoCommand
{
public:
    LokalizeUnitCmd(Catalog *catalog, const DocPosition& pos, const QString& name);
    ~LokalizeUnitCmd() override = default;
    void undo() override;
    void redo() override;
    DocPosition pos()const
    {
        return _pos;
    }
protected:
    virtual void doRedo() = 0;
    virtual void doUndo() = 0;
    /**
     * may be overridden to set customized pos
     * alternatively customized pos may be set manually in do*()
     */
    virtual void setJumpingPos();
protected:
    Catalog* _catalog;
    DocPosition _pos;
    bool _firstModificationForThisEntry;
//    QString _prevPhase; currently xliffstorage doesn't support non-target phase setting
};

class LokalizeTargetCmd: public LokalizeUnitCmd
{
public:
    LokalizeTargetCmd(Catalog *catalog, const DocPosition& pos, const QString& name);
    ~LokalizeTargetCmd() override = default;
    void undo() override;
    void redo() override;
protected:
    QString _prevTargetPhase;
};

/**
 * how undo system works:
 * undo() and redo() functions call appropriate private method of Catalog to change catalog contents,
 * then set DocPosition (posBuffer var in Catalog), which is used to navigate editor to appr. place
 * @short Do insert text
 */
class InsTextCmd: public LokalizeTargetCmd
{
public:
    InsTextCmd(Catalog *catalog, const DocPosition& pos, const QString& str);
    ~InsTextCmd() override = default;
    int id() const override
    {
        return Insert;
    }
    bool mergeWith(const QUndoCommand *other) override;
    void doRedo() override;
    void doUndo() override;
private:
    QString _str;
};

/// @see InsTextCmd
class DelTextCmd: public LokalizeTargetCmd
{
public:
    DelTextCmd(Catalog *catalog, const DocPosition& pos, const QString& str);
    ~DelTextCmd() override = default;
    int id() const override
    {
        return Delete;
    }
    bool mergeWith(const QUndoCommand *other) override;
    void doRedo() override;
    void doUndo() override;
private:
    QString _str;
};

class SetStateCmd: public LokalizeUnitCmd
{
private:
    SetStateCmd(Catalog *catalog, const DocPosition& pos, TargetState state);
public:
    ~SetStateCmd() override = default;

    int id() const override
    {
        return ToggleApprovement;
    }
    void doRedo() override;
    void doUndo() override;

    static void push(Catalog *catalog, const DocPosition& pos, bool approved);
    static void instantiateAndPush(Catalog *catalog, const DocPosition& pos, TargetState state);

    TargetState _state;
    TargetState _prevState;
};

/// @short Do insert tag
class InsTagCmd: public LokalizeTargetCmd
{
public:
    /// offset is taken from @a tag and not from @a pos
    InsTagCmd(Catalog *catalog, const DocPosition& pos, const InlineTag& tag);
    ~InsTagCmd() override = default;
    int id() const override
    {
        return InsertTag;
    }
    void doRedo() override;
    void doUndo() override;
private:
    InlineTag _tag;
};

/**
 * TagRange is filled from document
 *
 * @short Do delete tag
 */
class DelTagCmd: public LokalizeTargetCmd
{
public:
    DelTagCmd(Catalog *catalog, const DocPosition& pos);
    ~DelTagCmd() override = default;
    int id() const override
    {
        return DeleteTag;
    }
    void doRedo() override;
    void doUndo() override;
    InlineTag tag()const
    {
        return _tag;   //used to get proprties of deleted tag
    }
private:
    InlineTag _tag;
};

/// @short Insert or remove (if content is empty) a note
class SetNoteCmd: public LokalizeUnitCmd
{
public:
    /// @a pos.form is note number
    SetNoteCmd(Catalog *catalog, const DocPosition& pos, const Note& note);
    ~SetNoteCmd() override = default;
    int id() const override
    {
        return SetNote;
    }
protected:
    void doRedo() override;
    void doUndo() override;
    void setJumpingPos() override;
private:
    Note _note;
    Note _prevNote;
};

/// @short Add or remove (if content is empty) a phase
class UpdatePhaseCmd: public QUndoCommand
{
public:
    /// @a pos.form is note number
    UpdatePhaseCmd(Catalog *catalog, const Phase& phase);
    ~UpdatePhaseCmd() override = default;
    int id() const override
    {
        return UpdatePhase;
    }
    void redo() override;
    void undo() override;
private:
    Catalog* _catalog;
    Phase _phase;
    Phase _prevPhase;
};


class SetEquivTransCmd: public LokalizeTargetCmd
{
public:
    SetEquivTransCmd(Catalog *catalog, const DocPosition& pos, bool equivTrans);
    ~SetEquivTransCmd() override = default;
    int id() const override
    {
        return EquivTrans;
    }
    void doRedo() override;
    void doUndo() override;
private:
    bool _equivTrans;
};

/**
 * CatalogString cmds helper function.
 *
 * tagPlaces: pos -> int:
 * >0 if both start and end parts of tag were (to be) deleted
 * 1 means this is start, 2 means this is end
 * @returns false if it can't find second part of any paired tag in the range
 */
bool fillTagPlaces(QMap<int, int>& tagPlaces, const CatalogString& catalogString, int start, int len);
bool removeTargetSubstring(Catalog* catalog, DocPosition pos, int delStart = 0, int delLen = -1);
void insertCatalogString(Catalog* catalog, DocPosition pos, const CatalogString& catStr, int start = 0);

#endif // CMD_H
