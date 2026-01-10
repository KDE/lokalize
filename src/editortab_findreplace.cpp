/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2009 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "catalog.h"
#include "cmd.h"
#include "editortab.h"
#include "editorview.h"
#include "lokalize_debug.h"
#include "pos.h"
#include "project.h"
#include "stemming.h"
#include "ui_findextension.h"

#include <KLocalizedString>
#include <KMessageBox>
#include <KReplace>
#include <KReplaceDialog>
#include <Sonnet/BackgroundChecker>
#include <Sonnet/Dialog>
#include <kcoreaddons_version.h>

#include <QElapsedTimer>
#include <QPointer>
#include <QTimer>

#define IGNOREACCELS KFind::MinimumUserOption
#define INCLUDENOTES KFind::MinimumUserOption * 2

static long makeOptions(long options, const Ui_findExtension *ui_findExtension)
{
    return options + IGNOREACCELS * ui_findExtension->m_ignoreAccelMarks->isChecked() + INCLUDENOTES * ui_findExtension->m_notes->isChecked();
}

class EntryFindDialog : public KFindDialog
{
public:
    explicit EntryFindDialog(QWidget *parent);
    ~EntryFindDialog();
    long options() const
    {
        return makeOptions(KFindDialog::options(), ui_findExtension);
    }
    static EntryFindDialog *instance(QWidget *parent = nullptr);

private:
    static QPointer<EntryFindDialog> _instance;
    static void cleanup()
    {
        delete EntryFindDialog::_instance;
    }

private:
    Ui_findExtension *ui_findExtension;
};

QPointer<EntryFindDialog> EntryFindDialog::_instance = nullptr;
EntryFindDialog *EntryFindDialog::instance(QWidget *parent)
{
    if (_instance == nullptr) {
        _instance = new EntryFindDialog(parent);
        qAddPostRoutine(EntryFindDialog::cleanup);
    }
    return _instance;
}

EntryFindDialog::EntryFindDialog(QWidget *parent)
    : KFindDialog(parent)
    , ui_findExtension(new Ui_findExtension)
{
    ui_findExtension->setupUi(findExtension());
    setHasSelection(false);

    KConfig config;
    KConfigGroup stateGroup(&config, QStringLiteral("FindReplace"));
    setOptions(stateGroup.readEntry("FindOptions", (qlonglong)0));
    setFindHistory(stateGroup.readEntry("FindHistory", QStringList()));
}

EntryFindDialog::~EntryFindDialog()
{
    KConfig config;
    KConfigGroup stateGroup(&config, QStringLiteral("FindReplace"));
    stateGroup.writeEntry("FindOptions", (qlonglong)options());
    stateGroup.writeEntry("FindHistory", findHistory());

    delete ui_findExtension;
}

// BEGIN EntryReplaceDialog
class EntryReplaceDialog : public KReplaceDialog
{
public:
    explicit EntryReplaceDialog(QWidget *parent);
    ~EntryReplaceDialog();
    long options() const
    {
        return makeOptions(KReplaceDialog::options(), ui_findExtension);
    }
    static EntryReplaceDialog *instance(QWidget *parent = nullptr);

private:
    static QPointer<EntryReplaceDialog> _instance;
    static void cleanup()
    {
        delete EntryReplaceDialog::_instance;
    }

private:
    Ui_findExtension *ui_findExtension;
};

QPointer<EntryReplaceDialog> EntryReplaceDialog::_instance = nullptr;
EntryReplaceDialog *EntryReplaceDialog::instance(QWidget *parent)
{
    if (_instance == nullptr) {
        _instance = new EntryReplaceDialog(parent);
        qAddPostRoutine(EntryReplaceDialog::cleanup);
    }
    return _instance;
}

