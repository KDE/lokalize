/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>
  SPDX-FileCopyrightText: 2023 Łukasz Wojniłowicz <lukasz.wojnilowicz@gmail.com>
  SPDX-FileCopyrightText: 2019, 2024 Karl Ove Hufthammer <karl@huftis.org>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "xlifftextedit.h"

#include "lokalize_debug.h"

#include "catalog.h"
#include "cmd.h"
#include "completionstorage.h"
#include "languagetoolmanager.h"
#include "languagetoolparser.h"
#include "languagetoolresultjob.h"
#include "prefs.h"
#include "prefs_lokalize.h"
#include "project.h"
#include "syntaxhighlighter.h"

#include <kcompletionbox.h>
#include <klocalizedstring.h>

#include <QApplication>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QMetaType>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QScrollBar>
#include <QStringBuilder>
#include <QStyle>
#include <QStyleOptionButton>
#include <QToolTip>

inline static QImage generateImage(const QString &str, const QFont &font)
{
    QStyleOptionButton opt;
    opt.fontMetrics = QFontMetrics(font);
    opt.text = QLatin1Char(' ') + str + QLatin1Char(' ');
    opt.rect = opt.fontMetrics.boundingRect(opt.text).adjusted(0, 0, 5, 5);
    opt.rect.moveTo(0, 0);

    QImage result(opt.rect.size(), QImage::Format_ARGB32);
    result.fill(0); // 0xAARRGGBB
    QPainter painter(&result);
    QApplication::style()->drawControl(QStyle::CE_PushButton, &opt, &painter);

    return result;
}
class MyCompletionBox : public KCompletionBox
{
public:
    explicit MyCompletionBox(QWidget *p)
        : KCompletionBox(p)
    {
    }
    ~MyCompletionBox() override = default;
    QSize sizeHint() const override;

    bool eventFilter(QObject *, QEvent *) override; // reimplemented to deliver more keypresses to XliffTextEdit
};

QSize MyCompletionBox::sizeHint() const
{
    int h = count() ? (sizeHintForRow(0)) : 0;
    h = qMin(count() * h, 10 * h) + 2 * frameWidth();
    int w = sizeHintForColumn(0) + verticalScrollBar()->width() + 2 * frameWidth();
    return QSize(w, h);
}

bool MyCompletionBox::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *e = static_cast<QKeyEvent *>(event);
        if (e->key() == Qt::Key_PageDown || e->key() == Qt::Key_PageUp) {
            hide();
            return false;
        }
    }
    return KCompletionBox::eventFilter(object, event);
}

TranslationUnitTextEdit::~TranslationUnitTextEdit()
{
    disconnect(document(), &QTextDocument::contentsChange, this, &TranslationUnitTextEdit::contentsChanged);
}

TranslationUnitTextEdit::TranslationUnitTextEdit(Catalog *catalog, DocPosition::Part part, QWidget *parent)
    : KTextEdit(parent)
    , m_catalog(catalog)
    , m_part(part)
    , m_highlighter(new SyntaxHighlighter(this))
    , m_enabled(Settings::autoSpellcheck())
    , m_languageToolTimer(new QTimer(this))
{
    setReadOnly(part == DocPosition::Source);
    setUndoRedoEnabled(false);
    setAcceptRichText(false);

    m_highlighter->setActive(m_enabled);
    setHighlighter(m_highlighter);

    if (part == DocPosition::Target) {
        connect(document(), &QTextDocument::contentsChange, this, &TranslationUnitTextEdit::contentsChanged);
        connect(this, &KTextEdit::cursorPositionChanged, this, &TranslationUnitTextEdit::emitCursorPositionChanged);
        connect(m_languageToolTimer, &QTimer::timeout, this, &TranslationUnitTextEdit::launchLanguageTool);
    }
    connect(catalog, qOverload<>(&Catalog::signalFileLoaded), this, &TranslationUnitTextEdit::fileLoaded);
    m_languageToolTimer->setSingleShot(true);
}

void TranslationUnitTextEdit::setSpellCheckingEnabled(bool enable)
{
    Settings::setAutoSpellcheck(enable);
    m_enabled = enable;
    m_highlighter->setActive(enable);
    SettingsController::instance()->dirty = true;
}

void TranslationUnitTextEdit::setVisualizeSeparators(bool enable)
{
    if (enable) {
        QTextOption textoption = document()->defaultTextOption();
        textoption.setFlags(textoption.flags() | QTextOption::ShowLineAndParagraphSeparators | QTextOption::ShowTabsAndSpaces);
        document()->setDefaultTextOption(textoption);
    } else {
        QTextOption textoption = document()->defaultTextOption();
        textoption.setFlags(textoption.flags() & (~QTextOption::ShowLineAndParagraphSeparators) & (~QTextOption::ShowTabsAndSpaces));
        document()->setDefaultTextOption(textoption);
    }
}

void TranslationUnitTextEdit::fileLoaded()
{
    QString langCode = m_part == DocPosition::Source ? m_catalog->sourceLangCode() : m_catalog->targetLangCode();

    QLocale langLocale(langCode);
    // First try to use a locale name derived from the language code
    m_highlighter->setCurrentLanguage(langLocale.name());
    //  If that fails, try to use the language code directly
    if (m_highlighter->currentLanguage() != langLocale.name() || m_highlighter->currentLanguage().isEmpty()) {
        m_highlighter->setCurrentLanguage(langCode);
        if (m_highlighter->currentLanguage() != langCode && langCode.length() > 2) {
            m_highlighter->setCurrentLanguage(langCode.left(2));
        }
    }
    m_highlighter->setAutoDetectLanguageDisabled(m_highlighter->spellCheckerFound());
    //"i use an english locale while translating kde pot files from english to hebrew" Bug #181989
    Qt::LayoutDirection targetLanguageDirection = Qt::LeftToRight;
    static const QLocale::Language rtlLanguages[] = {QLocale::Arabic, QLocale::Hebrew, QLocale::Urdu, QLocale::Persian, QLocale::Pashto};
    int i = sizeof(rtlLanguages) / sizeof(QLocale::Arabic);
    while (--i >= 0 && langLocale.language() != rtlLanguages[i])
        ;
    if (i != -1)
        targetLanguageDirection = Qt::RightToLeft;
    setLayoutDirection(targetLanguageDirection);

    if (m_part == DocPosition::Source)
        return;

    //"Some language do not need space between words. For example Chinese."
    static const QLocale::Language noSpaceLanguages[] = {QLocale::Chinese};
    i = sizeof(noSpaceLanguages) / sizeof(QLocale::Chinese);
    while (--i >= 0 && langLocale.language() != noSpaceLanguages[i])
        ;
    m_langUsesSpaces = (i == -1);
}

