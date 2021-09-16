/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

*/

#ifndef MSGCTXTVIEW_H
#define MSGCTXTVIEW_H

#include "pos.h"
#include "note.h"

#include <QMap>
#include <QDockWidget>
#include <KProcess>

class Catalog;
class NoteEditor;
class QTextBrowser;
class QStackedLayout;

class MsgCtxtView: public QDockWidget
{
    Q_OBJECT

public:
    explicit MsgCtxtView(QWidget*, Catalog*);
    ~MsgCtxtView();

    void gotoEntry(const DocPosition&, int selection = 0);
    void addNote(DocPosition, const QString& text);
    void addTemporaryEntryNote(int entry, const QString& text);
public Q_SLOTS:
    void removeErrorNotes();
    void cleanup();
    void languageTool(const QString& text);
    void addNoteUI();
private Q_SLOTS:
    void anchorClicked(const QUrl& link);
    void noteEditAccepted();
    void noteEditRejected();
    void process();
    void pology();
    void pologyReceivedStandardOutput();
    void pologyReceivedStandardError();
    void pologyHasFinished();

Q_SIGNALS:
    void srcFileOpenRequested(const QString& srcPath, int line);
    void escaped();

private:
    QTextBrowser* m_browser;
    NoteEditor* m_editor;
    QStackedLayout* m_stackedLayout;

    Catalog* m_catalog;
    QMap< DocPos, QPair<Note, int> > m_unfinishedNotes; //note and its index
    QMap< int, QString > m_tempNotes;
    QMap< int, QString > m_pologyNotes;
    QMap< int, QString > m_languageToolNotes;
    int  m_selection;
    int  m_offset;
    bool m_hasInfo;
    bool m_hasErrorNotes;
    DocPos m_entry;
    DocPos m_prevEntry;

    KProcess* m_pologyProcess;
    int m_pologyProcessInProgress;
    bool m_pologyStartedReceivingOutput;
    QString m_pologyData;

    static const QString BR;
};



#endif
