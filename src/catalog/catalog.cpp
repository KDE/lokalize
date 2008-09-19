/* ****************************************************************************
  This file is part of Lokalize
  This file contains parts of KBabel code

  Copyright (C) 1999-2000   by Matthias Kiefer <matthias.kiefer@gmx.de>
                2001-2005   by Stanislav Visnovsky <visnovsky@kde.org>
                2006        by Nicolas Goutte <goutte@kde.org>
                2007-2008   by Nick Shaforostoff <shafff@ukr.net>

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
// #define KDE_NO_DEBUG_OUTPUT
#define XLIFF 1

#include "catalog.h"
#include "catalog_private.h"
#include "project.h"

#include "catalogstorage.h"
#include "gettextstorage.h"
#include "gettextimport.h"
#include "gettextexport.h"

#ifdef XLIFF
#include "xliffstorage.h"
#endif


#include "version.h"
#include "prefs_lokalize.h"
#include "jobs.h"

#include <threadweaver/ThreadWeaver.h>

#include <QProcess>
#include <QString>
#include <QMap>

#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
// #include <kmessagebox.h>
#include <kdatetime.h>

#include <kio/netaccess.h>
#include <ktemporaryfile.h>


Catalog::Catalog(QObject *parent)
    : QUndoStack(parent)
    , d(new CatalogPrivate())
    , m_storage(0)
{
    //cause refresh events for files modified from lokalize itself aint delivered automatically
    connect(this,SIGNAL(signalFileSaved(KUrl)),
            Project::instance()->model()->dirLister(),SLOT(slotFileSaved(KUrl)));

}

Catalog::~Catalog()
{
    delete d;
    delete m_storage;
}


void Catalog::clear()
{
    QUndoStack::clear();
    d->_errorIndex.clear();
    d->_fuzzyIndex.clear();
    d->_untransIndex.clear();
    delete m_storage;m_storage=0;
    d->_url.clear();
    d->_lastModifiedPos=DocPosition();
/*
    d->msgidDiffList.clear();
    d->msgstr2MsgidDiffList.clear();
    d->diffCache.clear();
    */
}


//BEGIN STORAGE TRANSLATION

int Catalog::numberOfEntries() const
{
    if (KDE_ISUNLIKELY( !m_storage ))
        return 0;

    return m_storage->size();
}

QString Catalog::msgid(const DocPosition& pos, const bool noNewlines) const
{
    if (KDE_ISUNLIKELY( !m_storage || m_storage->isEmpty() ))
        return d->CatalogPrivate::_emptyStr;

    //if source lang is english (implied) and target lang has only 1 plural form (e.g. Chinese)
    if (KDE_ISUNLIKELY(d->_numberOfPluralForms==1))
    {
        DocPosition newPos=pos;
        newPos.form=1;
        return m_storage->source(newPos);
    }

    return m_storage->source(pos);
}

QString Catalog::msgstr(const DocPosition& pos, const bool noNewlines) const
{
    if (KDE_ISUNLIKELY( !m_storage || m_storage->isEmpty() ))
        return d->CatalogPrivate::_emptyStr;

   return m_storage->target(pos);

}


CatalogString Catalog::sourceWithTags(const DocPosition& pos) const
{
    if (KDE_ISUNLIKELY( !m_storage || m_storage->isEmpty() ))
        return CatalogString();

    return m_storage->sourceWithTags(pos);

}
CatalogString Catalog::targetWithTags(const DocPosition& pos) const
{
    if (KDE_ISUNLIKELY( !m_storage || m_storage->isEmpty() ))
        return CatalogString();

    return m_storage->targetWithTags(pos);
}


QString Catalog::comment(uint index) const
{
    if (KDE_ISUNLIKELY( !m_storage || m_storage->isEmpty() ))
        return d->CatalogPrivate::_emptyStr;

    DocPosition pos(index);
    return m_storage->noteCount(pos)?
                m_storage->note(pos):
                d->CatalogPrivate::_emptyStr;
}

QString Catalog::msgctxt(uint index) const
{
    if (KDE_ISUNLIKELY( !m_storage || m_storage->isEmpty() ))
        return d->CatalogPrivate::_emptyStr;

    DocPosition pos(index);
    return m_storage->contextCount(pos)?
                m_storage->context(pos):
                d->CatalogPrivate::_emptyStr;
}