void TranslationUnitTextEdit::reflectApprovementState()
{
    if (m_part == DocPosition::Source || m_currentPos.entry == -1)
        return;

    bool approved = m_catalog->isApproved(m_currentPos.entry);

    disconnect(document(), &QTextDocument::contentsChange, this, &TranslationUnitTextEdit::contentsChanged);
    m_highlighter->setApprovementState(approved);
    m_highlighter->rehighlight();
    connect(document(), &QTextDocument::contentsChange, this, &TranslationUnitTextEdit::contentsChanged);
    viewport()->setBackgroundRole(approved ? QPalette::Base : QPalette::AlternateBase);

    if (approved)
        Q_EMIT approvedEntryDisplayed();
    else
        Q_EMIT nonApprovedEntryDisplayed();

    bool untr = m_catalog->isEmpty(m_currentPos);
    if (untr)
        Q_EMIT untranslatedEntryDisplayed();
    else
        Q_EMIT translatedEntryDisplayed();
}

void TranslationUnitTextEdit::reflectUntranslatedState()
{
    if (m_part == DocPosition::Source || m_currentPos.entry == -1)
        return;

    bool untr = m_catalog->isEmpty(m_currentPos);
    if (untr)
        Q_EMIT untranslatedEntryDisplayed();
    else
        Q_EMIT translatedEntryDisplayed();
}

/**
 * makes MsgEdit reflect current entry
 **/
CatalogString TranslationUnitTextEdit::showPos(DocPosition docPosition, const CatalogString &refStr, bool keepCursor)
{
    docPosition.part = m_part;
    m_currentPos = docPosition;

    CatalogString catalogString = m_catalog->catalogString(m_currentPos);
    QString target = catalogString.string;
    _oldMsgstr = target;

    // BEGIN pos
    QTextCursor cursor = textCursor();
    int pos = cursor.position();
    int anchor = cursor.anchor();
    if (!keepCursor && toPlainText() != target) {
        pos = 0;
        anchor = 0;
    }
    // END pos

    disconnect(document(), &QTextDocument::contentsChange, this, &TranslationUnitTextEdit::contentsChanged);
    if (docPosition.part == DocPosition::Source)
        setContent(catalogString);
    else
        setContent(catalogString, refStr.string.isEmpty() ? m_catalog->sourceWithTags(docPosition) : refStr);
    connect(document(), &QTextDocument::contentsChange, this, &TranslationUnitTextEdit::contentsChanged);

    _oldMsgstrAscii = document()->toPlainText();

    // BEGIN pos
    QTextCursor t = textCursor();
    t.movePosition(QTextCursor::Start);
    if (pos || anchor) {
        //  I don't know why the following (more correct) code does not work
        t.setPosition(anchor, QTextCursor::MoveAnchor);
        int length = pos - anchor;
        if (length)
            t.movePosition(length < 0 ? QTextCursor::PreviousCharacter : QTextCursor::NextCharacter, QTextCursor::KeepAnchor, qAbs(length));
    }
    setTextCursor(t);
    // END pos

    reflectApprovementState();
    reflectUntranslatedState();
    return catalogString; // for the sake of not calling XliffStorage/doContent twice
}

void TranslationUnitTextEdit::setContent(const CatalogString &catStr, const CatalogString &refStr)
{
    // prevent undo tracking system from recording this 'action'
    document()->blockSignals(true);
    clear();

    QTextCursor c = textCursor();
    insertContent(c, catStr, refStr);

    document()->blockSignals(false);

    if (m_part == DocPosition::Target)
        m_highlighter->setSourceString(refStr.string);
    else
        // reflectApprovementState() does this for Target
        m_highlighter->rehighlight(); // explicitly because the signals were disabled
    if (Settings::self()->languageToolDelay() > 0) {
        m_languageToolTimer->start(Settings::self()->languageToolDelay() * 1000);
    }
}

void insertContent(QTextCursor &cursor, const CatalogString &catStr, const CatalogString &refStr, bool insertText)
{
    // settings for TMView
    QTextCharFormat chF = cursor.charFormat();
    QFont font = cursor.document()->defaultFont();

    QMap<int, int> posToTag;
    int i = catStr.tags.size();
    while (--i >= 0) {
        posToTag.insert(catStr.tags.at(i).start, i);
        posToTag.insert(catStr.tags.at(i).end, i);
    }

    QMap<QString, int> sourceTagIdToIndex = refStr.tagIdToIndex();
    int refTagIndexOffset = sourceTagIdToIndex.size();

    i = 0;
    int prev = 0;
    while ((i = catStr.string.indexOf(TAGRANGE_IMAGE_SYMBOL, i)) != -1) {
        if (insertText)
            cursor.insertText(catStr.string.mid(prev, i - prev));
        else {
            cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor, i - prev);
            cursor.deleteChar(); // delete TAGRANGE_IMAGE_SYMBOL to insert it properly
        }

        if (!posToTag.contains(i)) {
            prev = ++i;
            continue;
        }
        int tagIndex = posToTag.value(i);
        InlineTag tag = catStr.tags.at(tagIndex);
        QString name = tag.id;
        QString text;
        if (tag.type == InlineTag::mrk)
            text = QStringLiteral("*");
        else if (!tag.equivText.isEmpty())
            text = tag.equivText; // TODO add number? when? -- right now this is done for gettext qt's 156 mark
        else
            text = QString::number(sourceTagIdToIndex.contains(tag.id) ? sourceTagIdToIndex.value(tag.id) : (tagIndex + refTagIndexOffset));
        if (tag.start != tag.end) {
            if (tag.start == i) {
                text.append(QLatin1String(" {"));
                name.append(QLatin1String("-start"));
            } else {
                text.prepend(QLatin1String("} "));
                name.append(QLatin1String("-end"));
            }
        }
        if (cursor.document()->resource(QTextDocument::ImageResource, QUrl(name)).isNull())
            cursor.document()->addResource(QTextDocument::ImageResource, QUrl(name), generateImage(text, font));
        cursor.insertImage(name); // NOTE what if twice the same name?
        cursor.setCharFormat(chF);

        prev = ++i;
    }
    cursor.insertText(catStr.string.mid(prev));
}

