/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2009 by Nick Shaforostoff <shafff@ukr.net>

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

#include <KTextEdit>
class QMouseEvent;
class SyntaxHighlighter;//TODO rename
class KCompletionBox;
class MyCompletionBox;

class TranslationUnitTextEdit: public KTextEdit
{
    Q_OBJECT
public:
    TranslationUnitTextEdit(Catalog* catalog, DocPosition::Part part, QWidget* parent=0);
    //NOTE remove this when Qt is fixed (hack for unbreakable spaces bug #162016)
    QString toPlainText();

    ///@returns targetWithTags for the sake of not calling XliffStorage/doContent twice
    CatalogString showPos(DocPosition pos, const CatalogString& refStr=CatalogString(), bool keepCursor=true);
    DocPosition currentPos()const {return m_currentPos;}

    void cursorToStart();

public slots:
    void reflectApprovementState();
    void reflectUntranslatedState();

    bool removeTargetSubstring(int start=0, int end=-1, bool refresh=true);
    void insertCatalogString(CatalogString catStr, int start=0, bool refresh=true);

    void source2target();
    void tagMenu();
    void tagImmediate();
    void insertTag(InlineTag tag);
    void spellReplace();

    void emitCursorPositionChanged();//for leds

    void doExplicitCompletion();

protected:
    void keyPressEvent(QKeyEvent *keyEvent);
    void keyReleaseEvent(QKeyEvent* e);
    QMimeData* createMimeDataFromSelection() const;
    void insertFromMimeData(const QMimeData* source);
    void mouseReleaseEvent(QMouseEvent* event);

    void contextMenuEvent(QContextMenuEvent *event);
    void wheelEvent(QWheelEvent *event);
    bool event(QEvent *event);

private:
    ///@a refStr is for proper numbering
    void setContent(const CatalogString& catStr, const CatalogString& refStr=CatalogString());

    int strForMicePosIfUnderTag(QPoint mice, CatalogString& str, bool tryHarder=false);

    void requestToggleApprovement();

    void doTag(bool immediate);

    void doCompletion(int pos);

private slots:
    //for Undo/Redo tracking
    void contentsChanged(int position,int charsRemoved,int charsAdded);
    void completionActivated(const QString&);
    void fileLoaded();

signals:
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


    void tagInsertRequested(const InlineTag& tag);

    void binaryUnitSelectRequested(const QString&);
    void tmLookupRequested(DocPosition::Part,const QString&);

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

    MyCompletionBox* m_completionBox;

    //for undo/redo tracking
    QString _oldMsgstr;
    QString _oldMsgstrAscii; //HACK to workaround #218246
};


void insertContent(QTextCursor& cursor, const CatalogString& catStr, const CatalogString& refStr=CatalogString(), bool insertText=true);


#endif
