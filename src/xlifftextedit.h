/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>
  SPDX-FileCopyrightText: 2024 Karl Ove Hufthammer <karl@huftis.org>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/


#ifndef XLIFFTEXTEDITOR_H
#define XLIFFTEXTEDITOR_H

#include "pos.h"
#include "catalogstring.h"

#include <ktextedit.h>
class QMouseEvent;
class SyntaxHighlighter;//TODO rename
class KCompletionBox;
class MyCompletionBox;

class TranslationUnitTextEdit: public KTextEdit
{
    Q_OBJECT
public:
    explicit TranslationUnitTextEdit(Catalog* catalog, DocPosition::Part part, QWidget* parent = nullptr);
    ~TranslationUnitTextEdit() override;
    //NOTE remove this when Qt is fixed (hack for unbreakable spaces bug #162016)
    QString toPlainText();

    ///@returns targetWithTags for the sake of not calling XliffStorage/doContent twice
    CatalogString showPos(DocPosition pos, const CatalogString& refStr = CatalogString(), bool keepCursor = true);
    DocPosition currentPos()const
    {
        return m_currentPos;
    }

    void cursorToStart();

    bool isSpellCheckingEnabled() const
    {
        return m_enabled;
    }
    void setSpellCheckingEnabled(bool enable);
    void setVisualizeSeparators(bool enable);
    bool shouldBlockBeSpellChecked(const QString &block) const override
    {
        Q_UNUSED(block);
        return true;
    }
    void insertPlainTextWithCursorCheck(const QString &text);

public Q_SLOTS:
    void reflectApprovementState();
    void reflectUntranslatedState();

    bool removeTargetSubstring(int start = 0, int end = -1, bool refresh = true);
    void insertCatalogString(CatalogString catStr, int start = 0, bool refresh = true);

    void source2target();
    void tagMenu();
    void tagImmediate();
    void insertTag(InlineTag tag);
    void skipTags();
    void spellReplace();
    void launchLanguageTool();

    void emitCursorPositionChanged();//for leds

    void doExplicitCompletion();
    void zoomRequestedSlot(qreal fontSize);

protected:
    void keyPressEvent(QKeyEvent *keyEvent) override;
    void keyReleaseEvent(QKeyEvent* e) override;
    QMimeData* createMimeDataFromSelection() const override;
    void insertFromMimeData(const QMimeData* source) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void dropEvent(QDropEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;

    void contextMenuEvent(QContextMenuEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    bool event(QEvent *event) override;

private:
    ///@a refStr is for proper numbering
    void setContent(const CatalogString& catStr, const CatalogString& refStr = CatalogString());

    int strForMicePosIfUnderTag(QPoint mice, CatalogString& str, bool tryHarder = false);

    void requestToggleApprovement();

    void doTag(bool immediate);

    void doCompletion(int pos);

private Q_SLOTS:
    //for Undo/Redo tracking
    void contentsChanged(int position, int charsRemoved, int charsAdded);
    void completionActivated(const QString&);
    void fileLoaded();

    void slotLanguageToolFinished(const QString &result);
    void slotLanguageToolError(const QString &str);

Q_SIGNALS:
    void toggleApprovementRequested();
    void undoRequested();
    void redoRequested();
    void findRequested();
    void findNextRequested();
    void replaceRequested();
    void gotoFirstRequested();
    void gotoLastRequested();
    void gotoPrevRequested();
    void gotoNextRequested();
    void gotoPrevFuzzyRequested();
    void gotoNextFuzzyRequested();
    void gotoPrevUntranslatedRequested();
    void gotoNextUntranslatedRequested();
    void gotoPrevFuzzyUntrRequested();
    void gotoNextFuzzyUntrRequested();
    void gotoEntryRequested(const DocPosition&);
    void zoomRequested(qreal);


    void tagInsertRequested(const InlineTag& tag);

    void binaryUnitSelectRequested(const QString&);
    void languageToolChanged(const QString&);
    void tmLookupRequested(DocPosition::Part, const QString&);

    void contentsModified(const DocPosition&);
    void approvedEntryDisplayed();
    void nonApprovedEntryDisplayed();
    void translatedEntryDisplayed();
    void untranslatedEntryDisplayed();
    void cursorPositionChanged(int column);

private:
    int m_currentUnicodeNumber{}; //alt+NUM thing
    bool m_langUsesSpaces{true}; //e.g. Chinese doesn't

    Catalog* m_catalog{};
    DocPosition::Part m_part{};
    DocPosition m_currentPos{};
    SyntaxHighlighter* m_highlighter{};
    bool m_enabled{};

    MyCompletionBox* m_completionBox{};

    //for undo/redo tracking
    QString _oldMsgstr;
    QString _oldMsgstrAscii; //HACK to workaround #218246

    //For text move with mouse
    int m_cursorSelectionStart{};
    int m_cursorSelectionEnd{};

    //For LanguageTool timer
    QTimer* m_languageToolTimer{};
};


void insertContent(QTextCursor& cursor, const CatalogString& catStr, const CatalogString& refStr = CatalogString(), bool insertText = true);


#endif
