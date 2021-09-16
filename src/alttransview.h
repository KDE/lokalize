/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2008 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

*/

#ifndef ALTTRANSVIEW_H
#define ALTTRANSVIEW_H

#define ALTTRANS_SHORTCUTS 9

#include "pos.h"
#include "alttrans.h"
#include <QDockWidget>
namespace TM
{
class TextBrowser;
}
class Catalog;
class QAction;

class AltTransView: public QDockWidget
{
    Q_OBJECT

public:
    explicit AltTransView(QWidget*, Catalog*, const QVector<QAction*>&);
    ~AltTransView() override;


public Q_SLOTS:
    void slotNewEntryDisplayed(const DocPosition&);
    void fileLoaded();
    void attachAltTransFile(const QString&);
    void addAlternateTranslation(int entry, const QString&);

private Q_SLOTS:
    //void contextMenu(const QPoint & pos);
    void process();
    void initLater();
    void slotUseSuggestion(int);

Q_SIGNALS:
    void refreshRequested();
    void textInsertRequested(const QString&);

private:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent *event) override;
    bool event(QEvent *event) override;


private:
    TM::TextBrowser* m_browser;
    Catalog* m_catalog;
    QString m_normTitle;
    QString m_hasInfoTitle;
    bool m_hasInfo;
    bool m_everShown;
    DocPos m_entry;
    DocPos m_prevEntry;

    QVector<AltTrans> m_entries;
    QMap<int, int> m_entryPositions;
    QVector<QAction*> m_actions;//need them to get shortcuts
};

#endif