void TranslationUnitTextEdit::contentsChanged(int offset, int charsRemoved, int charsAdded)
{
    Q_ASSERT(!m_catalog->targetLangCode().isEmpty());
    Q_ASSERT(!Project::instance()->targetLangCode().isEmpty());

    // HACK to workaround #218246
    const QString &editTextAscii = document()->toPlainText();
    if (editTextAscii == _oldMsgstrAscii) {
        return;
    }

    const QString &editText = toPlainText();
    if (Q_UNLIKELY(m_currentPos.entry == -1 || editText == _oldMsgstr)) {
        return;
    }

    // ktextedit spellcheck handling:
    if (charsRemoved == 0 && editText.isEmpty() && !_oldMsgstr.isEmpty())
        charsRemoved = _oldMsgstr.length();
    if (charsAdded && editText.isEmpty())
        charsAdded = 0;
    if (charsRemoved && _oldMsgstr.isEmpty())
        charsRemoved = 0;

    DocPosition pos = m_currentPos;
    pos.offset = offset;

    QString target = m_catalog->targetWithTags(pos).string;
    const auto addedText = QStringView(editText).mid(offset, charsAdded);

    // BEGIN XLIFF markup handling
    // protect from tag removal
    // TODO use midRef when Qt 4.8 is in distros
    bool markupRemoved = charsRemoved && QStringView(target).mid(offset, charsRemoved).contains(TAGRANGE_IMAGE_SYMBOL);
    bool markupAdded = charsAdded && addedText.contains(TAGRANGE_IMAGE_SYMBOL);
    if (markupRemoved || markupAdded) {
        bool modified = false;
        CatalogString targetWithTags = m_catalog->targetWithTags(m_currentPos);
        // special case when the user presses Del w/o selection
        if (!charsAdded && charsRemoved == 1) {
            int i = targetWithTags.tags.size();
            while (--i >= 0) {
                if (targetWithTags.tags.at(i).start == offset || targetWithTags.tags.at(i).end == offset) {
                    modified = true;
                    pos.offset = targetWithTags.tags.at(i).start;
                    m_catalog->push(new DelTagCmd(m_catalog, pos));
                }
            }
        } else if (!markupAdded) { // check if all { plus } tags were selected
            modified = removeTargetSubstring(offset, charsRemoved, /*refresh*/ false);
            if (modified && charsAdded)
                m_catalog->push(new InsTextCmd(m_catalog, pos, addedText.toString()));
        }

        showPos(m_currentPos, CatalogString(), /*keepCursor*/ true);
        if (!modified) {
            return;
        }
    }
    // END XLIFF markup handling
    else {
        if (charsRemoved)
            m_catalog->push(new DelTextCmd(m_catalog, pos, _oldMsgstr.mid(offset, charsRemoved)));

        _oldMsgstr = editText; // newStr becomes OldStr
        _oldMsgstrAscii = editTextAscii;
        if (charsAdded)
            m_catalog->push(new InsTextCmd(m_catalog, pos, addedText.toString()));
    }

    requestToggleApprovement();
    reflectUntranslatedState();

    // for mergecatalog (remove entry from index)
    // and for statusbar
    Q_EMIT contentsModified(m_currentPos);
    if (charsAdded == 1) {
        auto sp = target.lastIndexOf(CompletionStorage::instance()->rxSplit, offset - 1);
        int len = (offset - sp);
        int wordCompletionLength = Settings::self()->wordCompletionLength();
        if (wordCompletionLength >= 3 && len >= wordCompletionLength)
            doCompletion(offset + 1);
        else if (m_completionBox)
            m_completionBox->hide();
    } else if (m_completionBox)
        m_completionBox->hide();
    // Start LanguageToolTimer
    if (Settings::self()->languageToolDelay() > 0) {
        m_languageToolTimer->start(Settings::self()->languageToolDelay() * 1000);
    }
}

bool TranslationUnitTextEdit::removeTargetSubstring(int delStart, int delLen, bool refresh)
{
    if (Q_UNLIKELY(m_currentPos.entry == -1))
        return false;

    if (!::removeTargetSubstring(m_catalog, m_currentPos, delStart, delLen))
        return false;

    requestToggleApprovement();

    if (refresh) {
        showPos(m_currentPos, CatalogString(), /*keepCursor*/ true);
    }
    Q_EMIT contentsModified(DocPosition(m_currentPos.entry));
    return true;
}

void TranslationUnitTextEdit::insertCatalogString(CatalogString catStr, int start, bool refresh)
{
    QString REMOVEME = QStringLiteral("REMOVEME");
    CatalogString sourceForReferencing = m_catalog->sourceWithTags(m_currentPos);
    const CatalogString target = m_catalog->targetWithTags(m_currentPos);

    QHash<QString, int> id2tagIndex;
    int i = sourceForReferencing.tags.size();
    while (--i >= 0)
        id2tagIndex.insert(sourceForReferencing.tags.at(i).id, i);

    // remove markup that is already in target, to avoid duplicates if the string being inserted contains it as well
    for (const InlineTag &tag : target.tags) {
        if (id2tagIndex.contains(tag.id))
            sourceForReferencing.tags[id2tagIndex.value(tag.id)].id = REMOVEME;
    }

    // iterating from the end is essential
    i = sourceForReferencing.tags.size();
    while (--i >= 0)
        if (sourceForReferencing.tags.at(i).id == REMOVEME)
            sourceForReferencing.tags.removeAt(i);

    adaptCatalogString(catStr, sourceForReferencing);

    ::insertCatalogString(m_catalog, m_currentPos, catStr, start);

    if (refresh) {
        showPos(m_currentPos, CatalogString(), /*keepCursor*/ true);
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor, catStr.string.size());
        setTextCursor(cursor);
    }
}

