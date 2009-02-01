/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2009 by Nick Shaforostoff <shafff@ukr.net>

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


#ifndef XLIFFTEXTEDITOR_H
#define XLIFFTEXTEDITOR_H

#include "pos.h"
#include "catalogstring.h"

#include <KTextEdit>
class SyntaxHighlighter;//TODO rename


class XliffTextEdit: public KTextEdit
{
    Q_OBJECT
public:
    XliffTextEdit(Catalog* catalog, DocPosition::Part part, QWidget* parent=0);
    //NOTE remove this when Qt is fixed (hack for unbreakable spaces bug #162016)
    QString toPlainText();

    ///@returns targetWithTags for the sake of not calling XliffStorage/doContent twice
    CatalogString showPos(DocPosition pos, const CatalogString& refStr=CatalogString(), bool keepCursor=true);
    DocPosition currentPos()const {return m_currentPos;}

public slots:
    void reflectApprovementState();
    void reflectUntranslatedState();
    void toggleApprovement(bool approved);

    bool removeTargetSubstring(int start=0, int end=-1, bool refresh=true);
    void insertCatalogString(const CatalogString& catStr, int start=0, bool refresh=true);

    void source2target();
    void tagMenu();


    void emitCursorPositionChanged();//for leds

protected:
    void keyPressEvent(QKeyEvent *keyEvent);
    void keyReleaseEvent(QKeyEvent* e);
    QMimeData* createMimeDataFromSelection() const;
    void insertFromMimeData(const QMimeData* source);

private:
    ///@a refStr is for proper numbering
    void setContent(const CatalogString& catStr, const CatalogString& refStr=CatalogString());


private slots:
    //for Undo/Redo tracking
    void contentsChanged(int position,int charsRemoved,int charsAdded);

signals:
    void signalSetApproved(bool approved=true);
    void undoRequested();
    void redoRequested();
    void gotoFirstRequested();
    void gotoLastRequested();
    void gotoPrevRequested();
    void gotoNextRequested();

    void contentsModified(const DocPosition&);
    void approvedEntryDisplayed();
    void nonApprovedEntryDisplayed();
    void translatedEntryDisplayed();
    void untranslatedEntryDisplayed();
    void cursorPositionChanged(int column);

private:
    int m_currentUnicodeNumber; //alt+NUM thing

    Catalog* m_catalog;
    DocPosition::Part m_part;
    DocPosition m_currentPos;
    SyntaxHighlighter* m_highlighter;


    //for undo/redo tracking
    QString _oldMsgstr;

};


#endif
