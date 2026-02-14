/*
  This file is part of Lokalize
  This file contains parts of KBabel code

  SPDX-FileCopyrightText: 1999-2000 Matthias Kiefer <matthias.kiefer@gmx.de>
  SPDX-FileCopyrightText: 2001-2005 Stanislav Visnovsky <visnovsky@kde.org>
  SPDX-FileCopyrightText: 2006      Nicolas Goutte <goutte@kde.org>
  SPDX-FileCopyrightText: 2007-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>
  SPDX-FileCopyrightText: 2026      Navya Sai Sadu <navyas.sadu@gmail.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/

#include "catalog.h"
#include "catalogstorage.h"
#include "dbfilesmodel.h"
#include "gettextstorage.h"
#include "jobs.h"
#include "lokalize_debug.h"
#include "mergecatalog.h"
#include "prefs_lokalize.h"
#include "project.h"
#include "projectmodel.h" //to notify about modification
#include "tsstorage.h"
#include "xliffstorage.h"

#include <KLocalizedString>

#include <QBuffer>
#include <QDir>
#include <QFileInfo>
#include <QMap>
#include <QString>
#include <QStringBuilder>

QString Catalog::supportedFileTypes(bool includeTemplates)
{
    QStringList supportedTypes;
    if (includeTemplates) {
        supportedTypes << i18n("All supported files (*.po *.pot *.xlf *.xliff *.ts)");
        supportedTypes << i18n("Gettext (*.po *.pot)");
    } else {
        supportedTypes << i18n("All supported files (*.po *.xlf *.xliff *.ts)");
        supportedTypes << i18n("Gettext (*.po)");
    }
    supportedTypes << i18n("XLIFF (*.xlf *.xliff)");
    supportedTypes << i18n("Linguist (*.ts)");

    return supportedTypes.join(QLatin1String(";;"));
}

static const QString extensions[] = {QStringLiteral(".po"), QStringLiteral(".pot"), QStringLiteral(".xlf"), QStringLiteral(".xliff"), QStringLiteral(".ts")};

QStringList Catalog::translatedStates()
{
    static QStringList const xliff_states = {i18n("New"),
                                             i18n("Needs translation"),
                                             i18n("Needs full localization"),
                                             i18n("Needs adaptation"),
                                             i18n("Translated"),
                                             i18n("Needs translation review"),
                                             i18n("Needs full localization review"),
                                             i18n("Needs adaptation review"),
                                             i18n("Final"),
                                             i18n("Signed-off")};

    return xliff_states;
}

QStringList Catalog::supportedExtensions()
{
    QStringList result;
    int i = sizeof(extensions) / sizeof(QString);
    while (--i >= 0)
        result.append(extensions[i]);
    return result;
}

bool Catalog::extIsSupported(const QString &path)
{
    QStringList ext = supportedExtensions();
    int i = ext.size();
    while (--i >= 0 && !path.endsWith(ext.at(i)))
        ;
    return i != -1;
}

Catalog::Catalog(QObject *parent)
    : QUndoStack(parent)
    , d(this)
{
    // cause refresh events for files modified from lokalize itself aint delivered automatically
    connect(this,
            qOverload<const QString &>(&Catalog::signalFileSaved),
            Project::instance()->model(),
            qOverload<const QString &>(&ProjectModel::slotFileSaved),
            Qt::QueuedConnection);

    QTimer *t = &(d._autoSaveTimer);
    t->setInterval(2 * 60 * 1000);
    t->setSingleShot(false);
    connect(t, &QTimer::timeout, this, &Catalog::doAutoSave);
    connect(this, qOverload<>(&Catalog::signalFileSaved), t, qOverload<>(&QTimer::start));
    connect(this, qOverload<>(&Catalog::signalFileLoaded), t, qOverload<>(&QTimer::start));
    connect(this, &Catalog::indexChanged, this, &Catalog::setAutoSaveDirty);
    connect(Project::local(), &Project::configChanged, this, &Catalog::projectConfigChanged);
}

Catalog::~Catalog()
{
    clear();
}

void Catalog::clear()
{
    setIndex(cleanIndex()); // to keep TM in sync
    QUndoStack::clear();
    d._errorIndex.clear();
    d._nonApprovedIndex.clear();
    d._nonApprovedNonEmptyIndex.clear();
    d._emptyIndex.clear();
    delete m_storage;
    m_storage = nullptr;
    d._filePath.clear();
    d._lastModifiedPos = DocPosition();
    d._modifiedEntries.clear();

    while (!d._altTransCatalogs.empty()) {
        d._altTransCatalogs.front()->deleteLater();
        d._altTransCatalogs.pop_front();
    }
    d._altTranslations.clear();
}

void Catalog::push(QUndoCommand *cmd)
{
    generatePhaseForCatalogIfNeeded(this);
    QUndoStack::push(cmd);
}

// BEGIN STORAGE TRANSLATION

int Catalog::capabilities() const
{
    if (Q_UNLIKELY(!m_storage))
        return 0;

    return m_storage->capabilities();
}

int Catalog::numberOfEntries() const
{
    if (Q_UNLIKELY(!m_storage))
        return 0;

    return m_storage->size();
}

static DocPosition alterForSinglePlural(const Catalog *th, DocPosition pos)
{
    // if source lang is english (implied) and target lang has only 1 plural form (e.g. Chinese)
    if (Q_UNLIKELY(th->numberOfPluralForms() == 1 && th->isPlural(pos)))
        pos.form = 1;
    return pos;
}

QString Catalog::msgid(const DocPosition &pos) const
{
    if (Q_UNLIKELY(!m_storage))
        return QString();

    return m_storage->source(alterForSinglePlural(this, pos));
}

QString Catalog::msgidWithPlurals(const DocPosition &pos, bool truncateFirstLine) const
{
    if (Q_UNLIKELY(!m_storage))
        return QString();
    return m_storage->sourceWithPlurals(pos, truncateFirstLine);
}

QString Catalog::msgstr(const DocPosition &pos) const
{
    if (Q_UNLIKELY(!m_storage))
        return QString();

    return m_storage->target(pos);
}

QString Catalog::msgstrWithPlurals(const DocPosition &pos, bool truncateFirstLine) const
{
    if (Q_UNLIKELY(!m_storage))
        return QString();

    return m_storage->targetWithPlurals(pos, truncateFirstLine);
}

CatalogString Catalog::sourceWithTags(const DocPosition &pos) const
{
    if (Q_UNLIKELY(!m_storage))
        return CatalogString();

    return m_storage->sourceWithTags(alterForSinglePlural(this, pos));
}
CatalogString Catalog::targetWithTags(const DocPosition &pos) const
{
    if (Q_UNLIKELY(!m_storage))
        return CatalogString();

    return m_storage->targetWithTags(pos);
}

CatalogString Catalog::catalogString(const DocPosition &pos) const
{
    if (Q_UNLIKELY(!m_storage))
        return CatalogString();

    return m_storage->catalogString(pos.part == DocPosition::Source ? alterForSinglePlural(this, pos) : pos);
}

QVector<Note> Catalog::notes(const DocPosition &pos) const
{
    if (Q_UNLIKELY(!m_storage))
        return QVector<Note>();

    return m_storage->notes(pos);
}

QVector<Note> Catalog::developerNotes(const DocPosition &pos) const
{
    if (Q_UNLIKELY(!m_storage))
        return QVector<Note>();

    return m_storage->developerNotes(pos);
}

Note Catalog::setNote(const DocPosition &pos, const Note &note)
{
    if (Q_UNLIKELY(!m_storage))
        return Note();

    return m_storage->setNote(pos, note);
}

QStringList Catalog::noteAuthors() const
{
    if (Q_UNLIKELY(!m_storage))
        return QStringList();

    return m_storage->noteAuthors();
}

void Catalog::attachAltTransCatalog(Catalog *altCat)
{
    d._altTransCatalogs.push_back(altCat);
    if (numberOfEntries() != altCat->numberOfEntries())
        qCWarning(LOKALIZE_LOG) << altCat->url() << "has different number of entries";
}

void Catalog::attachAltTrans(int entry, const AltTrans &trans)
{
    d._altTranslations.insert(entry, trans);
}

QVector<AltTrans> Catalog::altTrans(const DocPosition &pos) const
{
    QVector<AltTrans> result;
    if (m_storage)
        result = m_storage->altTrans(pos);

    for (const Catalog *altCat : d._altTransCatalogs) {
        if (pos.entry >= altCat->numberOfEntries()) {
            qCDebug(LOKALIZE_LOG) << "ignoring" << altCat->url() << "this time because" << pos.entry << "<" << altCat->numberOfEntries();
            continue;
        }

        if (altCat->source(pos) != source(pos)) {
            qCDebug(LOKALIZE_LOG) << "ignoring" << altCat->url() << "this time because <source>s don't match";
            continue;
        }

        QString alternateTranslation = altCat->msgstr(pos);
        if (!alternateTranslation.isEmpty() && altCat->isApproved(pos)) {
            result << AltTrans();
            result.last().target = CatalogString(alternateTranslation);
            result.last().type = AltTrans::Reference;
            result.last().origin = altCat->url();
        }
    }
    if (d._altTranslations.contains(pos.entry))
        result << d._altTranslations.value(pos.entry);
    return result;
}

QStringList Catalog::sourceFiles(const DocPosition &pos) const
{
    if (Q_UNLIKELY(!m_storage))
        return QStringList();

    return m_storage->sourceFiles(pos);
}

QString Catalog::id(const DocPosition &pos) const
{
    if (Q_UNLIKELY(!m_storage))
        return QString();

    return m_storage->id(pos);
}

QStringList Catalog::context(const DocPosition &pos) const
{
    if (Q_UNLIKELY(!m_storage))
        return QStringList();

    return m_storage->context(pos);
}

QString Catalog::setPhase(const DocPosition &pos, const QString &phase)
{
    if (Q_UNLIKELY(!m_storage))
        return QString();

    return m_storage->setPhase(pos, phase);
}

void Catalog::setActivePhase(const QString &phase, ProjectLocal::PersonRole role)
{
    d._phase = phase;
    d._phaseRole = role;
    updateApprovedEmptyIndexCache();
    Q_EMIT activePhaseChanged();
}

/**
 * @see nextFuzzyIndex() nextUntranslatedIndex() findNextInList()
 * it maintains lists which are used by above mentioned methods
 */
