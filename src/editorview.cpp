/*
  This file is part of Lokalize (some bits of KBabel code were reused)

  SPDX-FileCopyrightText: 2007-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 1999-2000 Matthias Kiefer <matthias.kiefer@gmx.de>
  SPDX-FileCopyrightText: 2001-2004 Stanislav Visnovsky <visnovsky@kde.org>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/

#include "editorview.h"

#include "lokalize_debug.h"

#include "catalog.h"
#include "cmd.h"
#include "prefs.h"
#include "prefs_lokalize.h"
#include "project.h"
#include "xlifftextedit.h"

#include <QDragEnterEvent>
#include <QMenu>
#include <QTimer>

#include <QHBoxLayout>
#include <QLabel>
#include <QRegularExpression>
#include <QStringBuilder>
#include <QTabBar>

#include <kcolorscheme.h>
#include <kled.h>
#include <klocalizedstring.h>
#include <kmessagebox.h>
#include <kstandardshortcut.h>

// parent is set on qsplitter insertion
LedsWidget::LedsWidget(QWidget *parent)
    : QWidget(parent)
{
    KColorScheme colorScheme(QPalette::Normal);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addStretch();
    layout->addWidget(new QLabel(i18nc("@label whether entry is fuzzy", "Not ready:"), this));
    layout->addWidget(ledFuzzy = new KLed(colorScheme.foreground(KColorScheme::NeutralText).color() /*Qt::green*/, KLed::Off, KLed::Sunken, KLed::Rectangular));
    layout->addWidget(new QLabel(i18nc("@label whether entry is untranslated", "Untranslated:"), this));
    layout->addWidget(ledUntr = new KLed(colorScheme.foreground(KColorScheme::NegativeText).color() /*Qt::red*/, KLed::Off, KLed::Sunken, KLed::Rectangular));
    layout->addSpacing(1);
    layout->addWidget(lblColumn = new QLabel(this));
    layout->addStretch();
    setMaximumHeight(minimumSizeHint().height());
}

void LedsWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu;
    menu.addAction(i18nc("@action", "Hide"));
    if (!menu.exec(event->globalPos()))
        return; // NOTE the config doesn't seem to work
    Settings::setLeds(false);
    SettingsController::instance()->dirty = true;
    hide();
}

void LedsWidget::cursorPositionChanged(int column)
{
    lblColumn->setText(i18nc("@info:label cursor position", "Column: %1", column));
}

EditorView::EditorView(QWidget *parent, Catalog *catalog /*,keyEventHandler* kh*/)
    : QSplitter(Qt::Vertical, parent)
    , m_catalog(catalog)
    , m_sourceTextEdit(new TranslationUnitTextEdit(catalog, DocPosition::Source, this))
    , m_targetTextEdit(new TranslationUnitTextEdit(catalog, DocPosition::Target, this))
    , m_pluralTabBar(new QTabBar(this))
{
    m_pluralTabBar->hide();
    m_sourceTextEdit->setWhatsThis(
        i18n("<qt><p><b>Original String</b></p>\n"
             "<p>This part of the window shows the original message\n"
             "of the currently displayed entry.</p></qt>"));
    m_sourceTextEdit->viewport()->setBackgroundRole(QPalette::Window);

    m_sourceTextEdit->setVisualizeSeparators(Settings::self()->visualizeSeparators());
    m_targetTextEdit->setVisualizeSeparators(Settings::self()->visualizeSeparators());

    connect(m_targetTextEdit, &TranslationUnitTextEdit::contentsModified, this, &EditorView::resetFindForCurrent);
    connect(m_targetTextEdit, &TranslationUnitTextEdit::toggleApprovementRequested, this, &EditorView::toggleApprovement);
    connect(this, &EditorView::signalApprovedEntryDisplayed, m_targetTextEdit, &TranslationUnitTextEdit::reflectApprovementState);
    connect(m_sourceTextEdit, &TranslationUnitTextEdit::tagInsertRequested, m_targetTextEdit, &TranslationUnitTextEdit::insertTag);

    connect(m_sourceTextEdit, &TranslationUnitTextEdit::binaryUnitSelectRequested, this, &EditorView::binaryUnitSelectRequested);
    connect(m_targetTextEdit, &TranslationUnitTextEdit::binaryUnitSelectRequested, this, &EditorView::binaryUnitSelectRequested);
    connect(m_sourceTextEdit, &TranslationUnitTextEdit::gotoEntryRequested, this, &EditorView::gotoEntryRequested);
    connect(m_targetTextEdit, &TranslationUnitTextEdit::gotoEntryRequested, this, &EditorView::gotoEntryRequested);

    connect(m_sourceTextEdit, &TranslationUnitTextEdit::tmLookupRequested, this, &EditorView::tmLookupRequested);
    connect(m_targetTextEdit, &TranslationUnitTextEdit::tmLookupRequested, this, &EditorView::tmLookupRequested);

    connect(m_sourceTextEdit, &TranslationUnitTextEdit::findRequested, this, &EditorView::findRequested);
    connect(m_targetTextEdit, &TranslationUnitTextEdit::findRequested, this, &EditorView::findRequested);
    connect(m_sourceTextEdit, &TranslationUnitTextEdit::findNextRequested, this, &EditorView::findNextRequested);
    connect(m_targetTextEdit, &TranslationUnitTextEdit::findNextRequested, this, &EditorView::findNextRequested);
    connect(m_sourceTextEdit, &TranslationUnitTextEdit::replaceRequested, this, &EditorView::replaceRequested);
    connect(m_targetTextEdit, &TranslationUnitTextEdit::replaceRequested, this, &EditorView::replaceRequested);
    connect(m_sourceTextEdit, &TranslationUnitTextEdit::zoomRequested, m_targetTextEdit, &TranslationUnitTextEdit::zoomRequestedSlot);
    connect(m_targetTextEdit, &TranslationUnitTextEdit::zoomRequested, m_sourceTextEdit, &TranslationUnitTextEdit::zoomRequestedSlot);

    connect(this, &EditorView::doExplicitCompletion, m_targetTextEdit, &TranslationUnitTextEdit::doExplicitCompletion);

    addWidget(m_pluralTabBar);
    addWidget(m_sourceTextEdit);
    addWidget(m_targetTextEdit);

    QWidget::setTabOrder(m_targetTextEdit, m_sourceTextEdit);
    QWidget::setTabOrder(m_sourceTextEdit, m_targetTextEdit);
    setFocusProxy(m_targetTextEdit);
    settingsChanged();
}

