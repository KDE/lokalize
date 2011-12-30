/*****************************************************************************
  This file is part of Lokalize
  This file contains parts of KBabel code

  Copyright (C) 1999-2000   by Matthias Kiefer <matthias.kiefer@gmx.de>
                2001-2004   by Stanislav Visnovsky <visnovsky@kde.org>
                2007-2009   by Nick Shaforostoff <shafff@ukr.net>

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

#ifndef CATALOG_H
#define CATALOG_H

#include "pos.h"
#include "catalogstring.h"
#include "catalogcapabilities.h"
#include "note.h"
#include "state.h"
#include "phase.h"
#include "alttrans.h"
#include "catalog_private.h"
class CatalogStorage;

#include <QUndoStack>
#include <kurl.h>

namespace GettextCatalog {
  class CatalogImportPlugin;
  class CatalogExportPlugin;
}


bool isApproved(TargetState state, ProjectLocal::PersonRole role);
bool isApproved(TargetState state); //disregarding Phase
TargetState closestState(bool approved, ProjectLocal::PersonRole role);
int findPrevInList(const QLinkedList<int>& list,int index);
int findNextInList(const QLinkedList<int>& list,int index);
void insertInList(QLinkedList<int>& list, int index); // insert index in the right place in the list


/**
 * This class represents a catalog
 * It uses CatalogStorage interface to work with catalogs in different formats
 * Also it defines all necessary functions to set and get the entries
 *
 * @short Wrapper class that represents a translation catalog
 * @author Nick Shaforostoff <shafff@ukr.net>
*/
class Catalog: public QUndoStack
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.Lokalize.FileContainer")

public:
    Catalog(QObject* parent);
    virtual ~Catalog();

    QString msgid(const DocPosition&) const;
    virtual QString msgstr(const DocPosition&) const;

    static QStringList supportedExtensions();
    static bool extIsSupported(const QString& path);
    static const char* const* states();

    int capabilities() const;

    void push(QUndoCommand* cmd);

public slots: //DBus interface
    QString source(const DocPosition& pos) const {return msgid(pos);}
    QString target(const DocPosition& pos) const {return msgstr(pos);}
    // used by XLIFF storage)
    CatalogString sourceWithTags(const DocPosition& pos) const;
    CatalogString targetWithTags(const DocPosition& pos) const;
    CatalogString catalogString(const DocPosition& pos) const;

    /**
     * @a pos.form is note number
     * @returns previous note contents, if any
     */
    Note setNote(const DocPosition& pos, const Note& note);
    QVector<Note> notes(const DocPosition& pos) const;
    QVector<Note> developerNotes(const DocPosition& pos) const;
    QStringList noteAuthors() const;
    QVector<AltTrans> altTrans(const DocPosition& pos) const;
    QStringList sourceFiles(const DocPosition& pos) const;
    //QString msgctxt(uint index) const;
    //the result is guaranteed to have at least 1 string
    QStringList context(const DocPosition& pos) const;
    QString id(const DocPosition& pos) const;
    ///@returns previous phase-name
    QString setPhase(const DocPosition& pos, const QString& phase);
    QString phase(const DocPosition& pos) const;
    QString activePhase() const{return d->_phase;}
    ProjectLocal::PersonRole activePhaseRole() const{return d->_phaseRole;}
    void setActivePhase(const QString& phase, ProjectLocal::PersonRole role=ProjectLocal::Approver);
    Phase phase(const QString& name) const;
    QList<Phase> allPhases() const;
    QMap<QString,Tool> allTools() const;
    QVector<Note> phaseNotes(const QString& phase) const;
    ///@arg pos.entry - number of phase, @arg pos.form - number of note
    QVector<Note> setPhaseNotes(const QString& phase, QVector<Note>);

    bool isPlural(uint index) const;
    bool isPlural(const DocPosition& pos) const{return isPlural(pos.entry);}
    bool isApproved(uint index) const;
    bool isApproved(const DocPosition& pos) const{return isApproved(pos.entry);}
    TargetState state(const DocPosition& pos) const;
    bool isEquivTrans(const DocPosition&) const;
    ///@returns true if at least one form is untranslated
    bool isEmpty(uint index) const;
    bool isEmpty(const DocPosition&) const;
    bool isModified(DocPos entry) const;
    bool isModified(int entry) const;

    /// so DocPosition::entry may actually be < size()+binUnitsCount()
    int binUnitsCount() const;

    int unitById(const QString& id) const;

    bool isBookmarked(uint index) const{return d->_bookmarkIndex.contains(index);}
    void setBookmark(uint, bool);

    int numberOfPluralForms() const {return d->_numberOfPluralForms;}
    int numberOfEntries() const;
    int numberOfNonApproved() const {return d->_nonApprovedIndex.size();}
    int numberOfUntranslated() const {return d->_emptyIndex.size();}