PluralFormType Catalog::pluralFormType(uint index) const
{
    if (KDE_ISUNLIKELY( !m_storage || m_storage->isEmpty() ))
        return NoPluralForm;

//    uint max=m_storage->count()-1;
//    if(index > max)
//       index=max;
    return m_storage->isPlural(DocPosition(index))?Gettext:NoPluralForm;
}

bool Catalog::isFuzzy(uint index) const
{
    if (KDE_ISUNLIKELY( !m_storage || m_storage->isEmpty() ))
        return false;

    return !m_storage->isApproved(DocPosition(index));
}

bool Catalog::isUntranslated(uint index) const
{
    if (KDE_ISUNLIKELY( !m_storage || m_storage->isEmpty() ))
        return false;

    return m_storage->isUntranslated(DocPosition(index));
}

bool Catalog::isUntranslated(const DocPosition& pos) const
{
    if (KDE_ISUNLIKELY( !m_storage || m_storage->isEmpty() ))
        return false;

    return m_storage->isUntranslated(pos);
}


QString Catalog::mimetype()
{
    if (KDE_ISUNLIKELY( !m_storage || m_storage->isEmpty() ))
        return false;

    return m_storage->mimetype();
}

//END STORAGE TRANSLATION

//BEGIN OPEN/SAVE

bool Catalog::loadFromUrl(const KUrl& url)
{
    CatalogStorage* storage=0;

    if (url.fileName().endsWith(".po")||url.fileName().endsWith(".pot"))
        storage=new GettextCatalog::GettextStorage;
#ifdef XLIFF
    else if (url.fileName().endsWith(".xlf")||url.fileName().endsWith(".xliff"))
        storage=new XliffStorage;
#endif
    else
        return false;

    if (KDE_ISUNLIKELY( !storage->load(url) ))
        return false;

    //ok...
    clear();
    d->_url=url;
    //Plurals
    d->_numberOfPluralForms = storage->numberOfPluralForms();

    //index cache TODO profile?
    QList<int>& fuzzyIndex=d->_fuzzyIndex;
    QList<int>& untransIndex=d->_untransIndex;
    fuzzyIndex.clear();
    untransIndex.clear();

    DocPosition pos(0);
    int limit=storage->size();
    while(pos.entry<limit)
    {
        if (!storage->isApproved(pos))
            fuzzyIndex << pos.entry;
        if (storage->isUntranslated(pos))
            untransIndex << pos.entry;

        ++(pos.entry);
    }

    //commit transaction
    m_storage=storage;
    emit signalFileLoaded();
    emit signalFileLoaded(url);
    return true;
}

bool Catalog::save()
{
    return saveToUrl(d->_url);
}

bool Catalog::saveToUrl(KUrl url)
{
    if (KDE_ISUNLIKELY( !m_storage ))
        return true;

    bool nameChanged=false;
    if (KDE_ISLIKELY( url.isEmpty() ))
        url = d->_url;
    else
        nameChanged=true;

    if (KDE_ISLIKELY( m_storage->save(url) ))
    {
        setClean();
        if (nameChanged)
            d->_url=url;

//         Settings::self()->setCurrentGroup("Bookmarks");
//         Settings::self()->addItemIntList(d->_url.url(),d->_bookmarkIndex);

        emit signalFileSaved();
        emit signalFileSaved(url);
        return true;
    }/*
    else if (status==NO_PERMISSIONS)
    {
        if (KMessageBox::warningContinueCancel(this,
	     i18n("You do not have permission to write to file:\n%1\n"
		  "Do you want to save to another file or cancel?", _currentURL.prettyUrl()),
	     i18n("Error"),KStandardGuiItem::save())==KMessageBox::Continue)
            return fileSaveAs();

    }
*/
    return false;

}
//END OPEN/SAVE



    /**
     * helper method to keep db in a good shape :)
     * called on
     * 1) entry switch
     * 2) automatic editing code like replace or undo/redo operation
    **/
static void updateDB(
              const QString& filePath,
              const QString& ctxt,
              const QString& english,
              /*const QString& oldTarget,*/
              const QString& newTarget,
              int form,
              bool approved
              //const DocPosition&,//for back tracking
//              const QString& dbName,
             )
{
    TM::UpdateJob* j=new TM::UpdateJob(filePath,ctxt,english,/*oldTarget,*/newTarget,form,approved,
                               Project::instance()->projectID());
    j->connect(j,SIGNAL(failed(ThreadWeaver::Job*)),j,SLOT(deleteLater()));
    j->connect(j,SIGNAL(done(ThreadWeaver::Job*)),j,SLOT(deleteLater()));
    ThreadWeaver::Weaver::instance()->enqueue(j);
}