EditorView::~EditorView()
{
}

void EditorView::resetFindForCurrent(const DocPosition &pos)
{
    m_modifiedAfterFind = true;
    Q_EMIT signalChanged(pos.entry);
}

void EditorView::settingsChanged()
{
    // Settings::self()->config()->setGroup("Editor");
    m_sourceTextEdit->document()->setDefaultFont(Settings::msgFont());
    m_targetTextEdit->document()->setDefaultFont(Settings::msgFont());
    m_sourceTextEdit->setVisualizeSeparators(Settings::self()->visualizeSeparators());
    m_targetTextEdit->setVisualizeSeparators(Settings::self()->visualizeSeparators());
    if (m_leds)
        m_leds->setVisible(Settings::leds());
    else if (Settings::leds()) {
        m_leds = new LedsWidget(this);
        insertWidget(2, m_leds);
        connect(m_targetTextEdit, &TranslationUnitTextEdit::cursorPositionChanged, m_leds, &LedsWidget::cursorPositionChanged);
        connect(m_targetTextEdit, &TranslationUnitTextEdit::nonApprovedEntryDisplayed, m_leds->ledFuzzy, &KLed::on);
        connect(m_targetTextEdit, &TranslationUnitTextEdit::approvedEntryDisplayed, m_leds->ledFuzzy, &KLed::off);
        connect(m_targetTextEdit, &TranslationUnitTextEdit::untranslatedEntryDisplayed, m_leds->ledUntr, &KLed::on);
        connect(m_targetTextEdit, &TranslationUnitTextEdit::translatedEntryDisplayed, m_leds->ledUntr, &KLed::off);
        m_targetTextEdit->showPos(m_targetTextEdit->currentPos());
    }
}

void EditorView::gotoEntry()
{
    return gotoEntry(DocPosition(), 0);
}
// main function in this file :)
void EditorView::gotoEntry(DocPosition pos, int selection)
{
    setUpdatesEnabled(false);

    bool refresh = (pos.entry == -1);
    if (refresh)
        pos = m_targetTextEdit->currentPos();
    // qCWarning(LOKALIZE_LOG)<<"refresh"<<refresh;
    // qCWarning(LOKALIZE_LOG)<<"offset"<<pos.offset;
    // TODO trigger refresh directly via Catalog signal

    if (Q_UNLIKELY(m_catalog->isPlural(pos.entry))) {
        if (Q_UNLIKELY(m_catalog->numberOfPluralForms() != m_pluralTabBar->count())) {
            int i = m_pluralTabBar->count();
            if (m_catalog->numberOfPluralForms() > m_pluralTabBar->count())
                while (i < m_catalog->numberOfPluralForms())
                    m_pluralTabBar->addTab(i18nc("@title:tab", "Plural Form %1", ++i));
            else
                while (i > m_catalog->numberOfPluralForms())
                    m_pluralTabBar->removeTab(i--);
        }
        m_pluralTabBar->show();
        m_pluralTabBar->blockSignals(true);
        m_pluralTabBar->setCurrentIndex(pos.form);
        m_pluralTabBar->blockSignals(false);
    } else
        m_pluralTabBar->hide();

    // bool keepCursor=DocPos(pos)==DocPos(_msgidEdit->currentPos());
    bool keepCursor = false;
    CatalogString sourceWithTags = m_sourceTextEdit->showPos(pos, CatalogString(), keepCursor);

    // qCWarning(LOKALIZE_LOG)<<"calling showPos";
    QString targetString = m_targetTextEdit->showPos(pos, sourceWithTags, keepCursor).string;
    // qCWarning(LOKALIZE_LOG)<<"ss"<<_msgstrEdit->textCursor().anchor()<<_msgstrEdit->textCursor().position();
    m_sourceTextEdit->cursorToStart();
    m_targetTextEdit->cursorToStart();

    bool untrans = targetString.isEmpty();
    // qCWarning(LOKALIZE_LOG)<<"ss1"<<_msgstrEdit->textCursor().anchor()<<_msgstrEdit->textCursor().position();

    if (pos.offset || selection) {
        TranslationUnitTextEdit *msgEdit = (pos.part == DocPosition::Source ? m_sourceTextEdit : m_targetTextEdit);
        QTextCursor t = msgEdit->textCursor();
        t.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor, pos.offset);
        // NOTE this was kinda bug due to on-the-fly msgid wordwrap
        if (selection)
            t.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, selection);
        msgEdit->setTextCursor(t);
    } else if (!untrans) {
        QTextCursor t = m_targetTextEdit->textCursor();
        // what if msg starts with a tag?
        if (Q_UNLIKELY(targetString.startsWith(QLatin1Char('<')))) {
            const QRegularExpression regExp(QStringLiteral(">[^<]"));
            const auto match = regExp.match(targetString);
            if (match.hasMatch())
                t.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor, match.capturedStart() + 1);
        } else if (Q_UNLIKELY(targetString.startsWith(TAGRANGE_IMAGE_SYMBOL))) {
            const QRegularExpression regExp(QStringLiteral("[^") + QChar(TAGRANGE_IMAGE_SYMBOL) + QLatin1Char(']'));
            const auto match = regExp.match(targetString);
            if (match.hasMatch())
                t.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor, match.capturedStart() + 1);
        }
        m_targetTextEdit->setTextCursor(t);
    }
    // qCWarning(LOKALIZE_LOG)<<"set-->"<<_msgstrEdit->textCursor().anchor()<<_msgstrEdit->textCursor().position();
    // qCWarning(LOKALIZE_LOG)<<"anchor"<<t.anchor()<<"pos"<<t.position();
    m_targetTextEdit->setFocus();
    setUpdatesEnabled(true);
}