EntryReplaceDialog::EntryReplaceDialog(QWidget *parent)
    : KReplaceDialog(parent)
    , ui_findExtension(new Ui_findExtension)
{
    ui_findExtension->setupUi(findExtension());
    setHasSelection(false);

    KConfig config;
    KConfigGroup stateGroup(&config, QStringLiteral("FindReplace"));
    setOptions(stateGroup.readEntry("ReplaceOptions", (qlonglong)0));
    setFindHistory(stateGroup.readEntry("ReplacePatternHistory", QStringList()));
    setReplacementHistory(stateGroup.readEntry("ReplacementHistory", QStringList()));
}

EntryReplaceDialog::~EntryReplaceDialog()
{
    KConfig config;
    KConfigGroup stateGroup(&config, QStringLiteral("FindReplace"));
    stateGroup.writeEntry("ReplaceOptions", (qlonglong)options());
    stateGroup.writeEntry("ReplacePatternHistory", findHistory());
    stateGroup.writeEntry("ReplacementHistory", replacementHistory());

    delete ui_findExtension;
}
// END EntryReplaceDialog

// TODO &amp;, &nbsp; ...
static void calcOffsetWithAccels(const QString &data, int &offset, int &length)
{
    int i = 0;
    for (; i < offset; ++i)
        if (Q_UNLIKELY(data.at(i) == QLatin1Char('&')))
            ++offset;

    // if & is inside highlighted word
    int limit = offset + length;
    for (i = offset; i < limit; ++i)
        if (Q_UNLIKELY(data.at(i) == QLatin1Char('&'))) {
            ++length;
            limit = qMin(data.size(), offset + length); // just safety
        }
}
static bool determineStartingPos(Catalog *m_catalog, KFind *find, DocPosition &pos) // search or replace
// called from find() and findNext()
{
    if (find->options() & KFind::FindBackwards) {
        pos.entry = m_catalog->numberOfEntries() - 1;
        pos.form = (m_catalog->isPlural(pos.entry)) ? m_catalog->numberOfPluralForms() - 1 : 0;
    } else {
        pos.entry = 0;
        pos.form = 0;
    }
    return true;
}

void EditorTab::find()
{
    EntryFindDialog::instance(nativeParentWidget());

    QString sel = selectionInTarget();
    if (!(sel.isEmpty() && selectionInSource().isEmpty())) {
        if (sel.isEmpty())
            sel = selectionInSource();
        if (m_find && m_find->options() & IGNOREACCELS)
            sel.remove(QLatin1Char('&'));
        EntryFindDialog::instance()->setPattern(sel);
    }

    if (EntryFindDialog::instance()->exec() != QDialog::Accepted)
        return;

    if (m_find) {
        m_find->resetCounts();
        m_find->setPattern(EntryFindDialog::instance()->pattern());
        m_find->setOptions(EntryFindDialog::instance()->options());

    } else { // This creates a find-next-prompt dialog if needed.
        m_find = new KFind(EntryFindDialog::instance()->pattern(), EntryFindDialog::instance()->options(), this, EntryFindDialog::instance());
        connect(m_find, &KFind::textFound, this, &EditorTab::highlightFound);
        connect(m_find, &KFind::findNext, this, qOverload<>(&EditorTab::findNext));
        m_find->closeFindNextDialog();
    }

    DocPosition pos;
    if (m_find->options() & KFind::FromCursor)
        pos = m_currentPos;
    else if (!determineStartingPos(m_catalog, m_find, pos))
        return;

    findNext(pos);
}

