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

#include "mergecatalog.h"

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
#include <kdatetime.h>

#include <kio/netaccess.h>
#include <ktemporaryfile.h>


static const char* const extensions[]={".po",".pot",".xlf"};

static const char* const xliff_states[]={
        I18N_NOOP("New"),I18N_NOOP("Needs translation"),I18N_NOOP("Needs full localization"),I18N_NOOP("Needs adaptation"),I18N_NOOP("Translated"),
        I18N_NOOP("Needs translation review"),I18N_NOOP("Needs full localization review"),I18N_NOOP("Needs adaptation review"),I18N_NOOP("Final"),
        I18N_NOOP("Signed-off")};

const char* const* Catalog::states()
{
    return xliff_states;
}

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
    while (--i>=0 && !path.endsWith(ext.at(i)))
        ;
    return i!=-1;
}

Catalog::Catalog(QObject *parent)
    : QUndoStack(parent)
    , d(new CatalogPrivate(this))
    , m_storage(0)
{
    //cause refresh events for files modified from lokalize itself aint delivered automatically
    connect(this,SIGNAL(signalFileSaved(KUrl)),
            Project::instance()->model(),SLOT(slotFileSaved(KUrl)),Qt::QueuedConnection);


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
    d->_nonApprovedIndex.clear();
    d->_emptyIndex.clear();
    delete m_storage;m_storage=0;
    d->_url.clear();
    d->_lastModifiedPos=DocPosition();
    d->_modifiedEntries.clear();

    while (!d->_altTransCatalogs.isEmpty())
        d->_altTransCatalogs.takeFirst()->deleteLater();
/*
    d->msgidDiffList.clear();
    d->msgstr2MsgidDiffList.clear();
    d->diffCache.clear();
    */
}



