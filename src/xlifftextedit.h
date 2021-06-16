/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2014 by Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of
  the License or (at your option) version 3 or any later version
  accepted by the membership of KDE e.V. (or its successor approved
  by the membership of KDE e.V.), which shall act as a proxy
  defined in Section 14 of version 3 of the license.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

**************************************************************************** */


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
    int m_currentUnicodeNumber; //alt+NUM thing
    bool m_langUsesSpaces; //e.g. Chinese doesn't

    Catalog* m_catalog;
    DocPosition::Part m_part;
    DocPosition m_currentPos;
    SyntaxHighlighter* m_highlighter;
    bool m_enabled;

    MyCompletionBox* m_completionBox;

    //for undo/redo tracking
    QString _oldMsgstr;
    QString _oldMsgstrAscii; //HACK to workaround #218246

    //For text move with mouse
    int m_cursorSelectionStart;
    int m_cursorSelectionEnd;

    //For LanguageTool timer
    QTimer* m_languageToolTimer;
};


void insertContent(QTextCursor& cursor, const CatalogString& catStr, const CatalogString& refStr = CatalogString(), bool insertText = true);


#endif