void EditorTab::findNext(const DocPosition &startingPos)
{
    Catalog &catalog = *m_catalog;
    KFind &find = *m_find;

    if (Q_UNLIKELY(catalog.numberOfEntries() <= startingPos.entry))
        return; // for the case when app wasn't able to process event before file close

    bool anotherEntry = _searchingPos.entry != m_currentPos.entry;
    _searchingPos = startingPos;

    if (anotherEntry)
        _searchingPos.offset = 0;

    bool ignoreaccels = m_find->options() & IGNOREACCELS;
    bool includenotes = m_find->options() & INCLUDENOTES;
    int switchOptions = DocPosition::Source | DocPosition::Target | (includenotes * DocPosition::Comment);
    int flag = 1;
    while (flag) {
        flag = 0;
        KFind::Result res = KFind::NoMatch;
        while (true) {
            if (find.needData() || anotherEntry || m_view->m_modifiedAfterFind) {
                anotherEntry = false;
                m_view->m_modifiedAfterFind = false;

                QString data;
                if (_searchingPos.part == DocPosition::Comment)
                    data = catalog.notes(_searchingPos).at(_searchingPos.form).content;
                else
                    data = catalog.catalogString(_searchingPos).string;

                if (ignoreaccels)
                    data.remove(QLatin1Char('&'));
                find.setData(data);
            }

            res = find.find();
            if (res != KFind::NoMatch)
                break;

            if (!((find.options() & KFind::FindBackwards) ? switchPrev(m_catalog, _searchingPos, switchOptions)
                                                          : switchNext(m_catalog, _searchingPos, switchOptions)))
                break;
        }

        if (res == KFind::NoMatch) {
            // file-wide search
            if (find.shouldRestart(true, true)) {
                flag = 1;
                determineStartingPos(m_catalog, m_find, _searchingPos);
            }
            find.resetCounts();
        }
    }
}

void EditorTab::findNext()
{
    if (m_find) {
        findNext((m_currentPos.entry == _searchingPos.entry && _searchingPos.part == DocPosition::Comment) ? _searchingPos : m_currentPos);
    } else
        find();
}

void EditorTab::findPrev()
{
    if (m_find) {
        m_find->setOptions(m_find->options() ^ KFind::FindBackwards);
        findNext(m_currentPos);
    } else {
        find();
    }
}

void EditorTab::highlightFound(const QString &, int matchingIndex, int matchedLength)
{
    if (m_find->options() & IGNOREACCELS && _searchingPos.part != DocPosition::Comment) {
        QString data = m_catalog->catalogString(_searchingPos).string;
        calcOffsetWithAccels(data, matchingIndex, matchedLength);
    }

    _searchingPos.offset = matchingIndex;
    gotoEntry(_searchingPos, matchedLength);
}

void EditorTab::replace()
{
    EntryReplaceDialog::instance(nativeParentWidget());

    if (!m_view->selectionInTarget().isEmpty()) {
        if (m_replace && m_replace->options() & IGNOREACCELS) {
            QString tmp(m_view->selectionInTarget());
            tmp.remove(QLatin1Char('&'));
            EntryReplaceDialog::instance()->setPattern(tmp);
        } else
            EntryReplaceDialog::instance()->setPattern(m_view->selectionInTarget());
    }

    if (EntryReplaceDialog::instance()->exec() != QDialog::Accepted)
        return;

    if (m_replace)
        m_replace->deleteLater(); // _replace=0;

    // This creates a find-next-prompt dialog if needed.
    {
        m_replace = new KReplace(EntryReplaceDialog::instance()->pattern(),
                                 EntryReplaceDialog::instance()->replacement(),
                                 EntryReplaceDialog::instance()->options(),
                                 this,
                                 EntryReplaceDialog::instance());
        connect(m_replace, &KReplace::textFound, this, &EditorTab::highlightFound_);
        connect(m_replace, &KReplace::findNext, this, qOverload<>(&EditorTab::replaceNext));
        connect(m_replace, &KReplace::textReplaced, this, &EditorTab::doReplace);
        connect(m_replace, &KReplace::dialogClosed, this, &EditorTab::cleanupReplace);
    }
    m_doReplaceCalled = false;

    if (m_replace->options() & KFind::FromCursor)
        replaceNext(m_currentPos);
    else {
        DocPosition pos;
        if (!determineStartingPos(m_catalog, m_replace, pos))
            return;
        replaceNext(pos);
    }
}