const QString LOKALIZE_XLIFF_MIMETYPE = QStringLiteral("application/x-lokalize-xliff+xml");

QMimeData *TranslationUnitTextEdit::createMimeDataFromSelection() const
{
    QMimeData *mimeData = new QMimeData;

    CatalogString catalogString = m_catalog->catalogString(m_currentPos);

    QTextCursor cursor = textCursor();
    int start = qMin(cursor.anchor(), cursor.position());
    int end = qMax(cursor.anchor(), cursor.position());

    QMap<int, int> tagPlaces;
    if (fillTagPlaces(tagPlaces, catalogString, start, end - start)) {
        // transform CatalogString
        // TODO substring method
        catalogString.string = catalogString.string.mid(start, end - start);

        QList<InlineTag>::iterator it = catalogString.tags.begin();
        while (it != catalogString.tags.end()) {
            if (!tagPlaces.contains(it->start))
                it = catalogString.tags.erase(it);
            else {
                it->start -= start;
                it->end -= start;
                ++it;
            }
        }

        QByteArray a;
        QDataStream out(&a, QIODevice::WriteOnly);
        QVariant v;
        v.setValue(catalogString);
        out << v;
        mimeData->setData(LOKALIZE_XLIFF_MIMETYPE, a);
    }

    QString text = catalogString.string;
    text.remove(TAGRANGE_IMAGE_SYMBOL);
    mimeData->setText(text);
    return mimeData;
}

void TranslationUnitTextEdit::dragEnterEvent(QDragEnterEvent *event)
{
    QObject *dragSource = event->source();
    if (dragSource && dragSource->objectName() == QLatin1String("qt_scrollarea_viewport"))
        dragSource = dragSource->parent();
    // This is a deplacement within the Target area
    if (m_part == DocPosition::Target && this == dragSource) {
        QTextCursor cursor = textCursor();
        int start = qMin(cursor.anchor(), cursor.position());
        int end = qMax(cursor.anchor(), cursor.position());

        m_cursorSelectionEnd = end;
        m_cursorSelectionStart = start;
    }
    QTextEdit::dragEnterEvent(event);
}
void TranslationUnitTextEdit::dropEvent(QDropEvent *event)
{
    // Ensure the cursor moves to the correct location
    if (m_part == DocPosition::Target) {
        setTextCursor(cursorForPosition(event->position().toPoint()));
        // This is a copy modifier, disable the selection flags
        if (event->modifiers() & Qt::ControlModifier) {
            m_cursorSelectionEnd = 0;
            m_cursorSelectionStart = 0;
        }
    }
    QTextEdit::dropEvent(event);
}

void TranslationUnitTextEdit::insertFromMimeData(const QMimeData *source)
{
    if (m_part == DocPosition::Source)
        return;

    if (source->hasFormat(LOKALIZE_XLIFF_MIMETYPE)) {
        QVariant v;
        QByteArray data = source->data(LOKALIZE_XLIFF_MIMETYPE);
        QDataStream in(&data, QIODevice::ReadOnly);
        in >> v;

        int start = 0;
        m_catalog->beginMacro(i18nc("@item Undo action item", "Insert text with markup"));
        QTextCursor cursor = textCursor();
        if (cursor.hasSelection()) {
            start = qMin(cursor.anchor(), cursor.position());
            int end = qMax(cursor.anchor(), cursor.position());
            removeTargetSubstring(start, end - start);
            cursor.setPosition(start);
            setTextCursor(cursor);
        } else
        // sets right cursor position implicitly -- needed for mouse paste
        {
            QMimeData mimeData;
            mimeData.setText(QString());

            if (m_cursorSelectionEnd != m_cursorSelectionStart) {
                int oldCursorPosition = textCursor().position();
                removeTargetSubstring(m_cursorSelectionStart, m_cursorSelectionEnd - m_cursorSelectionStart);
                if (oldCursorPosition >= m_cursorSelectionEnd) {
                    cursor.setPosition(oldCursorPosition - (m_cursorSelectionEnd - m_cursorSelectionStart));
                    setTextCursor(cursor);
                }
            }
            KTextEdit::insertFromMimeData(&mimeData);
            start = textCursor().position();
        }

        insertCatalogString(v.value<CatalogString>(), start);
        m_catalog->endMacro();
    } else {
        QString text = source->text();
        text.remove(TAGRANGE_IMAGE_SYMBOL);
        insertPlainTextWithCursorCheck(text);
    }
}

static bool isMasked(const QString &str, uint col)
{
    if (col == 0 || str.isEmpty())
        return false;

    uint counter = 0;
    int pos = col;

    while (pos >= 0 && str.at(pos) == QLatin1Char('\\')) {
        counter++;
        pos--;
    }

    return !(bool)(counter % 2);
}

