/*****************************************************************************
  This file is part of KAider
  This file contains parts of KBabel code

  Copyright (C) 1999-2000 by Matthias Kiefer <matthias.kiefer@gmx.de>
		2001-2004 by Stanislav Visnovsky <visnovsky@kde.org>
		2007	  by Nick Shaforostoff <shafff@ukr.net>

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





#ifndef DATAMODEL_H
#define DATAMODEL_H

#include <QUndoStack>
class QUndoCommand;

#include <kurl.h>

#include "pos.h"
#include "catalogitem.h"
#include "catalogfileplugin.h"
#include "catalog_private.h"
class CatalogStorage;
#include "pluralformtypes_enum.h"

#include "tagrange.h"

#define CHECK_IF_EMPTY(_return_type)\
if (  d->_entries.isEmpty() )\
        return _return_type;


//XLIFF, чтоб его...




/**
* This class represents a catalog
* It uses CatalogStorage interface to work with catalogs in different formats
* Also it defines all necessary functions to set and get the entries
*
* @short class, that represents a translation catalog
* @author Nick Shaforostoff <shafff@ukr.net>
*/
class Catalog: public QUndoStack
{
    Q_OBJECT

public:
    Catalog(QObject* parent);
    virtual ~Catalog();

    QString msgid(const DocPosition&, const bool noNewlines=false) const;
    QString msgstr(const DocPosition&, const bool noNewlines=false) const;
    QString source(const DocPosition& pos) const {return msgid(pos);}
    QString target(const DocPosition& pos) const {return msgstr(pos);}
    QString source(const DocPosition& pos, QList<TagRange>& ranges) const;
    QString target(const DocPosition& pos, QList<TagRange>& ranges) const;

    QString comment(uint index) const;
    QString msgctxt(uint index) const;

    PluralFormType pluralFormType(uint index) const;
    bool isPlural(uint index) const{return pluralFormType(index)==Gettext;}
    int numberOfPluralForms() const {return d->_numberOfPluralForms;}
    int numberOfEntries() const;
    int numberOfFuzzies() const {return d->_fuzzyIndex.size();}
    int numberOfUntranslated() const {return d->_untransIndex.size();}
    bool isFuzzy(uint index) const;
    bool isApproved(uint index) const{return !isFuzzy(index);}
    bool isApproved(const DocPosition pos) const{return !isFuzzy(pos.entry);}
    bool isUntranslated(uint index) const; //at least one form is untranslated
    bool isUntranslated(const DocPosition&) const;
    int firstFuzzyIndex() const {return d->_fuzzyIndex.isEmpty()?numberOfEntries():d->_fuzzyIndex.first();}
    int lastFuzzyIndex() const {return d->_fuzzyIndex.isEmpty()?-1:d->_fuzzyIndex.last();}
    int nextFuzzyIndex(uint index) const {return findNextInList(d->_fuzzyIndex,index);}
    int prevFuzzyIndex(uint index) const {return findPrevInList(d->_fuzzyIndex,index);}
    int firstUntranslatedIndex() const {return d->_untransIndex.isEmpty()?numberOfEntries():d->_untransIndex.first();}
    int lastUntranslatedIndex() const {return d->_untransIndex.isEmpty()?-1:d->_untransIndex.last();}
    int nextUntranslatedIndex(uint index) const {return findNextInList(d->_untransIndex,index);}
    int prevUntranslatedIndex(uint index) const {return findPrevInList(d->_untransIndex,index);}

    int firstBookmarkIndex() const {return d->_bookmarkIndex.isEmpty()?numberOfEntries():d->_bookmarkIndex.first();}
    int lastBookmarkIndex() const {return d->_bookmarkIndex.isEmpty()?-1:d->_bookmarkIndex.last();}
    int nextBookmarkIndex(uint index) const {return findNextInList(d->_bookmarkIndex,index);}
    int prevBookmarkIndex(uint index) const {return findPrevInList(d->_bookmarkIndex,index);}
    bool isBookmarked(uint index) const{return d->_bookmarkIndex.contains(index);}

    void clear();
    bool isEmpty(){return !m_storage;}

//    void setFileCodec( QTextCodec* codec ){d->fileCodec = codec;}
//    QTextCodec* fileCodec() const {return d->fileCodec;}

    void setPackageName( QString s ){d->_packageName = s;}
    //QString _packageName() const {return d->fileCodec;}


    void setErrorIndex(const QList<int>& errors){d->_errorIndex=errors;}

    void setImportPluginID(const QString& id){d->_importID=id;}
    const QString& importPluginID() const {return d->_importID;}

    //void setMimeTypes(const QString& mimeTypes){d->_mimeTypes=mimeTypes;}
    QString mimetype();

    const KUrl& url() const {return d->_url;}
    void setUrl(const KUrl& u){d->_url=u;}//used for template load

    bool loadFromUrl(const KUrl& url);
    bool saveToUrl(KUrl url);

    void setBookmark(uint,bool);

    //void updateHeader(bool forSaving=true);

public:
    virtual const DocPosition& undo();
    virtual const DocPosition& redo();
    virtual void push(QUndoCommand *cmd,bool rebaseForDBUpdate=false);

public slots:
    bool save();

    //updates DB for _posBuffer and accompanying _originalForLastModified
    void flushUpdateDBBuffer();

protected:
    //EDITING
    //(accessed from undo/redo code)
    //called _BEFORE_ modification
    void setLastModifiedPos(const DocPosition&);

    //EDITING
    //(accessed from undo/redo code)
    //(accessed from mergeCatalog)
    void setApproved(const DocPosition& pos, bool approved);//_checks_ if action should be taken
    void targetDelete(const DocPosition& pos, int count);
    void targetInsert(const DocPosition& pos, const QString& arg);


protected:
    int findPrevInList(const QList<int>& list,int index) const;
    int findNextInList(const QList<int>& list,int index) const;

//private:
protected:
    CatalogPrivate *d;
    CatalogStorage *m_storage;

    friend class CatalogImportPlugin;
    friend class CatalogExportPlugin;
    friend class InsTextCmd;
    friend class DelTextCmd;
    friend class ToggleFuzzyCmd;
    friend class ToggleApprovementCmd;
    friend class MergeCatalog;

signals:
    void signalEntryChanged(const DocPosition&);
    void signalNumberOfFuzziesChanged();
    void signalNumberOfUntranslatedChanged();
    void signalFileLoaded();
    void signalFileLoaded(const KUrl&);
    void signalFileSaved();
    void signalFileSaved(const KUrl&);

    //void signalCmdPushed(QUndoCommand *cmd);//for merging (on-the-fly changes replication)


};

#endif