public:
    int firstFuzzyIndex() const {return d->_nonApprovedIndex.isEmpty()?numberOfEntries():d->_nonApprovedIndex.first();}
    int lastFuzzyIndex() const {return d->_nonApprovedIndex.isEmpty()?-1:d->_nonApprovedIndex.last();}
    int nextFuzzyIndex(uint index) const {return findNextInList(d->_nonApprovedIndex,index);}
    int prevFuzzyIndex(uint index) const {return findPrevInList(d->_nonApprovedIndex,index);}
    int firstUntranslatedIndex() const {return d->_emptyIndex.isEmpty()?numberOfEntries():d->_emptyIndex.first();}
    int lastUntranslatedIndex() const {return d->_emptyIndex.isEmpty()?-1:d->_emptyIndex.last();}
    int nextUntranslatedIndex(uint index) const {return findNextInList(d->_emptyIndex,index);}
    int prevUntranslatedIndex(uint index) const {return findPrevInList(d->_emptyIndex,index);}

    int firstBookmarkIndex() const {return d->_bookmarkIndex.isEmpty()?numberOfEntries():d->_bookmarkIndex.first();}
    int lastBookmarkIndex() const {return d->_bookmarkIndex.isEmpty()?-1:d->_bookmarkIndex.last();}
    int nextBookmarkIndex(uint index) const {return findNextInList(d->_bookmarkIndex,index);}
    int prevBookmarkIndex(uint index) const {return findPrevInList(d->_bookmarkIndex,index);}

    bool autoSaveRecovered(){return d->_autoSaveRecovered;}
public:
    void clear();
    bool isEmpty(){return !m_storage;}
    bool isReadOnly(){return d->_readOnly;}

    void attachAltTransCatalog(Catalog*);


    virtual const DocPosition& undo();
    virtual const DocPosition& redo();

    //void setErrorIndex(const QList<int>& errors){d->_errorIndex=errors;}
    void setUrl(const KUrl& u){d->_url=u;}//used for template load
public slots: //DBus interface
    const KUrl& url() const {return d->_url;}
    ///@returns 0 if success, >0 erroneous line (parsing error)
    int loadFromUrl(const KUrl& url, const KUrl& saidUrl=KUrl(), int* fileSize=0);
    bool saveToUrl(KUrl url);
    bool save();
    QByteArray contents();
    QString mimetype();
    QString sourceLangCode() const;
    QString targetLangCode() const;

protected:
    virtual KAutoSaveFile* checkAutoSave(const KUrl& url);

protected slots:
    /**
     * updates DB for _posBuffer and accompanying _originalForLastModified
     */
    void flushUpdateDBBuffer();

    void doAutoSave();
    void setAutoSaveDirty(){d->_autoSaveDirty=true;}
    
    void projectConfigChanged();

protected:
    /**
     * (EDITING)
     * accessed from undo/redo code
     * called _BEFORE_ modification
    */
    void setLastModifiedPos(const DocPosition&);

    /**
     * (EDITING)
     * accessed from undo/redo code
     * accessed from mergeCatalog)
     * it _does_ check if action should be taken
     */
    void setApproved(const DocPosition& pos, bool approved);
    void targetDelete(const DocPosition& pos, int count);
    void targetInsert(const DocPosition& pos, const QString& arg);
    InlineTag targetDeleteTag(const DocPosition& pos);
    void targetInsertTag(const DocPosition& pos, const InlineTag& tag);
    TargetState setState(const DocPosition& pos, TargetState state);
    Phase updatePhase(const Phase& phase);
    void setEquivTrans(const DocPosition&, bool equivTrans);

    /// @returns true if entry wasn't modified before
    bool setModified(DocPos entry, bool modif);
    
    void updateApprovedEmptyIndexCache();

protected:
    CatalogPrivate *d;
    CatalogStorage *m_storage;

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

signals:
    void signalEntryModified(const DocPosition&);
    void activePhaseChanged();
    void signalNumberOfFuzziesChanged();
    void signalNumberOfEmptyChanged();
    Q_SCRIPTABLE void signalFileLoaded();
    void signalFileLoaded(const KUrl&);
    Q_SCRIPTABLE void signalFileSaved();
    void signalFileSaved(const KUrl&);
    void signalFileAutoSaveFailed(const QString&);
};


#endif