void Catalog::updateApprovedEmptyIndexCache()
{
    if (Q_UNLIKELY(!m_storage))
        return;

    // index cache TODO profile?
    d._nonApprovedIndex.clear();
    d._nonApprovedNonEmptyIndex.clear();
    d._emptyIndex.clear();

    DocPosition pos(0);
    const int limit = m_storage->size();
    while (pos.entry < limit) {
        if (m_storage->isEmpty(pos))
            d._emptyIndex.push_back(pos.entry);
        if (!isApproved(pos)) {
            d._nonApprovedIndex.push_back(pos.entry);
            if (!m_storage->isEmpty(pos)) {
                d._nonApprovedNonEmptyIndex.push_back(pos.entry);
            }
        }
        ++(pos.entry);
    }

    Q_EMIT signalNumberOfFuzziesChanged();
    Q_EMIT signalNumberOfEmptyChanged();
}

QString Catalog::phase(const DocPosition &pos) const
{
    if (Q_UNLIKELY(!m_storage))
        return QString();

    return m_storage->phase(pos);
}

Phase Catalog::phase(const QString &name) const
{
    return m_storage->phase(name);
}

QList<Phase> Catalog::allPhases() const
{
    return m_storage->allPhases();
}

QVector<Note> Catalog::phaseNotes(const QString &phase) const
{
    return m_storage->phaseNotes(phase);
}

