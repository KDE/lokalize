/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2011 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef CMD_H
#define CMD_H

#include <QUndoCommand>

#include "catalogstring.h"
#include "note.h"
#include "phase.h"
#include "pos.h"
#include "state.h"
class Catalog;

enum Commands {
    Insert,
    Delete,
    InsertTag,
    DeleteTag,
    ToggleApprovement,
    EquivTrans,
    SetNote,
    UpdatePhase,
};

class LokalizeUnitCmd : public QUndoCommand
{
public:
    LokalizeUnitCmd(Catalog *catalog, const DocPosition &pos, const QString &name);
    ~LokalizeUnitCmd() override = default;
    void undo() override;
    void redo() override;
    DocPosition pos() const
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
    Catalog *_catalog{nullptr};
    DocPosition _pos;
    bool _firstModificationForThisEntry{false};
    //    QString _prevPhase; currently xliffstorage doesn't support non-target phase setting
};

class LokalizeTargetCmd : public LokalizeUnitCmd
{
public:
    LokalizeTargetCmd(Catalog *catalog, const DocPosition &pos, const QString &name);
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
class InsTextCmd : public LokalizeTargetCmd
{
public:
    InsTextCmd(Catalog *catalog, const DocPosition &pos, const QString &str);
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
class DelTextCmd : public LokalizeTargetCmd
{
public:
    DelTextCmd(Catalog *catalog, const DocPosition &pos, const QString &str);
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

class SetStateCmd : public LokalizeUnitCmd
{
private:
    SetStateCmd(Catalog *catalog, const DocPosition &pos, TargetState state);

public:
    ~SetStateCmd() override = default;

    int id() const override
    {
        return ToggleApprovement;
    }
    void doRedo() override;
    void doUndo() override;

    static void push(Catalog *catalog, const DocPosition &pos, bool approved);
    static void instantiateAndPush(Catalog *catalog, const DocPosition &pos, TargetState state);

    TargetState _state;
    TargetState _prevState{SignedOff};
};

/// @short Do insert tag
class InsTagCmd : public LokalizeTargetCmd
{
public:
    /// offset is taken from @a tag and not from @a pos
    InsTagCmd(Catalog *catalog, const DocPosition &pos, const InlineTag &tag);
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
class DelTagCmd : public LokalizeTargetCmd
{
public:
    DelTagCmd(Catalog *catalog, const DocPosition &pos);
    ~DelTagCmd() override = default;
    int id() const override
    {
        return DeleteTag;
    }
    void doRedo() override;
    void doUndo() override;
    InlineTag tag() const
    {
        return _tag; // used to get proprties of deleted tag
    }

private:
    InlineTag _tag;
};

/// @short Insert or remove (if content is empty) a note
class SetNoteCmd : public LokalizeUnitCmd
{
public:
    /// @a pos.form is note number
    SetNoteCmd(Catalog *catalog, const DocPosition &pos, const Note &note);
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
class UpdatePhaseCmd : public QUndoCommand
{
public:
    /// @a pos.form is note number
    UpdatePhaseCmd(Catalog *catalog, const Phase &phase);
    ~UpdatePhaseCmd() override = default;
    int id() const override
    {
        return UpdatePhase;
    }
    void redo() override;
    void undo() override;

private:
    Catalog *_catalog{nullptr};
    Phase _phase;
    Phase _prevPhase;
};

class SetEquivTransCmd : public LokalizeTargetCmd
{
public:
    SetEquivTransCmd(Catalog *catalog, const DocPosition &pos, bool equivTrans);
    ~SetEquivTransCmd() override = default;
    int id() const override
    {
        return EquivTrans;
    }
    void doRedo() override;
    void doUndo() override;

private:
    bool _equivTrans{};
};

/**
 * CatalogString cmds helper function.
 *
 * tagPlaces: pos -> int:
 * >0 if both start and end parts of tag were (to be) deleted
 * 1 means this is start, 2 means this is end
 * @returns false if it can't find second part of any paired tag in the range
 */
bool fillTagPlaces(QMap<int, int> &tagPlaces, const CatalogString &catalogString, int start, int len);
bool removeTargetSubstring(Catalog *catalog, DocPosition pos, int delStart = 0, int delLen = -1);
void insertCatalogString(Catalog *catalog, DocPosition pos, const CatalogString &catStr, int start = 0);

#endif // CMD_H