void Catalog::push(QUndoCommand* cmd)
{
    generatePhaseForCatalogIfNeeded(this);
    QUndoStack::push(cmd);
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


static DocPosition alterForSinglePlural(const Catalog* th, DocPosition pos)
{
    //if source lang is english (implied) and target lang has only 1 plural form (e.g. Chinese)
    if (KDE_ISUNLIKELY(th->numberOfPluralForms()==1 && th->isPlural(pos)))
        pos.form=1;
    return pos;
}

QString Catalog::msgid(const DocPosition& pos) const
{
    if (KDE_ISUNLIKELY( !m_storage ))
        return d->CatalogPrivate::_emptyStr;

    return m_storage->source(alterForSinglePlural(this, pos));
}

QString Catalog::msgstr(const DocPosition& pos) const
{
    if (KDE_ISUNLIKELY( !m_storage ))
        return d->CatalogPrivate::_emptyStr;

   return m_storage->target(pos);
}

CatalogString Catalog::sourceWithTags(const DocPosition& pos) const
{
    if (KDE_ISUNLIKELY( !m_storage ))
        return CatalogString();

    return m_storage->sourceWithTags(alterForSinglePlural(this, pos));

}
CatalogString Catalog::targetWithTags(const DocPosition& pos) const
{
    if (KDE_ISUNLIKELY( !m_storage ))
        return CatalogString();

    return m_storage->targetWithTags(pos);
}

CatalogString Catalog::catalogString(const DocPosition& pos) const
{
    if (KDE_ISUNLIKELY( !m_storage ))
        return CatalogString();

    return m_storage->catalogString(pos.part==DocPosition::Source?alterForSinglePlural(this, pos):pos);
}


QVector<Note> Catalog::notes(const DocPosition& pos) const
{
    if (KDE_ISUNLIKELY( !m_storage ))
        return QVector<Note>();

    return m_storage->notes(pos);
}

QVector<Note> Catalog::developerNotes(const DocPosition& pos) const
{
    if (KDE_ISUNLIKELY( !m_storage ))
        return QVector<Note>();

    return m_storage->developerNotes(pos);
}

Note Catalog::setNote(const DocPosition& pos, const Note& note)
{
    if (KDE_ISUNLIKELY( !m_storage ))
        return Note();

    return m_storage->setNote(pos,note);
}

QStringList Catalog::noteAuthors() const
{
    if (KDE_ISUNLIKELY( !m_storage ))
        return QStringList();

    return m_storage->noteAuthors();
}

void Catalog::attachAltTransCatalog(Catalog* cat)
{
    d->_altTransCatalogs.append(cat);
}

QVector<AltTrans> Catalog::altTrans(const DocPosition& pos) const
{
    QVector<AltTrans> result;
    if (m_storage)
        result=m_storage->altTrans(pos);

    foreach(Catalog* altCat, d->_altTransCatalogs)
    {
        QString target=altCat->msgstr(pos);
        if (!target.isEmpty())
        {
            result<<AltTrans();
            result.last().target=target;
        }
    }
    return result;
}

QStringList Catalog::sourceFiles(const DocPosition& pos) const
{
    if (KDE_ISUNLIKELY( !m_storage ))
        return QStringList();

    return m_storage->sourceFiles(pos);
}

QString Catalog::id(const DocPosition& pos) const
{
    if (KDE_ISUNLIKELY( !m_storage ))
        return d->CatalogPrivate::_emptyStr;

    return m_storage->id(pos);
}

QString Catalog::msgctxt(uint index) const
{
    if (KDE_ISUNLIKELY( !m_storage ))
        return d->CatalogPrivate::_emptyStr;

    DocPosition pos(index);
    return m_storage->contextCount(pos)?
                m_storage->context(pos):
                d->CatalogPrivate::_emptyStr;
}

QString Catalog::setPhase(const DocPosition& pos, const QString& phase)
{
    if (KDE_ISUNLIKELY( !m_storage ))
        return QString();

    return m_storage->setPhase(pos,phase);
}


void Catalog::setActivePhase(const QString& phase, ProjectLocal::PersonRole role)
{
    //TODO approved index cache change.
    d->_phase=phase;
    d->_phaseRole=role;
    emit activePhaseChanged();
}

QString Catalog::phase(const DocPosition& pos) const
{
    if (KDE_ISUNLIKELY( !m_storage ))
        return QString();

    return m_storage->phase(pos);
}

Phase Catalog::phase(const QString& name) const
{
    return m_storage->phase(name);
}

QList<Phase> Catalog::allPhases() const
{
    return m_storage->allPhases();
}

QVector<Note> Catalog::phaseNotes(const QString& phase) const
{
    return m_storage->phaseNotes(phase);
}

QVector<Note> Catalog::setPhaseNotes(const QString& phase, QVector<Note> notes)
{
    return m_storage->setPhaseNotes(phase, notes);
}

QMap<QString,Tool> Catalog::allTools() const
{
    return m_storage->allTools();
}

bool Catalog::isPlural(uint index) const
{
    if (KDE_ISUNLIKELY( !m_storage ))
        return false;

    return m_storage->isPlural(DocPosition(index));
}

bool Catalog::isApproved(uint index) const
{
    if (KDE_ISUNLIKELY( !m_storage ))
        return false;

    bool extendedStates=m_storage->capabilities()&ExtendedStates;

    return (extendedStates&&::isApproved(state(DocPosition(index)),activePhaseRole()))
    ||(!extendedStates&&m_storage->isApproved(DocPosition(index)));
}

TargetState Catalog::state(const DocPosition& pos) const
{
    if (KDE_ISUNLIKELY( !m_storage ))
        return NeedsTranslation;

    if (m_storage->capabilities()&ExtendedStates)
        return m_storage->state(pos);
    else
        return closestState(m_storage->isApproved(pos), activePhaseRole());
}

bool Catalog::isEmpty(uint index) const
{
    if (KDE_ISUNLIKELY( !m_storage ))
        return false;

    return m_storage->isEmpty(DocPosition(index));
}

bool Catalog::isEmpty(const DocPosition& pos) const
{
    if (KDE_ISUNLIKELY( !m_storage ))
        return false;

    return m_storage->isEmpty(pos);
}


bool Catalog::isEquivTrans(const DocPosition& pos) const
{
    return m_storage && m_storage->isEquivTrans(pos);
}

int Catalog::binUnitsCount() const
{
    return m_storage?m_storage->binUnitsCount():0;
}

int Catalog::unitById(const QString& id) const
{
    return m_storage?m_storage->unitById(id):0;
}

QString Catalog::mimetype()
{
    if (KDE_ISUNLIKELY( !m_storage ))
        return QString();

    return m_storage->mimetype();
}
//END STORAGE TRANSLATION

//BEGIN OPEN/SAVE
#define DOESNTEXIST -1
#define ISNTREADABLE -2
#define UNKNOWNFORMAT -3

KAutoSaveFile* Catalog::checkAutoSave(const KUrl& url)
{
    KAutoSaveFile* autoSave=0;
    QList<KAutoSaveFile*> staleFiles = KAutoSaveFile::staleFiles(url);
    if (!staleFiles.isEmpty())
    {
        foreach (KAutoSaveFile *stale, staleFiles)
        {
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
    return autoSave;
}

int Catalog::loadFromUrl(const KUrl& url, const KUrl& saidUrl, int* fileSize)
{
    bool readOnly=false;
    if (url.isLocalFile())
    {
        QFileInfo info(url.toLocalFile());
        if(KDE_ISUNLIKELY( !info.exists() || info.isDir()) )
            return DOESNTEXIST;
        if(KDE_ISUNLIKELY( !info.isReadable() ))
            return ISNTREADABLE;
        readOnly=!info.isWritable();
    }


    QTime a;a.start();

    QString target;
    if(KDE_ISUNLIKELY( !KIO::NetAccess::download(url,target,NULL) ))
        return ISNTREADABLE;
    QFile* file=new QFile(target);
    file->deleteLater();//kung-fu
    if (!file->open(QIODevice::ReadOnly))
        return ISNTREADABLE;//TODO

    CatalogStorage* storage=0;
    if (url.fileName().endsWith(".po")||url.fileName().endsWith(".pot"))
        storage=new GettextCatalog::GettextStorage;
    else if (url.fileName().endsWith(".xlf")||url.fileName().endsWith(".xliff"))
        storage=new XliffStorage;
    else
    {
        //try harder
        QTextStream in(file);
        int i=0;
        bool gettext=false;
        while (!in.atEnd()&& i<64 && !gettext)
            gettext=in.readLine().contains("msgid");
        if (gettext) storage=new GettextCatalog::GettextStorage;
        else return UNKNOWNFORMAT;
    }

    int line=storage->load(file);

    file->close();
    KIO::NetAccess::removeTempFile(target);

    if (KDE_ISUNLIKELY(line!=0 || (!storage->size() && (line=-1) ) ))
    {
        delete storage;
        return line;
    }

    kWarning() <<"file opened in"<<a.elapsed();

    //ok...
    clear();

    //index cache TODO profile?
    d->_nonApprovedIndex.clear();
    d->_emptyIndex.clear();

    DocPosition pos(0);
    int limit=storage->size();
    while (pos.entry<limit)
    {
        if (!storage->isApproved(pos))
            d->_nonApprovedIndex << pos.entry;
        if (storage->isEmpty(pos))
            d->_emptyIndex << pos.entry;

        ++(pos.entry);
    }

    //commit transaction
    m_storage=storage;

    d->_numberOfPluralForms = storage->numberOfPluralForms();
    d->_autoSaveDirty=true;
    d->_readOnly=readOnly;
    d->_url=saidUrl.isEmpty()?url:saidUrl;

    KAutoSaveFile* autoSave=checkAutoSave(d->_url);
    d->_autoSaveRecovered=autoSave;
    if (autoSave)
    {
        d->_autoSave->deleteLater();
        d->_autoSave=autoSave;

        //restore 'modified' status for entries
        MergeCatalog* mergeCatalog=new MergeCatalog(this,this);
        int errorLine=mergeCatalog->loadFromUrl(KUrl::fromPath(autoSave->fileName()));
        if (KDE_ISLIKELY(errorLine==0))
            mergeCatalog->copyToBaseCatalog();
        mergeCatalog->deleteLater();
    }
    else
        d->_autoSave->setManagedFile(d->_url);

    if (fileSize)
        *fileSize=file->size();

    emit signalFileLoaded();
    emit signalFileLoaded(d->_url);
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

    if (d->_modifiedEntries.isEmpty())
        return true; // nothing changed to save

    bool nameChanged=false;
    if (KDE_ISLIKELY( url.isEmpty() ))
        url = d->_url;
    else
        nameChanged=true;


    bool remote=url.isLocalFile();
    QFile* file;
    if (KDE_ISUNLIKELY( !remote ))
        file=new KTemporaryFile();
    else
    {
        if (!QFile::exists(url.directory()))
            if (!QDir::root().mkpath(url.directory()))
                return false;
        file=new QFile(url.toLocalFile());
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
    d->_autoSaveRecovered=false;

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


QByteArray Catalog::contents()
{
    QBuffer buf;
    buf.open(QIODevice::WriteOnly);
    m_storage->save(&buf);
    buf.close();
    return buf.data();
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
              const CatalogString& english,
              const CatalogString& newTarget,
              int form,
              bool approved
              //const DocPosition&,//for back tracking
             )
{
    TM::UpdateJob* j=new TM::UpdateJob(filePath,ctxt,english,newTarget,form,approved,
                               Project::instance()->projectID());
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
             sourceWithTags(pos),
             targetWithTags(pos),
             form,
             isApproved(pos.entry));

    d->_lastModifiedPos=DocPosition();
}

void Catalog::setLastModifiedPos(const DocPosition& pos)
{
    if (pos.entry>=numberOfEntries()) //bin-units
        return;

    bool entryChanged=DocPos(d->_lastModifiedPos)!=DocPos(pos);
    if (entryChanged)
        flushUpdateDBBuffer();

    d->_lastModifiedPos=pos;
}

bool CatalogPrivate::addToEmptyIndexIfAppropriate(CatalogStorage* storage, const DocPosition& pos, bool alreadyEmpty)
{
    if ((!pos.offset)&&(storage->target(pos).isEmpty())&&(!alreadyEmpty))
    {
        insertInList(_emptyIndex,pos.entry);
        return true;
    }
    return false;
}

void Catalog::targetDelete(const DocPosition& pos, int count)
{
    if (KDE_ISUNLIKELY( !m_storage ))
        return;

    bool alreadyEmpty = m_storage->isEmpty(pos);
    m_storage->targetDelete(pos,count);

    if (d->addToEmptyIndexIfAppropriate(m_storage,pos,alreadyEmpty))
        emit signalNumberOfEmptyChanged();
    emit signalEntryModified(pos);
}

bool CatalogPrivate::removeFromUntransIndexIfAppropriate(CatalogStorage* storage, const DocPosition& pos)
{
    if ((!pos.offset)&&(storage->isEmpty(pos)))
    {
        _emptyIndex.removeAll(pos.entry);
        return true;
    }
    return false;
}

void Catalog::targetInsert(const DocPosition& pos, const QString& arg)
{
    if (KDE_ISUNLIKELY( !m_storage ))
        return;

    if (d->removeFromUntransIndexIfAppropriate(m_storage,pos))
        emit signalNumberOfEmptyChanged();

    m_storage->targetInsert(pos,arg);
    emit signalEntryModified(pos);
}

void Catalog::targetInsertTag(const DocPosition& pos, const InlineTag& tag)
{
    if (KDE_ISUNLIKELY( !m_storage ))
        return;

    if (d->removeFromUntransIndexIfAppropriate(m_storage,pos))
        emit signalNumberOfEmptyChanged();

    m_storage->targetInsertTag(pos,tag);
    emit signalEntryModified(pos);
}

InlineTag Catalog::targetDeleteTag(const DocPosition& pos)
{
    if (KDE_ISUNLIKELY( !m_storage ))
        return InlineTag();

    bool alreadyEmpty = m_storage->isEmpty(pos);
    InlineTag tag=m_storage->targetDeleteTag(pos);

    if (d->addToEmptyIndexIfAppropriate(m_storage,pos,alreadyEmpty))
        emit signalNumberOfEmptyChanged();
    emit signalEntryModified(pos);
    return tag;
}

TargetState Catalog::setState(const DocPosition& pos, TargetState state)
{
    bool extendedStates=m_storage->capabilities()&ExtendedStates;
    bool approved=::isApproved(state,activePhaseRole());
    if (KDE_ISUNLIKELY( !m_storage
        || (extendedStates && m_storage->state(pos)==state)
        || (!extendedStates && m_storage->isApproved(pos)==approved)))
        return this->state(pos);

    TargetState prevState;
    if (extendedStates)
    {
        prevState=m_storage->setState(pos,state);
        d->_statesIndex[prevState].removeAll(pos.entry);
        insertInList(d->_statesIndex[state],pos.entry);
    }
    else
    {
        prevState=closestState(!approved,activePhaseRole());
        m_storage->setApproved(pos,approved);
    }

    if (!approved)
        insertInList(d->_nonApprovedIndex,pos.entry);
    else
        d->_nonApprovedIndex.removeAll(pos.entry);

    emit signalNumberOfFuzziesChanged();
    emit signalEntryModified(pos);

    return prevState;
}

Phase Catalog::updatePhase(const Phase& phase)
{
    return m_storage->updatePhase(phase);
}

void Catalog::setEquivTrans(const DocPosition& pos, bool equivTrans)
{
    if (m_storage) m_storage->setEquivTrans(pos, equivTrans);
}

bool Catalog::setModified(DocPos entry, bool modified)
{
    if (modified)
    {
        if (d->_modifiedEntries.contains(entry))
            return false;
        d->_modifiedEntries.insert(entry);
    }
    else
        d->_modifiedEntries.remove(entry);
    return true;
}

bool Catalog::isModified(DocPos entry) const
{
    return d->_modifiedEntries.contains(entry);
}

bool Catalog::isModified(int entry) const
{
    if (!isPlural(entry))
        return isModified(DocPos(entry,0));

    int f=numberOfPluralForms();
    while(--f>=0)
        if (isModified(DocPos(entry,f)))
            return true;
    return false;
}

//END UNDO/REDO




int findNextInList(const QLinkedList<int>& list, int index)
{
    int nextIndex=-1;
    foreach(int key, list)
    {
        if (KDE_ISUNLIKELY( key>index ))
        {
            nextIndex = key;
            break;
        }
    }
    return nextIndex;
}

int findPrevInList(const QLinkedList<int>& list, int index)
{
    int prevIndex=-1;
    foreach(int key, list)
    {
        if (KDE_ISUNLIKELY( key>=index ))
            break;
        prevIndex = key;
    }
    return prevIndex;
}

void insertInList(QLinkedList<int>& list, int index)
{
    QLinkedList<int>::Iterator it=list.begin();
    while(it != list.end() && index > *it)
        ++it;
    list.insert(it,index);
}

void Catalog::setBookmark(uint idx, bool set)
{
    if (set)
        insertInList(d->_bookmarkIndex,idx);
    else
        d->_bookmarkIndex.removeAll(idx);
}


bool isApproved(TargetState state, ProjectLocal::PersonRole role)
{
    static const TargetState marginStates[]={Translated, Final, SignedOff};
    return state>=marginStates[role];
}

TargetState closestState(bool approved, ProjectLocal::PersonRole role)
{
    static const TargetState approvementStates[][3]={
        {NeedsTranslation, NeedsReviewTranslation, NeedsReviewTranslation},
        {Translated, Final, SignedOff}
    };
    return approvementStates[approved][role];
}


#include "catalog.moc"