void EditorTab::replaceNext(const DocPosition &startingPos)
{
    bool anotherEntry = m_currentPos.entry != _replacingPos.entry;
    _replacingPos = startingPos;

    if (anotherEntry)
        _replacingPos.offset = 0;

    int flag = 1;
    bool ignoreaccels = m_replace->options() & IGNOREACCELS;
    bool includenotes = m_replace->options() & INCLUDENOTES;
    qCWarning(LOKALIZE_LOG) << "includenotes" << includenotes;
    int switchOptions = DocPosition::Target | (includenotes * DocPosition::Comment);
    while (flag) {
        flag = 0;
        KFind::Result res = KFind::NoMatch;
        while (true) {
            if (m_replace->needData() || anotherEntry) {
                anotherEntry = false;

                QString data;
                if (_replacingPos.part == DocPosition::Comment)
                    data = m_catalog->notes(_replacingPos).at(_replacingPos.form).content;
                else {
                    data = m_catalog->targetWithTags(_replacingPos).string;
                    if (ignoreaccels)
                        data.remove(QLatin1Char('&'));
                }
                m_replace->setData(data);
            }
            res = m_replace->replace();
            if (res != KFind::NoMatch)
                break;

            if (!((m_replace->options() & KFind::FindBackwards) ? switchPrev(m_catalog, _replacingPos, switchOptions)
                                                                : switchNext(m_catalog, _replacingPos, switchOptions)))
                break;
        }

        if (res == KFind::NoMatch) {
            if ((m_replace->options() & KFind::FromCursor) && m_replace->shouldRestart(true)) {
                flag = 1;
                determineStartingPos(m_catalog, m_replace, _replacingPos);
            } else {
                if (!(m_replace->options() & KFind::FromCursor))
                    m_replace->displayFinalDialog();

                m_replace->closeReplaceNextDialog();
                cleanupReplace();
            }
            m_replace->resetCounts();
        }
    }
}

void EditorTab::cleanupReplace()
{
    if (m_doReplaceCalled) {
        m_doReplaceCalled = false;
        m_catalog->endMacro();
    }
}

void EditorTab::replaceNext()
{
    replaceNext(m_currentPos);
}

void EditorTab::highlightFound_(const QString &, int matchingIndex, int matchedLength)
{
    if (m_replace->options() & IGNOREACCELS) {
        QString data = m_catalog->targetWithTags(_replacingPos).string;
        calcOffsetWithAccels(data, matchingIndex, matchedLength);
    }

    _replacingPos.offset = matchingIndex;
    gotoEntry(_replacingPos, matchedLength);
}

void EditorTab::doReplace(const QString &newStr, int offset, int newLen, int remLen)
{
    if (!m_doReplaceCalled) {
        m_doReplaceCalled = true;
        m_catalog->beginMacro(i18nc("@item Undo action item", "Replace"));
    }
    DocPosition pos = _replacingPos;
    if (_replacingPos.part == DocPosition::Comment)
        m_catalog->push(new SetNoteCmd(m_catalog, pos, Note(newStr)));
    else {
        QString oldStr = m_catalog->target(_replacingPos);

        if (m_replace->options() & IGNOREACCELS)
            calcOffsetWithAccels(oldStr, offset, remLen);

        pos.offset = offset;
        m_catalog->push(new DelTextCmd(m_catalog, pos, oldStr.mid(offset, remLen)));

        if (newLen)
            m_catalog->push(new InsTextCmd(m_catalog, pos, newStr.mid(offset, newLen)));
    }
    if (pos.entry == m_currentPos.entry) {
        pos.offset += newLen;
        m_view->gotoEntry(pos, 0);
    }
}