QVector<Note> Catalog::setPhaseNotes(const QString &phase, const QVector<Note> notes)
{
    return m_storage->setPhaseNotes(phase, notes);
}

QMap<QString, Tool> Catalog::allTools() const
{
    return m_storage->allTools();
}

bool Catalog::isPlural(uint index) const
{
    return m_storage && m_storage->isPlural(DocPosition(index));
}

bool Catalog::isApproved(uint index) const
{
    if (Q_UNLIKELY(!m_storage))
        return false;

    bool extendedStates = m_storage->capabilities() & ExtendedStates;

    return (extendedStates && ::isApproved(state(DocPosition(index)), activePhaseRole())) || (!extendedStates && m_storage->isApproved(DocPosition(index)));
}

TargetState Catalog::state(const DocPosition &pos) const
{
    if (Q_UNLIKELY(!m_storage))
        return NeedsTranslation;

    if (m_storage->capabilities() & ExtendedStates)
        return m_storage->state(pos);
    else
        return closestState(m_storage->isApproved(pos), activePhaseRole());
}

bool Catalog::isEmpty(uint index) const
{
    return m_storage && m_storage->isEmpty(DocPosition(index));
}

bool Catalog::isEmpty(const DocPosition &pos) const
{
    return m_storage && m_storage->isEmpty(pos);
}