//BEGIN UNDO/REDO
const DocPosition& Catalog::undo()
{
    QUndoStack::undo();
    return d->_lastModifiedPos;
}

const DocPosition& Catalog::redo()
{
    QUndoStack::redo();
    return d->_lastModifiedPos;
}


void Catalog::push(QUndoCommand *cmd, bool rebaseForDBUpdate)
{
    QUndoStack::push(cmd);
}

void Catalog::flushUpdateDBBuffer()
{
    if (!Settings::autoaddTM())
        return;

    DocPosition pos=d->_lastModifiedPos;
    if (pos.entry==-1 || pos.entry>=numberOfEntries())
    {
        //nothing to flush
        kWarning()<<"nothing to flush or new file opened";
        return;
    }
    kWarning()<<"updating!!";
    int form=-1;
    if (isPlural(pos.entry))
        form=pos.form;
    updateDB(url().pathOrUrl(),
             msgctxt(pos.entry),
             source(pos),
             target(pos),
             form,
             isApproved(pos.entry));

}

void Catalog::setLastModifiedPos(const DocPosition& pos)
{
    bool entryChanged=DocPos(d->_lastModifiedPos)!=DocPos(pos);
    if (entryChanged)
        flushUpdateDBBuffer();

    d->_lastModifiedPos=pos;
}

void Catalog::targetDelete(const DocPosition& pos, int count)
{
    if (KDE_ISUNLIKELY( !m_storage ))
        return;

    m_storage->targetDelete(pos,count);

    //BEGIN addToUntransIndex
    if ((!pos.offset)&&(isUntranslated(pos)))
    {

        // insert index in the right place in the list
        QList<int>::Iterator it = d->_untransIndex.begin();
        while(it != d->_untransIndex.end() && pos.entry > (int)*it)
            ++it;
        d->_untransIndex.insert(it,pos.entry);
        emit signalNumberOfUntranslatedChanged();
    }
    //END addToUntransIndex

    emit signalEntryChanged(pos);
}

void Catalog::targetInsert(const DocPosition& pos, const QString& arg)
{
    if (KDE_ISUNLIKELY( !m_storage ))
        return;

    if ((!pos.offset)&&(isUntranslated(pos)))
    {
        //removeFromUntransIndex

        d->_untransIndex.removeAll(pos.entry);
        emit signalNumberOfUntranslatedChanged();
    }

    m_storage->targetInsert(pos,arg);

    emit signalEntryChanged(pos);
}


void Catalog::setApproved(const DocPosition& pos, bool approved)
{
    if (KDE_ISUNLIKELY( !m_storage || m_storage->isApproved(pos)==approved))
        return;

    m_storage->setApproved(pos,approved);

    //cache maintainance
    QList<int>& idx=d->_fuzzyIndex;
    if (!approved)
    {
        // insert index in the right place in the list
        QList<int>::Iterator it = idx.begin();
        while(it != idx.end() && pos.entry > short(*it))
            ++it;
        idx.insert(it,pos.entry);
    }
    else
        idx.removeAll(pos.entry);

    emit signalNumberOfFuzziesChanged();
    emit signalEntryChanged(pos);

}

//END UNDO/REDO




int Catalog::findNextInList(const QList<int>& list,int index) const
{
    if(KDE_ISUNLIKELY( list.isEmpty() ))
        return -1;

    int nextIndex=-1;
    for ( int i = 0; i < list.size(); ++i )
    {
        if (KDE_ISUNLIKELY( list.at(i) > index ))
        {
            nextIndex = list.at(i);
            break;
        }
    }

    return nextIndex;
}

int Catalog::findPrevInList(const QList<int>& list,int index) const
{
    if (KDE_ISUNLIKELY( list.isEmpty() ))
        return -1;

    int prevIndex=-1;
    for ( int i = list.size()-1; i >= 0; --i )
    {
        if (KDE_ISUNLIKELY( list.at(i) < index )) 
        {
            prevIndex = list.at(i);
            break;
        }
    }

    return prevIndex;
}







void Catalog::setBookmark(uint idx,bool set)
{
    if (set)
    {
        // insert index in the right place in the list
        QList<int>::Iterator it = d->_bookmarkIndex.begin();
        while(it != d->_bookmarkIndex.end() && (int)idx > (*it))
            ++it;
        d->_bookmarkIndex.insert(it,idx);
    }
    else
    {
        d->_bookmarkIndex.removeAll(idx);
    }
}





#include "catalog.moc"