void TranslationUnitTextEdit::keyPressEvent(QKeyEvent *keyEvent)
{
    QString spclChars = QStringLiteral("abfnrtv'?\\");

    if (keyEvent->matches(QKeySequence::MoveToPreviousPage))
        Q_EMIT gotoPrevRequested();
    else if (keyEvent->matches(QKeySequence::MoveToNextPage))
        Q_EMIT gotoNextRequested();
    else if (keyEvent->matches(QKeySequence::Undo))
        Q_EMIT undoRequested();
    else if (keyEvent->matches(QKeySequence::Redo))
        Q_EMIT redoRequested();
    else if (keyEvent->matches(QKeySequence::Find))
        Q_EMIT findRequested();
    else if (keyEvent->matches(QKeySequence::FindNext))
        Q_EMIT findNextRequested();
    else if (keyEvent->matches(QKeySequence::Replace))
        Q_EMIT replaceRequested();
    else if (keyEvent->modifiers() == (Qt::AltModifier | Qt::ControlModifier)) {
        if (keyEvent->key() == Qt::Key_Home)
            Q_EMIT gotoFirstRequested();
        else if (keyEvent->key() == Qt::Key_End)
            Q_EMIT gotoLastRequested();
    } else if (keyEvent->matches(QKeySequence::MoveToNextLine) || keyEvent->matches(QKeySequence::MoveToPreviousLine)) {
        keyEvent->setAccepted(true);
        bool up = keyEvent->key() == Qt::Key_Up;
        QTextCursor c = textCursor();
        if (!c.movePosition(up ? QTextCursor::Up : QTextCursor::Down)) {
            QTextCursor::MoveOperation op;
            if (up && !c.atStart())
                op = QTextCursor::Start;
            else if (!up && !c.atEnd())
                op = QTextCursor::End;
            else if (up) {
                Q_EMIT gotoPrevRequested();
                op = QTextCursor::End;
            } else {
                Q_EMIT gotoNextRequested();
                op = QTextCursor::Start;
            }
            c.movePosition(op);
        }
        setTextCursor(c);
    } else if (m_part == DocPosition::Source)
        return KTextEdit::keyPressEvent(keyEvent);

    // BEGIN GENERAL
    //  ALT+123 feature TODO this is general so should be on another level
    else if ((keyEvent->modifiers() & Qt::AltModifier) && !keyEvent->text().isEmpty() && keyEvent->text().at(0).isDigit()) {
        QString text = keyEvent->text();
        while (!text.isEmpty() && text.at(0).isDigit()) {
            m_currentUnicodeNumber = 10 * m_currentUnicodeNumber + (text.at(0).digitValue());
            text.remove(0, 1);
        }
        KTextEdit::keyPressEvent(keyEvent);
    }
    // END GENERAL

    else if (!keyEvent->modifiers() && (keyEvent->key() == Qt::Key_Backspace || keyEvent->key() == Qt::Key_Delete)) {
        // only for cases when:
        //-BkSpace was hit and cursor was atStart
        //-Del was hit and cursor was atEnd
        if (Q_UNLIKELY(!m_catalog->isApproved(m_currentPos.entry) && !textCursor().hasSelection())
            && ((textCursor().atStart() && keyEvent->key() == Qt::Key_Backspace) || (textCursor().atEnd() && keyEvent->key() == Qt::Key_Delete)))
            requestToggleApprovement();
        else
            KTextEdit::keyPressEvent(keyEvent);
    } else if (keyEvent->key() == Qt::Key_Space && (keyEvent->modifiers() & Qt::AltModifier))
        insertPlainTextWithCursorCheck(QChar(0x00a0U));
    else if (keyEvent->key() == Qt::Key_Minus && (keyEvent->modifiers() & Qt::AltModifier))
        insertPlainTextWithCursorCheck(QChar(0x0000AD));
    // BEGIN clever editing
    else if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
        if (m_completionBox && m_completionBox->isVisible()) {
            if (m_completionBox->currentItem())
                completionActivated(m_completionBox->currentItem()->text());
            else
                qCWarning(LOKALIZE_LOG) << "avoided a crash. a case for bug 238835!";
            m_completionBox->hide();
            return;
        }
        if (m_catalog->type() != Gettext)
            return KTextEdit::keyPressEvent(keyEvent);

        QString str = toPlainText();
        QTextCursor t = textCursor();
        int pos = t.position();
        QString ins;
        if (keyEvent->modifiers() & Qt::ShiftModifier) {
            if (pos > 0 && !str.isEmpty() && str.at(pos - 1) == QLatin1Char('\\') && !isMasked(str, pos - 1)) {
                ins = QLatin1Char('n');
            } else {
                ins = QStringLiteral("\\n");
            }
        } else if (!(keyEvent->modifiers() & Qt::ControlModifier)) {
            if (m_langUsesSpaces && pos > 0 && !str.isEmpty() && !str.at(pos - 1).isSpace()) {
                if (str.at(pos - 1) == QLatin1Char('\\') && !isMasked(str, pos - 1))
                    ins = QLatin1Char('\\');
                // if there is no new line at the end
                if (pos < 2 || QStringView(str).mid(pos - 2, 2) != QLatin1String("\\n"))
                    ins += QLatin1Char(' ');
            } else if (str.isEmpty()) {
                ins = QStringLiteral("\\n");
            }
        }
        if (!str.isEmpty()) {
            ins += QLatin1Char('\n');
            insertPlainTextWithCursorCheck(ins);
        } else
            KTextEdit::keyPressEvent(keyEvent);
    } else if (m_catalog->type() != Gettext)
        KTextEdit::keyPressEvent(keyEvent);
    else if ((keyEvent->modifiers() & Qt::ControlModifier) ? (keyEvent->key() == Qt::Key_D) : (keyEvent->key() == Qt::Key_Delete) && textCursor().atEnd()) {
        qCWarning(LOKALIZE_LOG) << "workaround for Qt/X11 bug";
        QTextCursor t = textCursor();
        if (!t.hasSelection()) {
            int pos = t.position();
            QString str = toPlainText();
            // workaround for Qt/X11 bug: if Del on NumPad is pressed, then pos is beyond end
            if (pos == str.size())
                --pos;
            if (!str.isEmpty() && str.at(pos) == QLatin1Char('\\') && !isMasked(str, pos) && pos < str.length() - 1 && spclChars.contains(str.at(pos + 1))) {
                t.deleteChar();
            }
        }

        t.deleteChar();
        setTextCursor(t);
    } else if ((!keyEvent->modifiers() && keyEvent->key() == Qt::Key_Backspace)
               || ((keyEvent->modifiers() & Qt::ControlModifier) && keyEvent->key() == Qt::Key_H)) {
        QTextCursor t = textCursor();
        if (!t.hasSelection()) {
            int pos = t.position();
            QString str = toPlainText();
            if (!str.isEmpty() && pos > 0 && spclChars.contains(str.at(pos - 1))) {
                if (pos > 1 && str.at(pos - 2) == QLatin1Char('\\') && !isMasked(str, pos - 2)) {
                    t.deletePreviousChar();
                    t.deletePreviousChar();
                    setTextCursor(t);
                }
            }
        }
        KTextEdit::keyPressEvent(keyEvent);
    } else if (keyEvent->key() == Qt::Key_Tab)
        insertPlainTextWithCursorCheck(QStringLiteral("\\t"));
    else
        KTextEdit::keyPressEvent(keyEvent);
    // END clever editing
}