bool Catalog::isEquivTrans(const DocPosition &pos) const
{
    return m_storage && m_storage->isEquivTrans(pos);
}

int Catalog::binUnitsCount() const
{
    return m_storage ? m_storage->binUnitsCount() : 0;
}

int Catalog::unitById(const QString &id) const
{
    return m_storage ? m_storage->unitById(id) : 0;
}

QString Catalog::mimetype()
{
    if (Q_UNLIKELY(!m_storage))
        return QString();

    return m_storage->mimetype();
}

QString Catalog::fileType()
{
    if (Q_UNLIKELY(!m_storage))
        return QString();

    return m_storage->fileType();
}

CatalogType Catalog::type()
{
    if (Q_UNLIKELY(!m_storage))
        return Gettext;

    return m_storage->type();
}

QString Catalog::sourceLangCode() const
{
    if (Q_UNLIKELY(!m_storage))
        return QString();

    return m_storage->sourceLangCode();
}

QString Catalog::targetLangCode() const
{
    if (Q_UNLIKELY(!m_storage))
        return QString();

    return m_storage->targetLangCode();
}

void Catalog::setTargetLangCode(const QString &targetLangCode)
{
    if (Q_UNLIKELY(!m_storage))
        return;

    bool notify = m_storage->targetLangCode() != targetLangCode;
    m_storage->setTargetLangCode(targetLangCode);
    if (notify)
        Q_EMIT signalFileLoaded();
}

// END STORAGE TRANSLATION

// BEGIN OPEN/SAVE
#define DOESNTEXIST -1
#define ISNTREADABLE -2
#define UNKNOWNFORMAT -3

KAutoSaveFile *Catalog::checkAutoSave(const QString &url)
{
    KAutoSaveFile *autoSave = nullptr;
    const QList<KAutoSaveFile *> staleFiles = KAutoSaveFile::staleFiles(QUrl::fromLocalFile(url));
    for (KAutoSaveFile *stale : staleFiles) {
        if (stale->open(QIODevice::ReadOnly) && !autoSave) {
            autoSave = stale;
            autoSave->setParent(this);
        } else
            stale->deleteLater();
    }
    if (autoSave)
        qCInfo(LOKALIZE_LOG) << "autoSave" << autoSave->fileName();
    return autoSave;
}

int Catalog::loadFromUrl(const QString &filePath, const QString &saidUrl, int *fileSize, bool fast)
{
    QFileInfo info(filePath);
    if (Q_UNLIKELY(!info.exists() || info.isDir()))
        return DOESNTEXIST;
    if (Q_UNLIKELY(!info.isReadable()))
        return ISNTREADABLE;
    bool readOnly = !info.isWritable();

    QElapsedTimer a;
    a.start();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return ISNTREADABLE; // TODO

    CatalogStorage *storage = nullptr;
    if (filePath.endsWith(QLatin1String(".po")) || filePath.endsWith(QLatin1String(".pot")))
        storage = new GettextCatalog::GettextStorage;
    else if (filePath.endsWith(QLatin1String(".xlf")) || filePath.endsWith(QLatin1String(".xliff")))
        storage = new XliffStorage;
    else if (filePath.endsWith(QLatin1String(".ts")))
        storage = new TsStorage;
    else {
        // try harder
        QTextStream in(&file);
        int i = 0;
        bool gettext = false;
        while (!in.atEnd() && ++i < 64 && !gettext)
            gettext = in.readLine().contains(QLatin1String("msgid"));
        if (gettext)
            storage = new GettextCatalog::GettextStorage;
        else
            return UNKNOWNFORMAT;
    }

    int line = storage->load(&file);

    file.close();

    if (Q_UNLIKELY(line != 0 || (!storage->size() && (line == -1)))) {
        delete storage;
        return line;
    }

    if (a.elapsed() > 100)
        qCDebug(LOKALIZE_LOG) << filePath << "opened in" << a.elapsed();

    // ok...
    clear();

    // commit transaction
    m_storage = storage;

    updateApprovedEmptyIndexCache();

    d._numberOfPluralForms = storage->numberOfPluralForms();
    d._autoSaveDirty = true;
    d._readOnly = readOnly;
    d._filePath = saidUrl.isEmpty() ? filePath : saidUrl;

    // set some sane role, a real phase with a nmae will be created later with the first edit command
    setActivePhase(QString(), Project::local()->role());

    if (!fast) {
        KAutoSaveFile *autoSave = checkAutoSave(d._filePath);
        d._autoSaveRecovered = autoSave;
        if (autoSave) {
            d._autoSave->deleteLater();
            d._autoSave = autoSave;

            // restore 'modified' status for entries
            MergeCatalog *mergeCatalog = new MergeCatalog(this, this);
            int errorLine = mergeCatalog->loadFromUrl(autoSave->fileName());
            if (Q_LIKELY(errorLine == 0))
                mergeCatalog->copyToBaseCatalog();
            mergeCatalog->deleteLater();
            d._autoSave->close();
        } else
            d._autoSave->setManagedFile(QUrl::fromLocalFile(d._filePath));
    }

    if (fileSize)
        *fileSize = file.size();

    Q_EMIT signalFileLoaded();
    Q_EMIT signalFileLoaded(d._filePath);
    return 0;
}

