/*****************************************************************************
  This file is part of Lokalize
  This file contains parts of KBabel code

  SPDX-FileCopyrightText: 1999-2000 Matthias Kiefer <matthias.kiefer@gmx.de>
  SPDX-FileCopyrightText: 2001-2004 Stanislav Visnovsky <visnovsky@kde.org>
  SPDX-FileCopyrightText: 2007-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/

#ifndef CATALOG_H
#define CATALOG_H

#include "alttrans.h"
#include "catalog_private.h"
#include "catalogcapabilities.h"
#include "catalogstring.h"
#include "note.h"
#include "phase.h"
#include "pos.h"
#include "state.h"
class CatalogStorage;
class MassReplaceJob;
class KAutoSaveFile;

#include <QUndoStack>

namespace GettextCatalog
{
class CatalogImportPlugin;
class CatalogExportPlugin;
}

bool isApproved(TargetState state, ProjectLocal::PersonRole role);
bool isApproved(TargetState state); // disregarding Phase
TargetState closestState(bool approved, ProjectLocal::PersonRole role);
int findPrevInList(const std::list<int> &list, int index);
int findNextInList(const std::list<int> &list, int index);
void insertInList(std::list<int> &list, int index); // insert index in the right place in the list

/**
 * This class represents a catalog
 * It uses CatalogStorage interface to work with catalogs in different formats
 * Also it defines all necessary functions to set and get the entries
 *
 * @short Wrapper class that represents a translation catalog
 * @author Nick Shaforostoff <shafff@ukr.net>
 */
class Catalog : public QUndoStack
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.Lokalize.FileContainer")

public:
    explicit Catalog(QObject *parent);
    ~Catalog() override;

    QString msgid(const DocPosition &) const;
    virtual QString msgstr(const DocPosition &) const;
    QString msgidWithPlurals(const DocPosition &, bool truncateFirstLine) const;
    QString msgstrWithPlurals(const DocPosition &, bool truncateFirstLine) const;

    static QStringList supportedExtensions();
    static bool extIsSupported(const QString &path);
    static QStringList translatedStates();

    int capabilities() const;

    void push(QUndoCommand *cmd);

public Q_SLOTS: // DBus interface
    QString source(const DocPosition &pos) const
    {
        return msgid(pos);
    }
    QString target(const DocPosition &pos) const
    {
        return msgstr(pos);
    }
    // used by XLIFF storage)
    CatalogString sourceWithTags(const DocPosition &pos) const;
    CatalogString targetWithTags(const DocPosition &pos) const;
    CatalogString catalogString(const DocPosition &pos) const;

    /**
     * @a pos.form is note number
     * @returns previous note contents, if any
     */
    Note setNote(const DocPosition &pos, const Note &note);
    QVector<Note> notes(const DocPosition &pos) const;
    QVector<Note> developerNotes(const DocPosition &pos) const;
    QStringList noteAuthors() const;
    QVector<AltTrans> altTrans(const DocPosition &pos) const;
    QStringList sourceFiles(const DocPosition &pos) const;
    // the result is guaranteed to have at least 1 string
    QStringList context(const DocPosition &pos) const;
    QString id(const DocPosition &pos) const;
    ///@returns previous phase-name
    QString setPhase(const DocPosition &pos, const QString &phase);
    QString phase(const DocPosition &pos) const;
    QString activePhase() const
    {
        return d._phase;
    }
    ProjectLocal::PersonRole activePhaseRole() const
    {
        return d._phaseRole;
    }
    void setActivePhase(const QString &phase, ProjectLocal::PersonRole role = ProjectLocal::Approver);
    Phase phase(const QString &name) const;
    QList<Phase> allPhases() const;
    QMap<QString, Tool> allTools() const;
    QVector<Note> phaseNotes(const QString &phase) const;
    ///@arg pos.entry - number of phase, @arg pos.form - number of note
    QVector<Note> setPhaseNotes(const QString &phase, QVector<Note>);

    bool isPlural(uint index) const;
    bool isPlural(const DocPosition &pos) const
    {
        return isPlural(pos.entry);
    }
    bool isApproved(uint index) const;
    bool isApproved(const DocPosition &pos) const
    {
        return isApproved(pos.entry);
    }
    TargetState state(const DocPosition &pos) const;
    bool isEquivTrans(const DocPosition &) const;
    ///@returns true if at least one form is untranslated
    bool isEmpty(uint index) const;
    bool isEmpty(const DocPosition &) const;
    bool isModified(DocPos entry) const;
    bool isModified(int entry) const;
    bool isObsolete(int entry) const;

    /// so DocPosition::entry may actually be < size()+binUnitsCount()
    int binUnitsCount() const;

    int unitById(const QString &id) const;

    bool isBookmarked(uint index) const
    {
        return std::find(d._bookmarkIndex.begin(), d._bookmarkIndex.end(), index) != d._bookmarkIndex.end();
    }
    void setBookmark(uint, bool);

    int numberOfPluralForms() const
    {
        return d._numberOfPluralForms;
    }
    int numberOfEntries() const;
    int numberOfNonApproved() const
    {
        return d._nonApprovedNonEmptyIndex.size();
    }
    int numberOfUntranslated() const
    {
        return d._emptyIndex.size();
    }