void EditorTab::spellcheck()
{
    if (!m_sonnetDialog) {
        m_sonnetChecker = new Sonnet::BackgroundChecker(this);
        m_sonnetChecker->changeLanguage(enhanceLangCode(Project::instance()->langCode()));
        m_sonnetChecker->setAutoDetectLanguageDisabled(true);
        m_sonnetDialog = new Sonnet::Dialog(m_sonnetChecker, this);
        connect(m_sonnetDialog, &Sonnet::Dialog::spellCheckDone, this, &EditorTab::spellcheckNext);
        connect(m_sonnetDialog, &Sonnet::Dialog::replace, this, &EditorTab::spellcheckReplace);
        connect(m_sonnetDialog, &Sonnet::Dialog::stop, this, &EditorTab::spellcheckStop);
        connect(m_sonnetDialog, &Sonnet::Dialog::cancel, this, &EditorTab::spellcheckCancel);

        connect(m_sonnetDialog, &Sonnet::Dialog::misspelling, this, &EditorTab::spellcheckShow);
    }

    QString text = m_catalog->msgstr(m_currentPos);
    if (!m_view->selectionInTarget().isEmpty())
        text = m_view->selectionInTarget();
    text.remove(QLatin1Char('&'));
    m_sonnetDialog->setBuffer(text);

    _spellcheckPos = m_currentPos;
    _spellcheckStartPos = m_currentPos;
    m_spellcheckStop = false;
    m_spellcheckStartUndoIndex = m_catalog->index();
    m_sonnetDialog->show();
}

void EditorTab::spellcheckNext()
{
    if (m_spellcheckStop)
        return;

    do {
        if (!switchNext(m_catalog, _spellcheckPos)) {
            qCDebug(LOKALIZE_LOG) << _spellcheckStartPos.entry;
            qCDebug(LOKALIZE_LOG) << _spellcheckStartPos.form;
            bool continueFromStart = !(_spellcheckStartPos.entry == 0 && _spellcheckStartPos.form == 0)
                && KMessageBox::PrimaryAction
                    == KMessageBox::questionTwoActions(this,
                                                       i18n("Lokalize has reached end of document. Do you want to continue from start?"),
                                                       i18nc("@title", "Spellcheck"),
                                                       KStandardGuiItem::cont(),
                                                       KStandardGuiItem::cancel());
            if (continueFromStart) {
                _spellcheckStartPos.entry = 0;
                _spellcheckStartPos.form = 0;
                _spellcheckPos = _spellcheckStartPos;
            } else {
                KMessageBox::information(this, i18n("Lokalize has finished spellchecking"), i18nc("@title", "Spellcheck"));
                return;
            }
        }
    } while (m_catalog->msgstr(_spellcheckPos).isEmpty() || !m_catalog->isApproved(_spellcheckPos.entry));

    m_sonnetDialog->setBuffer(m_catalog->msgstr(_spellcheckPos).remove(Project::instance()->accel()));
}

void EditorTab::spellcheckStop()
{
    m_spellcheckStop = true;
}

void EditorTab::spellcheckCancel()
{
    m_catalog->setIndex(m_spellcheckStartUndoIndex);
    gotoEntry(_spellcheckPos);
}

void EditorTab::spellcheckShow(const QString &word, int offset)
{
    const Project &project = *Project::instance();
    const QString accel = project.accel();

    QString source = m_catalog->source(_spellcheckPos);
    source.remove(accel);
    if (source.contains(word) && QStringView(project.targetLangCode()).left(2) != QStringView(project.sourceLangCode()).left(2)) {
        m_sonnetDialog->setUpdatesEnabled(false);
        m_sonnetChecker->continueChecking();
        return;
    }

    m_sonnetDialog->setUpdatesEnabled(true);

    show();

    DocPosition pos = _spellcheckPos;
    int length = word.length();
    calcOffsetWithAccels(m_catalog->target(pos), offset, length);
    pos.offset = offset;

    gotoEntry(pos, length);
}

void EditorTab::spellcheckReplace(QString oldWord, int offset, const QString &newWord)
{
    DocPosition pos = _spellcheckPos;
    int length = oldWord.length();
    calcOffsetWithAccels(m_catalog->target(pos), offset, length);
    pos.offset = offset;
    if (length > oldWord.length()) // replaced word contains accel mark
        oldWord = m_catalog->target(pos).mid(offset, length);

    m_catalog->push(new DelTextCmd(m_catalog, pos, oldWord));
    m_catalog->push(new InsTextCmd(m_catalog, pos, newWord));

    gotoEntry(pos, newWord.length());
}