bool Catalog::save()
{
    return saveToUrl(d._filePath);
}

// this function is not called if QUndoStack::isClean() !
bool Catalog::saveToUrl(QString localFilePath)
{
    if (Q_UNLIKELY(!m_storage))
        return true;

    bool nameChanged = !localFilePath.isEmpty();
    if (Q_LIKELY(!nameChanged))
        localFilePath = d._filePath;

    QString localPath = QFileInfo(localFilePath).absolutePath();
    if (!QFileInfo::exists(localPath))
        if (!QDir::root().mkpath(localPath))
            return false;
    QFile file(localFilePath);
    if (Q_UNLIKELY(!file.open(QIODevice::WriteOnly))) // i18n("Wasn't able to open file %1",filename.ascii());
        return false;

    bool belongsToProject = localFilePath.contains(Project::instance()->poDir());
    if (Q_UNLIKELY(!m_storage->save(&file, belongsToProject)))
        return false;

    file.close();

    d._autoSave->remove();
    d._autoSaveRecovered = false;
    setClean(); // undo/redo
    if (nameChanged) {
        d._filePath = localFilePath;
        d._autoSave->setManagedFile(QUrl::fromLocalFile(localFilePath));
    }

    Q_EMIT signalFileSaved();
    Q_EMIT signalFileSaved(localFilePath);
    return true;
}

void Catalog::doAutoSave()
{
    if (isClean() || !(d._autoSaveDirty))
        return;
    if (Q_UNLIKELY(!m_storage))
        return;
    if (!d._autoSave->open(QIODevice::WriteOnly)) {
        Q_EMIT signalFileAutoSaveFailed(d._autoSave->fileName());
        return;
    }
    qCInfo(LOKALIZE_LOG) << "doAutoSave" << d._autoSave->fileName();
    m_storage->save(d._autoSave);
    d._autoSave->close();
    d._autoSaveDirty = false;
}

void Catalog::projectConfigChanged()
{
    setActivePhase(activePhase(), Project::local()->role());
}

QByteArray Catalog::contents()
{
    QBuffer buf;
    buf.open(QIODevice::WriteOnly);
    m_storage->save(&buf);
    buf.close();
    return buf.data();
}

// END OPEN/SAVE

/**
 * helper method to keep db in a good shape :)
 * called on
 * 1) entry switch
 * 2) automatic editing code like replace or undo/redo operation
 **/
static void updateDB(const QString &filePath,
                     const QString &ctxt,
                     const CatalogString &source,
                     const CatalogString &newTarget,
                     int form,
                     bool approved,
                     const QString &dbName)
{
    TM::UpdateJob *j = new TM::UpdateJob(filePath, ctxt, source, newTarget, form, approved, dbName);
    TM::threadPool()->start(j);
}

// BEGIN UNDO/REDO
const DocPosition &Catalog::undo()
{
    QUndoStack::undo();
    return d._lastModifiedPos;
}

const DocPosition &Catalog::redo()
{
    QUndoStack::redo();
    return d._lastModifiedPos;
}