void TranslationUnitTextEdit::keyReleaseEvent(QKeyEvent *e)
{
    if ((e->key() == Qt::Key_Alt) && m_currentUnicodeNumber >= 32) {
        insertPlainTextWithCursorCheck(QChar(m_currentUnicodeNumber));
        m_currentUnicodeNumber = 0;
    } else
        KTextEdit::keyReleaseEvent(e);
}

void TranslationUnitTextEdit::insertPlainTextWithCursorCheck(const QString &text)
{
    insertPlainText(text);
    KTextEdit::ensureCursorVisible();
}

QString TranslationUnitTextEdit::toPlainText()
{
    QTextCursor cursor = textCursor();
    cursor.select(QTextCursor::Document);
    QString text = cursor.selectedText();
    text.replace(QChar(8233), QLatin1Char('\n'));
    return text;
}

void TranslationUnitTextEdit::emitCursorPositionChanged()
{
    Q_EMIT cursorPositionChanged(textCursor().columnNumber());
}

void TranslationUnitTextEdit::insertTag(InlineTag tag)
{
    QTextCursor cursor = textCursor();
    tag.start = qMin(cursor.anchor(), cursor.position());
    tag.end = qMax(cursor.anchor(), cursor.position()) + tag.isPaired();
    qCDebug(LOKALIZE_LOG) << "insert tag" << (m_part == DocPosition::Source) << tag.start << tag.end;
    m_catalog->push(new InsTagCmd(m_catalog, currentPos(), tag));
    showPos(currentPos(), CatalogString(), true);
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor, tag.end + 1 + tag.isPaired());
    setFocus();
}

int TranslationUnitTextEdit::strForMicePosIfUnderTag(QPoint mice, CatalogString &str, bool tryHarder)
{
    if (m_currentPos.entry == -1)
        return -1;
    QTextCursor cursor = cursorForPosition(mice);
    int pos = cursor.position();
    str = m_catalog->catalogString(m_currentPos);
    if (pos == -1 || pos >= str.string.size())
        return -1;

    if (str.string.at(pos) != TAGRANGE_IMAGE_SYMBOL) {
        bool cont = tryHarder && --pos >= 0 && str.string.at(pos) == TAGRANGE_IMAGE_SYMBOL;
        if (!cont)
            return -1;
    }

    int result = str.tags.size();
    while (--result >= 0 && str.tags.at(result).start != pos && str.tags.at(result).end != pos)
        ;
    return result;
}

void TranslationUnitTextEdit::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        CatalogString str;
        int pos = strForMicePosIfUnderTag(event->pos(), str);
        if (pos != -1 && m_part == DocPosition::Source) {
            Q_EMIT tagInsertRequested(str.tags.at(pos));
            event->accept();
            return;
        }
    }
    KTextEdit::mouseReleaseEvent(event);
}

void TranslationUnitTextEdit::contextMenuEvent(QContextMenuEvent *event)
{
    CatalogString str;
    int pos = strForMicePosIfUnderTag(event->pos(), str);
    if (pos != -1) {
        QString xid = str.tags.at(pos).xid;

        if (!xid.isEmpty()) {
            QMenu menu;
            int entry = m_catalog->unitById(xid);

            QAction *result = menu.exec(event->globalPos());
            if (result) {
                if (entry >= m_catalog->numberOfEntries())
                    Q_EMIT binaryUnitSelectRequested(xid);
                else
                    Q_EMIT gotoEntryRequested(DocPosition(entry));
                event->accept();
            }
            return;
        }
    }
    if (textCursor().hasSelection() && m_part == DocPosition::Target) {
        QMenu menu;
        menu.addAction(i18nc("@action:inmenu", "Lookup selected text in translation memory"));
        if (menu.exec(event->globalPos()))
            Q_EMIT tmLookupRequested(m_part, textCursor().selectedText());
        return;
    }

    if (m_part != DocPosition::Source && m_part != DocPosition::Target)
        return;

    KTextEdit::contextMenuEvent(event);
}
void TranslationUnitTextEdit::zoomRequestedSlot(qreal fontSize)
{
    QFont curFont = font();
    curFont.setPointSizeF(fontSize);
    setFont(curFont);
}

void TranslationUnitTextEdit::wheelEvent(QWheelEvent *event)
{
    // Override default KTextEdit behavior which ignores Ctrl+wheelEvent when the field is not ReadOnly (i/o zooming)
    if (m_part == DocPosition::Target && !Settings::mouseWheelGo() && (event->modifiers() == Qt::ControlModifier)) {
        float delta = event->angleDelta().y() / 120.f;
        zoomInF(delta);
        // Also zoom in the source
        Q_EMIT zoomRequested(font().pointSizeF());
        return;
    }

    if (m_part == DocPosition::Source || !Settings::mouseWheelGo()) {
        if (event->modifiers() == Qt::ControlModifier) {
            float delta = event->angleDelta().y() / 120.f;
            zoomInF(delta);
            // Also zoom in the target
            Q_EMIT zoomRequested(font().pointSizeF());
            return;
        }
        return KTextEdit::wheelEvent(event);
    }

    switch (event->modifiers()) {
    case Qt::ControlModifier:
        if (event->angleDelta().y() > 0)
            Q_EMIT gotoPrevFuzzyRequested();
        else
            Q_EMIT gotoNextFuzzyRequested();
        break;
    case Qt::AltModifier:
        if (event->angleDelta().y() > 0)
            Q_EMIT gotoPrevUntranslatedRequested();
        else
            Q_EMIT gotoNextUntranslatedRequested();
        break;
    case Qt::ControlModifier | Qt::ShiftModifier:
        if (event->angleDelta().y() > 0)
            Q_EMIT gotoPrevFuzzyUntrRequested();
        else
            Q_EMIT gotoNextFuzzyUntrRequested();
        break;
    case Qt::ShiftModifier:
        return KTextEdit::wheelEvent(event);
    default:
        if (event->angleDelta().y() > 0)
            Q_EMIT gotoPrevRequested();
        else
            Q_EMIT gotoNextRequested();
    }
}