public:
    QString originalOdfFilePath();
    void setOriginalOdfFilePath(const QString &);

    int firstFuzzyIndex() const
    {
        return d._nonApprovedIndex.empty() ? numberOfEntries() : d._nonApprovedIndex.front();
    }
    int lastFuzzyIndex() const
    {
        return d._nonApprovedIndex.empty() ? -1 : d._nonApprovedIndex.back();
    }
    int nextFuzzyIndex(uint index) const
    {
        return findNextInList(d._nonApprovedIndex, index);
    }
    int prevFuzzyIndex(uint index) const
    {
        return findPrevInList(d._nonApprovedIndex, index);
    }
    int firstUntranslatedIndex() const
    {
        return d._emptyIndex.empty() ? numberOfEntries() : d._emptyIndex.front();
    }
    int lastUntranslatedIndex() const
    {
        return d._emptyIndex.empty() ? -1 : d._emptyIndex.back();
    }
    int nextUntranslatedIndex(uint index) const
    {
        return findNextInList(d._emptyIndex, index);
    }
    int prevUntranslatedIndex(uint index) const
    {
        return findPrevInList(d._emptyIndex, index);
    }

    int firstBookmarkIndex() const
    {
        return d._bookmarkIndex.empty() ? numberOfEntries() : d._bookmarkIndex.front();
    }
    int lastBookmarkIndex() const
    {
        return d._bookmarkIndex.empty() ? -1 : d._bookmarkIndex.back();
    }
    int nextBookmarkIndex(uint index) const
    {
        return findNextInList(d._bookmarkIndex, index);
    }
    int prevBookmarkIndex(uint index) const
    {
        return findPrevInList(d._bookmarkIndex, index);
    }

    bool autoSaveRecovered()
    {
        return d._autoSaveRecovered;
    }

public:
    void clear();
    bool isEmpty()
    {
        return !m_storage;
    }
    bool isReadOnly()
    {
        return d._readOnly;
    }

    void attachAltTransCatalog(Catalog *);
    void attachAltTrans(int entry, const AltTrans &trans);

    virtual const DocPosition &undo();
    virtual const DocPosition &redo();

    void setTarget(DocPosition pos, const CatalogString &s); // for batch use only!

    // void setErrorIndex(const QList<int>& errors){d._errorIndex=errors;}
    void setUrl(const QString &u)
    {
        d._filePath = u; // used for template load
    }
public Q_SLOTS: // DBus interface
    const QString &url() const
    {
        return d._filePath;
    }
    ///@returns 0 if success, >0 erroneous line (parsing error)
    int loadFromUrl(const QString &url, const QString &saidUrl = QString(), int *fileSize = nullptr, bool fast = false);
    bool saveToUrl(QString url);
    bool save();
    QByteArray contents();
    QString mimetype();
    QString fileType();
    CatalogType type();
    QString sourceLangCode() const;
    QString targetLangCode() const;
    void setTargetLangCode(const QString &targetLangCode);
    /**
     * updates DB for _posBuffer and accompanying _originalForLastModified
     */
    void flushUpdateDBBuffer();

protected:
    virtual KAutoSaveFile *checkAutoSave(const QString &url);

protected Q_SLOTS:
    void doAutoSave();
    void setAutoSaveDirty()
    {
        d._autoSaveDirty = true;
    }

    void projectConfigChanged();

protected:
    /**
     * (EDITING)
     * accessed from undo/redo code
     * called _BEFORE_ modification
     */
    void setLastModifiedPos(const DocPosition &);

    /**
     * (EDITING)
     * accessed from undo/redo code
     * accessed from mergeCatalog)
     * it _does_ check if action should be taken
     */
    void setApproved(const DocPosition &pos, bool approved);
    void targetDelete(const DocPosition &pos, int count);
    void targetInsert(const DocPosition &pos, const QString &arg);
    InlineTag targetDeleteTag(const DocPosition &pos);
    void targetInsertTag(const DocPosition &pos, const InlineTag &tag);
    TargetState setState(const DocPosition &pos, TargetState state);
    Phase updatePhase(const Phase &phase);
    void setEquivTrans(const DocPosition &, bool equivTrans);

    /// @returns true if entry wasn't modified before
    bool setModified(DocPos entry, bool modif);

    void updateApprovedEmptyIndexCache();

protected:
    CatalogPrivate d;
    CatalogStorage *m_storage{};

    friend class GettextCatalog::CatalogImportPlugin;
    friend class GettextCatalog::CatalogExportPlugin;
    friend class LokalizeUnitCmd;
    friend class InsTextCmd;
    friend class DelTextCmd;
    friend class InsTagCmd;
    friend class DelTagCmd;
    friend class SetStateCmd;
    friend class SetNoteCmd;
    friend class UpdatePhaseCmd;
    friend class MergeCatalog;
    friend class SetEquivTransCmd;
    friend class MassReplaceJob;

public:
    static QString supportedFileTypes(bool includeTemplates = true);

Q_SIGNALS:
    void signalEntryModified(const DocPosition &);
    void activePhaseChanged();
    void signalNumberOfFuzziesChanged();
    void signalNumberOfEmptyChanged();
    Q_SCRIPTABLE void signalFileLoaded();
    void signalFileLoaded(const QString &);
    Q_SCRIPTABLE void signalFileSaved();
    void signalFileSaved(const QString &);
    void signalFileAutoSaveFailed(const QString &);
};

#endif