void Catalog::flushUpdateDBBuffer()
{
    if (!Settings::autoaddTM())
        return;

    DocPosition pos = d._lastModifiedPos;
    if (pos.entry == -1 || pos.entry >= numberOfEntries()) {
        // nothing to flush
        return;
    }
    QString dbName;
    if (Project::instance()->targetLangCode() == targetLangCode()) {
        dbName = Project::instance()->projectID();
    } else {
        dbName = sourceLangCode() + QLatin1Char('-') + targetLangCode();
        qCInfo(LOKALIZE_LOG) << "updating" << dbName << "because target language of project db does not match" << Project::instance()->targetLangCode()
                             << targetLangCode();
        if (!TM::DBFilesModel::instance()->m_configurations.contains(dbName)) {
            TM::OpenDBJob *openDBJob = new TM::OpenDBJob(dbName, TM::Local, true);
            connect(openDBJob, &TM::OpenDBJob::done, TM::DBFilesModel::instance(), &TM::DBFilesModel::updateProjectTmIndex);

            openDBJob->m_setParams = true;
            openDBJob->m_tmConfig.markup = Project::instance()->markup();
            openDBJob->m_tmConfig.accel = Project::instance()->accel();
            openDBJob->m_tmConfig.sourceLangCode = sourceLangCode();
            openDBJob->m_tmConfig.targetLangCode = targetLangCode();

            TM::DBFilesModel::instance()->openDB(openDBJob);
        }
    }
    int form = -1;
    if (isPlural(pos.entry))
        form = pos.form;
    updateDB(url(), context(DocPosition(pos.entry)).first(), sourceWithTags(pos), targetWithTags(pos), form, isApproved(pos.entry), dbName);

    d._lastModifiedPos = DocPosition();
}

void Catalog::setLastModifiedPos(const DocPosition &pos)
{
    if (pos.entry >= numberOfEntries()) // bin-units
        return;

    bool entryChanged = DocPos(d._lastModifiedPos) != DocPos(pos);
    if (entryChanged)
        flushUpdateDBBuffer();

    d._lastModifiedPos = pos;
}

bool CatalogPrivate::addToEmptyIndexIfAppropriate(const CatalogStorage *storage, const DocPosition &pos, bool alreadyEmpty)
{
    if ((!pos.offset) && (storage->target(pos).isEmpty()) && (!alreadyEmpty)) {
        insertInList(_emptyIndex, pos.entry);
        return true;
    }
    return false;
}

void Catalog::targetDelete(const DocPosition &pos, int count)
{
    if (Q_UNLIKELY(!m_storage))
        return;

    bool alreadyEmpty = m_storage->isEmpty(pos);
    m_storage->targetDelete(pos, count);

    if (d.addToEmptyIndexIfAppropriate(m_storage, pos, alreadyEmpty))
        Q_EMIT signalNumberOfEmptyChanged();
    Q_EMIT signalEntryModified(pos);
}

bool CatalogPrivate::removeFromUntransIndexIfAppropriate(const CatalogStorage *storage, const DocPosition &pos)
{
    if ((!pos.offset) && (storage->isEmpty(pos))) {
        _emptyIndex.remove(pos.entry);
        return true;
    }
    return false;
}

void Catalog::targetInsert(const DocPosition &pos, const QString &arg)
{
    if (Q_UNLIKELY(!m_storage))
        return;

    if (d.removeFromUntransIndexIfAppropriate(m_storage, pos))
        Q_EMIT signalNumberOfEmptyChanged();

    m_storage->targetInsert(pos, arg);
    Q_EMIT signalEntryModified(pos);
}

void Catalog::targetInsertTag(const DocPosition &pos, const InlineTag &tag)
{
    if (Q_UNLIKELY(!m_storage))
        return;

    if (d.removeFromUntransIndexIfAppropriate(m_storage, pos))
        Q_EMIT signalNumberOfEmptyChanged();

    m_storage->targetInsertTag(pos, tag);
    Q_EMIT signalEntryModified(pos);
}

InlineTag Catalog::targetDeleteTag(const DocPosition &pos)
{
    if (Q_UNLIKELY(!m_storage))
        return InlineTag();

    bool alreadyEmpty = m_storage->isEmpty(pos);
    InlineTag tag = m_storage->targetDeleteTag(pos);

    if (d.addToEmptyIndexIfAppropriate(m_storage, pos, alreadyEmpty))
        Q_EMIT signalNumberOfEmptyChanged();
    Q_EMIT signalEntryModified(pos);
    return tag;
}

void Catalog::setTarget(DocPosition pos, const CatalogString &s)
{
    // TODO for case of markup present
    m_storage->setTarget(pos, s.string);
}

