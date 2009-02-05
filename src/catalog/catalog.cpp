/* ****************************************************************************
  This file is part of Lokalize
  This file contains parts of KBabel code

  Copyright (C) 1999-2000   by Matthias Kiefer <matthias.kiefer@gmx.de>
                2001-2005   by Stanislav Visnovsky <visnovsky@kde.org>
                2006        by Nicolas Goutte <goutte@kde.org>
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
// #define KDE_NO_DEBUG_OUTPUT

#include "catalog.h"
#include "catalog_private.h"
#include "project.h"
#include "projectmodel.h" //to notify about modification 

#include "catalogstorage.h"
#include "gettextstorage.h"
#include "gettextimport.h"
#include "gettextexport.h"

#include "xliffstorage.h"


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


static const char* extensions[]={".po",".pot",".xlf"};

QStringList Catalog::supportedExtensions()
{
    QStringList result;
    int i=sizeof(extensions)/sizeof(char*);
    while (--i>=0)
        result.append(QString(extensions[i]));
    return result;
}

bool Catalog::extIsSupported(const QString& path)
{
    QStringList ext=supportedExtensions();
    int i=ext.size();
    while (--i>=0)
        if (path.endsWith(ext.at(i)))
            break;
    return i!=-1;
}

Catalog::Catalog(QObject *parent)
    : QUndoStack(parent)
    , d(new CatalogPrivate(this))
    , m_storage(0)
{
    //cause refresh events for files modified from lokalize itself aint delivered automatically
    connect(this,SIGNAL(signalFileSaved(KUrl)),
            Project::instance()->model()->dirLister(),SLOT(slotFileSaved(KUrl)));


    QTimer* t=&(d->_autoSaveTimer);
    t->setInterval(5*60*1000);
    t->setSingleShot(false);
    connect(t,   SIGNAL(timeout()),        this,SLOT(doAutoSave()));
    connect(this,SIGNAL(signalFileSaved()),   t,SLOT(start()));
    connect(this,SIGNAL(signalFileLoaded()),  t,SLOT(start()));
    connect(this,SIGNAL(indexChanged(int)),this,SLOT(setAutoSaveDirty()));
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
    d->_modifiedEntries.clear();
/*
    d->msgidDiffList.clear();
    d->msgstr2MsgidDiffList.clear();
    d->diffCache.clear();
    */
}


//BEGIN STORAGE TRANSLATION

int Catalog::capabilities() const
{
    if (KDE_ISUNLIKELY( !m_storage )) return 0;

    return m_storage->capabilities();
}


int Catalog::numberOfEntries() const
{
    if (KDE_ISUNLIKELY( !m_storage )) return 0;

    return m_storage->size();
}

QString Catalog::msgid(const DocPosition& pos) const
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

QString Catalog::msgstr(const DocPosition& pos) const
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

CatalogString Catalog::catalogString(const DocPosition& pos) const
{
    if (KDE_ISUNLIKELY( !m_storage || m_storage->isEmpty() ))
        return CatalogString();

    return m_storage->catalogString(pos);
}


QList<Note> Catalog::notes(const DocPosition& pos) const
{
    if (KDE_ISUNLIKELY( !m_storage || m_storage->isEmpty() ))
        return QList<Note>();

    return m_storage->notes(pos);
}

Note Catalog::setNote(const DocPosition& pos, const Note& note)
{
    return m_storage->setNote(pos,note);
}

QStringList Catalog::noteAuthors() const
{
    return m_storage->noteAuthors();
}

QString Catalog::alttrans(const DocPosition& pos) const
{
    if (KDE_ISUNLIKELY( !m_storage || m_storage->isEmpty() ))
        return d->CatalogPrivate::_emptyStr;

    return m_storage->alttrans(pos);
}