void TranslationUnitTextEdit::spellReplace()
{
    QTextCursor wordSelectCursor = textCursor();
    wordSelectCursor.select(QTextCursor::WordUnderCursor);
    if (!m_highlighter->isWordMisspelled(wordSelectCursor.selectedText()))
        return;

    const QStringList &suggestions = m_highlighter->suggestionsForWord(wordSelectCursor.selectedText());
    if (suggestions.isEmpty())
        return;

    m_catalog->beginMacro(i18nc("@item Undo action item", "Replace text"));
    wordSelectCursor.insertText(suggestions.first());
    m_catalog->endMacro();
}

bool TranslationUnitTextEdit::event(QEvent *event)
{
#ifdef Q_OS_MAC
    if (event->type() == QEvent::InputMethod) {
        QInputMethodEvent *e = static_cast<QInputMethodEvent *>(event);
        insertPlainTextWithCursorCheck(e->commitString());
        e->accept();
        return true;
    }
#endif
    if (event->type() == QEvent::ToolTip) {
        QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
        CatalogString str;
        int pos = strForMicePosIfUnderTag(helpEvent->pos(), str, true);
        if (pos != -1) {
            QString tooltip = str.tags.at(pos).displayName();
            QToolTip::showText(helpEvent->globalPos(), tooltip);
            return true;
        }

        QString tip;

        QString langCode = m_highlighter->currentLanguage();

        bool nospell = langCode.isEmpty();
        if (nospell)
            langCode = m_part == DocPosition::Source ? m_catalog->sourceLangCode() : m_catalog->targetLangCode();
        QLocale l(langCode);
        if (l.language() != QLocale::C)
            tip = l.nativeLanguageName() + QLatin1String(" (");
        tip += langCode;
        if (l.language() != QLocale::C)
            tip += QLatin1Char(')');
        if (nospell)
            tip += QLatin1String(" - ") + i18n("no spellcheck available");
        QToolTip::showText(helpEvent->globalPos(), tip);
    }
    return KTextEdit::event(event);
}

void TranslationUnitTextEdit::slotLanguageToolFinished(const QString &result)
{
    LanguageToolParser parser;
    const QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
    const QJsonObject fields = doc.object();
    Q_EMIT languageToolChanged(parser.parseResult(fields, toPlainText()));
}

void TranslationUnitTextEdit::slotLanguageToolError(const QString &str)
{
    Q_EMIT languageToolChanged(i18n("An error was reported: %1", str));
}

void TranslationUnitTextEdit::launchLanguageTool()
{
    if (toPlainText().isEmpty())
        return;

    LanguageToolResultJob *job = new LanguageToolResultJob(this);
    job->setUrl(LanguageToolManager::self()->languageToolCheckPath());
    job->setNetworkAccessManager(LanguageToolManager::self()->networkAccessManager());
    job->setText(toPlainText().toHtmlEscaped().replace(QStringLiteral("%"), QStringLiteral("%25")));
    job->setLanguage(m_catalog->targetLangCode());
    connect(job, &LanguageToolResultJob::finished, this, &TranslationUnitTextEdit::slotLanguageToolFinished);
    connect(job, &LanguageToolResultJob::error, this, &TranslationUnitTextEdit::slotLanguageToolError);
    job->start();
}
void TranslationUnitTextEdit::tagMenu()
{
    doTag(false);
}
void TranslationUnitTextEdit::tagImmediate()
{
    doTag(true);
}

void TranslationUnitTextEdit::doTag(bool immediate)
{
    QMenu menu;
    QAction *txt = nullptr;

    CatalogString sourceWithTags = m_catalog->sourceWithTags(m_currentPos);
    int count = sourceWithTags.tags.size();
    if (count) {
        QMap<QString, int> tagIdToIndex = m_catalog->targetWithTags(m_currentPos).tagIdToIndex();
        bool hasActive = false;
        for (int i = 0; i < count; ++i) {
            txt = menu.addAction(QString::number(i));
            txt->setData(QVariant(i));
            if (!hasActive && !tagIdToIndex.contains(sourceWithTags.tags.at(i).id)) {
                if (immediate) {
                    insertTag(sourceWithTags.tags.at(txt->data().toInt()));
                    return;
                }
                hasActive = true;
                menu.setActiveAction(txt);
            }
        }
        if (immediate)
            return;
        txt = menu.exec(mapToGlobal(cursorRect().bottomRight()));
        if (!txt)
            return;
        insertTag(sourceWithTags.tags.at(txt->data().toInt()));
    } else {
        if (Q_UNLIKELY(Project::instance()->markup().isEmpty()))
            return;

        const QRegularExpression tag(Project::instance()->markup(), QRegularExpression::InvertedGreedinessOption);
        QString en = m_catalog->sourceWithTags(m_currentPos).string;
        QString target(toPlainText());
        en.remove(QLatin1Char('\n'));
        target.remove(QLatin1Char('\n'));
        int pos = 0;
        int posInMsgStr = 0;
        while (true) {
            const auto match = tag.match(en, pos);
            if (!match.hasMatch()) {
                break;
            }
            pos = match.capturedStart();
            txt = menu.addAction(match.captured(0));
            pos += match.capturedLength();

            if (posInMsgStr != -1 && (posInMsgStr = target.indexOf(match.captured(0), posInMsgStr)) == -1) {
                if (immediate) {
                    insertPlainTextWithCursorCheck(txt->text());
                    return;
                }
                menu.setActiveAction(txt);
            } else if (posInMsgStr != -1)
                posInMsgStr += match.capturedLength();
        }
        if (!txt || immediate)
            return;

        txt = menu.exec(mapToGlobal(cursorRect().bottomRight()));
        if (txt)
            insertPlainTextWithCursorCheck(txt->text());
    }
}