// BEGIN edit actions that are easier to do in this class
void EditorView::unwrap()
{
    unwrap(nullptr);
}
void EditorView::unwrap(TranslationUnitTextEdit *editor)
{
    if (!editor)
        editor = m_targetTextEdit;

    QTextCursor t = editor->document()->find(QRegularExpression(QStringLiteral("[^(\\\\n)]$")));
    if (t.isNull())
        return;

    if (editor == m_targetTextEdit)
        m_catalog->beginMacro(i18nc("@item Undo action item", "Unwrap"));
    t.movePosition(QTextCursor::EndOfLine);
    if (!t.atEnd())
        t.deleteChar();

    const QRegularExpression rx(QStringLiteral("[^(\\\\n)>]$"));
    // remove '\n's skipping "\\\\n"
    while (!(t = editor->document()->find(rx, t)).isNull()) {
        t.movePosition(QTextCursor::EndOfLine);
        if (!t.atEnd())
            t.deleteChar();
    }
    if (editor == m_targetTextEdit)
        m_catalog->endMacro();
}

void EditorView::insertTerm(const QString &term)
{
    m_targetTextEdit->insertPlainTextWithCursorCheck(term);
    m_targetTextEdit->setFocus();
}

QString EditorView::selectionInTarget() const
{
    // TODO remove IMAGES
    return m_targetTextEdit->textCursor().selectedText();
}

QString EditorView::selectionInSource() const
{
    // TODO remove IMAGES
    return m_sourceTextEdit->textCursor().selectedText();
}

void EditorView::setProperFocus()
{
    m_targetTextEdit->setFocus();
}

// END edit actions that are easier to do in this class

TranslationUnitTextEdit *EditorView::viewPort()
{
    return m_targetTextEdit;
}

void EditorView::toggleBookmark(bool checked)
{
    if (Q_UNLIKELY(m_targetTextEdit->currentPos().entry == -1))
        return;

    m_catalog->setBookmark(m_targetTextEdit->currentPos().entry, checked);
}

void EditorView::toggleApprovement()
{
    // qCWarning(LOKALIZE_LOG)<<"called";
    if (Q_UNLIKELY(m_targetTextEdit->currentPos().entry == -1))
        return;

    bool newState = !m_catalog->isApproved(m_targetTextEdit->currentPos().entry);
    SetStateCmd::push(m_catalog, m_targetTextEdit->currentPos(), newState);
    Q_EMIT signalApprovedEntryDisplayed(newState);
}

void EditorView::setState(TargetState state)
{
    if (Q_UNLIKELY(m_targetTextEdit->currentPos().entry == -1 || m_catalog->state(m_targetTextEdit->currentPos()) == state))
        return;

    SetStateCmd::instantiateAndPush(m_catalog, m_targetTextEdit->currentPos(), state);
    Q_EMIT signalApprovedEntryDisplayed(m_catalog->isApproved(m_targetTextEdit->currentPos()));
}

void EditorView::setEquivTrans(bool equivTrans)
{
    m_catalog->push(new SetEquivTransCmd(m_catalog, m_targetTextEdit->currentPos(), equivTrans));
}

#include "moc_editorview.cpp"