QStringList Catalog::sourceFiles(const DocPosition& pos) const
{
    if (KDE_ISUNLIKELY( !m_storage || m_storage->isEmpty() ))
        return QStringList();

    return m_storage->sourceFiles(pos);
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


bool Catalog::isPlural(uint index) const
{
    if (KDE_ISUNLIKELY( !m_storage || m_storage->isEmpty() ))
        return false;

    return m_storage->isPlural(DocPosition(index));
}

bool Catalog::isApproved(uint index) const
{
    if (KDE_ISUNLIKELY( !m_storage || m_storage->isEmpty() ))
        return false;

    return m_storage->isApproved(DocPosition(index));
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
#define DOESNTEXIST -1
#define ISNTREADABLE -2
#define UNKNOWNFORMAT -3

int Catalog::loadFromUrl(const KUrl& url)
{
    bool readOnly=false;
    if (url.isLocalFile())
    {
        QFileInfo info(url.path());
        if(KDE_ISUNLIKELY( !info.exists() || info.isDir()) )
            return DOESNTEXIST;
        if(KDE_ISUNLIKELY( !info.isReadable() ))
            return ISNTREADABLE;
        readOnly=!info.isWritable();
    }


    //BEGIN autosave files check
    KAutoSaveFile* autoSave=0;
    QList<KAutoSaveFile*> staleFiles = KAutoSaveFile::staleFiles(url);
    if (!staleFiles.isEmpty())
    {
        qWarning()<<"888";
        foreach (KAutoSaveFile *stale, staleFiles)
        {
            if (stale->managedFile()!=url)
                qWarning()<<"MOTHER FUCKER!!";
            if (stale->open(QIODevice::ReadOnly) && !autoSave)
            {
                autoSave=stale;
                autoSave->setParent(this);
            }
            else
                stale->deleteLater();
        }
    }
    qWarning()<<"autoSave"<<autoSave;
    if (autoSave)
        qWarning()<<"autoSave"<<autoSave->fileName();
    //END autosave files check



    CatalogStorage* storage=0;
    if (url.fileName().endsWith(".po")||url.fileName().endsWith(".pot"))
        storage=new GettextCatalog::GettextStorage;
    else if (url.fileName().endsWith(".xlf")||url.fileName().endsWith(".xliff"))
        storage=new XliffStorage;
    else
        return UNKNOWNFORMAT;

    QTime a;a.start();

    QString target;
    QFile* file=autoSave;
    if (!autoSave)
    {
        if(KDE_ISUNLIKELY( !KIO::NetAccess::download(url,target,NULL) ))
            return ISNTREADABLE;
        file=new QFile(target);
        file->deleteLater();//kung-fu
        if (!file->open(QIODevice::ReadOnly))
            return ISNTREADABLE;//TODO
    }

    int line=storage->load(file);

    file->close();
    if (!autoSave)
        KIO::NetAccess::removeTempFile(target);

    kWarning() <<"file opened in"<<a.elapsed();

    if (KDE_ISUNLIKELY(line!=0))
    {
        delete storage;
        return line;
    }

    //ok...
    clear();

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

    d->_numberOfPluralForms = storage->numberOfPluralForms();
    d->_autoSaveRecovered=autoSave;
    d->_autoSaveDirty=true;
    d->_readOnly=readOnly;
    d->_url=url;
    if (autoSave)
    {
        d->_autoSave->deleteLater();
        d->_autoSave=autoSave;
    }
    else
        d->_autoSave->setManagedFile(url);

    emit signalFileLoaded();
    emit signalFileLoaded(url);
    return 0;
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


    bool remote=url.isLocalFile();
    QFile* file;
    if (KDE_ISUNLIKELY( !remote ))
    {
        file=new KTemporaryFile();
    }
    else
    {
        if (!QFile::exists(url.directory()))
            if (!QDir::root().mkpath(url.directory()))
                return false;
        file=new QFile(url.path());
    }
    file->deleteLater(); //kung-fu ;)
    if (KDE_ISUNLIKELY( !file->open(QIODevice::WriteOnly) )) //i18n("Wasn't able to open file %1",filename.ascii());
        return false;
  
    if (KDE_ISUNLIKELY( !m_storage->save(file) ))
        return false;

    QString localFile=file->fileName();
    file->close();
    if (KDE_ISUNLIKELY(remote && !KIO::NetAccess::upload(localFile, url, NULL) ))
        return false;

    d->_autoSave->remove();
    setClean(); //undo/redo
    if (nameChanged)
    {
        d->_url=url;
        d->_autoSave->setManagedFile(url);
    }

    //Settings::self()->setCurrentGroup("Bookmarks");
    //Settings::self()->addItemIntList(d->_url.url(),d->_bookmarkIndex);

    emit signalFileSaved();
    emit signalFileSaved(url);
    return true;
/*
    else if (status==NO_PERMISSIONS)
    {
        if (KMessageBox::warningContinueCancel(this,
	     i18n("You do not have permission to write to file:\n%1\n"
		  "Do you want to save to another file or cancel?", _currentURL.prettyUrl()),
	     i18n("Error"),KStandardGuiItem::save())==KMessageBox::Continue)
            return fileSaveAs();

    }
*/
}


void Catalog::doAutoSave()
{
    if (isClean()||!(d->_autoSaveDirty))
        return;
    if (KDE_ISUNLIKELY( !m_storage ))
        return;
    if (!d->_autoSave->open(QIODevice::WriteOnly))
        return;
    qWarning()<<"doAutoSave"<<d->_autoSave->fileName();
    m_storage->save(d->_autoSave);
    d->_autoSave->close();
    d->_autoSaveDirty=false;
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


void Catalog::push(QUndoCommand *cmd/*, bool rebaseForDBUpdate*/)
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
        //kWarning()<<"nothing to flush or new file opened";
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

bool CatalogPrivate::addToUntransIndexIfAppropriate(CatalogStorage* storage, const DocPosition& pos)
{
    if ((!pos.offset)&&(storage->target(pos).isEmpty())&&(!storage->isUntranslated(pos)))
    {
        // insert index in the right place in the list
        QList<int>::Iterator it = _untransIndex.begin();
        while(it != _untransIndex.end() && pos.entry > (int)*it)
            ++it;
        _untransIndex.insert(it,pos.entry);
        return true;
    }
    return false;
}

void Catalog::targetDelete(const DocPosition& pos, int count)
{
    if (KDE_ISUNLIKELY( !m_storage ))
        return;

    m_storage->targetDelete(pos,count);
    
    if (d->addToUntransIndexIfAppropriate(m_storage,pos))
        emit signalNumberOfUntranslatedChanged();
    emit signalEntryModified(pos);
}


bool CatalogPrivate::removeFromUntransIndexIfAppropriate(CatalogStorage* storage, const DocPosition& pos)
{
    if ((!pos.offset)&&(storage->isUntranslated(pos)))
    {
        _untransIndex.removeAll(pos.entry);
        return true;
    }
    return false;
}

void Catalog::targetInsert(const DocPosition& pos, const QString& arg)
{
    if (KDE_ISUNLIKELY( !m_storage ))
        return;

    if (d->removeFromUntransIndexIfAppropriate(m_storage,pos))
        emit signalNumberOfUntranslatedChanged();

    m_storage->targetInsert(pos,arg);

    emit signalEntryModified(pos);
}

void Catalog::targetInsertTag(const DocPosition& pos, const TagRange& tag)
{
    if (KDE_ISUNLIKELY( !m_storage ))
        return;

    if (d->removeFromUntransIndexIfAppropriate(m_storage,pos))
        emit signalNumberOfUntranslatedChanged();

    m_storage->targetInsertTag(pos,tag);

    emit signalEntryModified(pos);
}

TagRange Catalog::targetDeleteTag(const DocPosition& pos)
{
    if (KDE_ISUNLIKELY( !m_storage ))
        return TagRange();

    TagRange tag=m_storage->targetDeleteTag(pos);
    
    if (d->addToUntransIndexIfAppropriate(m_storage,pos))
        emit signalNumberOfUntranslatedChanged();
    emit signalEntryModified(pos);
    return tag;
}

void Catalog::setApproved(const DocPosition& pos, bool approved)
{
    if (KDE_ISUNLIKELY( !m_storage || m_storage->isApproved(pos)==approved))
        return;

    m_storage->setApproved(pos,approved);

    // cache maintenance
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
    emit signalEntryModified(pos);

}

bool Catalog::setModified(int entry,bool modif)
{
    if (modif)
    {
        if (d->_modifiedEntries.contains(entry))
            return false;

        d->_modifiedEntries.append(entry);
    }
    else
        d->_modifiedEntries.remove(entry);
    return true;
}

bool Catalog::isModified(int entry)
{
    return d->_modifiedEntries.contains(entry);
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