void TranslationUnitTextEdit::skipTags()
{
    QTextCursor cursor = textCursor();
    QTextDocument *doc = cursor.document();
    bool posAfterTag = false;

    QChar tagStart = QLatin1Char('<');
    QChar tagEnd = QLatin1Char('>');

    const int currentPos = cursor.position();
    const int docLength = doc->characterCount();

    for (int pos = currentPos; pos < docLength; ++pos) {
        QChar ch = doc->characterAt(pos);
        if (ch == tagEnd) {
            posAfterTag = true;
        } else if (ch == tagStart) {
            posAfterTag = false;
        } else if (posAfterTag && !ch.isSpace()) {
            cursor.setPosition(pos);
            setTextCursor(cursor);
            break;
        }
    }
}

void TranslationUnitTextEdit::source2target()
{
    CatalogString sourceWithTags = m_catalog->sourceWithTags(m_currentPos);
    QString text = sourceWithTags.string;
    QString out;
    QString ctxt = m_catalog->context(DocPosition(m_currentPos.entry)).first();
    const QRegularExpression delimiter(QStringLiteral("\\s*,\\s*"));

    // TODO ask for the fillment if the first time.
    // BEGIN KDE specific part
    if (ctxt.startsWith(QLatin1String("NAME OF TRANSLATORS")) || text.startsWith(QLatin1String("_: NAME OF TRANSLATORS\\n"))) {
        if (!document()->toPlainText().split(delimiter).contains(Settings::authorLocalizedName())) {
            if (!document()->isEmpty())
                out = QLatin1String(", ");
            out += Settings::authorLocalizedName();
        }
    } else if (ctxt.startsWith(QLatin1String("EMAIL OF TRANSLATORS")) || text.startsWith(QLatin1String("_: EMAIL OF TRANSLATORS\\n"))) {
        if (!document()->toPlainText().split(delimiter).contains(Settings::authorEmail())) {
            if (!document()->isEmpty())
                out = QLatin1String(", ");
            out += Settings::authorEmail();
        }
    } else if (text.startsWith(QLatin1String("ROLES_OF_TRANSLATORS"))) {
        if (!document()->isEmpty())
            out = QLatin1Char('\n');
        out += QLatin1String(
                   "<othercredit role=\\\"translator\\\">\n"
                   "<firstname></firstname><surname></surname>\n"
                   "<affiliation><address><email>")
            + Settings::authorEmail()
            + QLatin1String(
                   "</email></address>\n"
                   "</affiliation><contrib></contrib></othercredit>");
    } else if (text.startsWith(QLatin1String("CREDIT_FOR_TRANSLATORS"))) {
        if (!document()->isEmpty())
            out = QLatin1Char('\n');
        out += QLatin1String("<para>") + Settings::authorLocalizedName() + QLatin1Char('\n') + QLatin1String("<email>") + Settings::authorEmail()
            + QLatin1String("</email></para>");
    }
    // END KDE specific part

    else {
        m_catalog->beginMacro(i18nc("@item Undo action item", "Copy source to target"));
        removeTargetSubstring(0, -1, /*refresh*/ false);
        insertCatalogString(sourceWithTags, 0, /*refresh*/ false);
        m_catalog->endMacro();

        showPos(m_currentPos, sourceWithTags, /*keepCursor*/ false);

        requestToggleApprovement();
    }
    if (!out.isEmpty()) {
        QTextCursor t = textCursor();
        t.movePosition(QTextCursor::End);
        t.insertText(out);
        setTextCursor(t);
    }
}

void TranslationUnitTextEdit::requestToggleApprovement()
{
    if (m_catalog->isApproved(m_currentPos.entry) || !Settings::autoApprove())
        return;

    bool skip = m_catalog->isPlural(m_currentPos);
    if (skip) {
        skip = false;
        DocPos pos(m_currentPos);
        for (pos.form = 0; pos.form < m_catalog->numberOfPluralForms(); ++(pos.form))
            skip = skip || !m_catalog->isModified(pos);
    }
    if (!skip)
        Q_EMIT toggleApprovementRequested();
}

void TranslationUnitTextEdit::cursorToStart()
{
    QTextCursor t = textCursor();
    t.movePosition(QTextCursor::Start);
    setTextCursor(t);
}

void TranslationUnitTextEdit::doCompletion(int pos)
{
    QString target = m_catalog->targetWithTags(m_currentPos).string;
    auto sp = target.lastIndexOf(CompletionStorage::instance()->rxSplit, pos - 1);
    int len = (pos - sp) - 1;

    QStringList s = CompletionStorage::instance()->makeCompletion(QString::fromRawData(target.unicode() + sp + 1, len));

    if (!m_completionBox) {
        // BEGIN creation
        m_completionBox = new MyCompletionBox(this);
        connect(m_completionBox, &MyCompletionBox::textActivated, this, &TranslationUnitTextEdit::completionActivated);
        m_completionBox->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        // END creation
    }
    m_completionBox->setItems(s);
    if (s.size() && !s.first().isEmpty()) {
        m_completionBox->setCurrentRow(0);
        if (!m_completionBox->isVisible()) // NOTE remove the check if kdelibs gets adapted
            m_completionBox->show();
        m_completionBox->resize(m_completionBox->sizeHint());
        QPoint p = cursorRect().bottomRight();
        if (p.x() < 10) // workaround Qt bug
            p.rx() += textCursor().verticalMovementX() + QFontMetrics(currentFont()).horizontalAdvance(QLatin1Char('W'));
        m_completionBox->move(viewport()->mapToGlobal(p));
    } else
        m_completionBox->hide();
}

void TranslationUnitTextEdit::doExplicitCompletion()
{
    doCompletion(textCursor().anchor());
}

void TranslationUnitTextEdit::completionActivated(const QString &semiWord)
{
    QTextCursor cursor = textCursor();
    cursor.insertText(semiWord);
    setTextCursor(cursor);
}

#include "moc_xlifftextedit.cpp"