TargetState Catalog::setState(const DocPosition &pos, TargetState state)
{
    bool extendedStates = m_storage && m_storage->capabilities() & ExtendedStates;
    bool approved = ::isApproved(state, activePhaseRole());
    if (Q_UNLIKELY(!m_storage || (extendedStates && m_storage->state(pos) == state) || (!extendedStates && m_storage->isApproved(pos) == approved)))
        return this->state(pos);

    TargetState prevState;
    if (extendedStates) {
        prevState = m_storage->setState(pos, state);
        d._statesIndex[prevState].remove(pos.entry);
        insertInList(d._statesIndex[state], pos.entry);
    } else {
        prevState = closestState(!approved, activePhaseRole());
        m_storage->setApproved(pos, approved);
    }

    if (!approved) {
        insertInList(d._nonApprovedIndex, pos.entry);
        if (!m_storage->isEmpty(pos))
            insertInList(d._nonApprovedNonEmptyIndex, pos.entry);
    } else {
        d._nonApprovedIndex.remove(pos.entry);
        d._nonApprovedNonEmptyIndex.remove(pos.entry);
    }

    Q_EMIT signalNumberOfFuzziesChanged();
    Q_EMIT signalEntryModified(pos);

    return prevState;
}

Phase Catalog::updatePhase(const Phase &phase)
{
    return m_storage->updatePhase(phase);
}

void Catalog::setEquivTrans(const DocPosition &pos, bool equivTrans)
{
    if (m_storage)
        m_storage->setEquivTrans(pos, equivTrans);
}

bool Catalog::setModified(DocPos entry, bool modified)
{
    if (modified) {
        if (d._modifiedEntries.contains(entry))
            return false;
        d._modifiedEntries.insert(entry);
    } else
        d._modifiedEntries.remove(entry);
    return true;
}

bool Catalog::isModified(DocPos entry) const
{
    return d._modifiedEntries.contains(entry);
}

bool Catalog::isModified(int entry) const
{
    if (!isPlural(entry))
        return isModified(DocPos(entry, 0));

    int f = numberOfPluralForms();
    while (--f >= 0)
        if (isModified(DocPos(entry, f)))
            return true;
    return false;
}

// END UNDO/REDO

// from the passed list, gets next entry relative to passed index
int findNextInList(const std::list<int> &list, int index)
{
    int nextIndex = -1;
    for (int key : list) {
        if (Q_UNLIKELY(key > index)) {
            nextIndex = key;
            break;
        }
    }
    return nextIndex;
}

int findPrevInList(const std::list<int> &list, int index)
{
    int prevIndex = -1;
    for (int key : list) {
        if (Q_UNLIKELY(key >= index))
            break;
        prevIndex = key;
    }
    return prevIndex;
}

void insertInList(std::list<int> &list, int index)
{
    std::list<int>::const_iterator it = list.cbegin();
    while (it != list.cend() && index > *it)
        ++it;
    list.insert(it, index);
}

void Catalog::setBookmark(uint idx, bool set)
{
    if (set)
        insertInList(d._bookmarkIndex, idx);
    else
        d._bookmarkIndex.remove(idx);
}

bool isApproved(TargetState state, ProjectLocal::PersonRole role)
{
    static const TargetState marginStates[] = {Translated, Final, SignedOff};
    return state >= marginStates[role];
}

bool isApproved(TargetState state)
{
    static const TargetState marginStates[] = {Translated, Final, SignedOff};
    return state == marginStates[0] || state == marginStates[1] || state == marginStates[2];
}

TargetState closestState(bool approved, ProjectLocal::PersonRole role)
{
    Q_ASSERT(role != ProjectLocal::Undefined);
    static const TargetState approvementStates[][3] = {{NeedsTranslation, NeedsReviewTranslation, NeedsReviewTranslation}, {Translated, Final, SignedOff}};
    return approvementStates[approved][role];
}

bool Catalog::isObsolete(int entry) const
{
    if (Q_UNLIKELY(!m_storage))
        return false;

    return m_storage->isObsolete(entry);
}

QString Catalog::originalOdfFilePath()
{
    if (Q_UNLIKELY(!m_storage))
        return QString();

    return m_storage->originalOdfFilePath();
}

void Catalog::setOriginalOdfFilePath(const QString &odfFilePath)
{
    if (Q_UNLIKELY(!m_storage))
        return;

    m_storage->setOriginalOdfFilePath(odfFilePath);
}

#include "moc_catalog.cpp"
