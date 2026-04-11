/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2009 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef GLOSSARYVIEW_H
#define GLOSSARYVIEW_H

#include "pos.h"

#include <QDockWidget>
#include <QRegularExpression>
#include <QVector>

class Catalog;
class FlowLayout;
class QAction;
class QDragEnterEvent;
class QDropEvent;
class QEvent;
class QFrame;
class QScrollArea;
class QByteArray;

namespace GlossaryNS
{
class Glossary;

#define GLOSSARY_SHORTCUTS 11
class GlossaryView : public QDockWidget
{
    Q_OBJECT

public:
    explicit GlossaryView(QWidget *, Catalog *, const QVector<QAction *> &);
    ~GlossaryView();

public Q_SLOTS:
    // plural messages usually contain the same words...
    void slotNewEntryDisplayed();
    void slotNewEntryDisplayed(DocPosition pos); // HACK: a little hacky, but... :)
    void slotSelectGlossaryEntryRequested(const QByteArray &entryId);

Q_SIGNALS:
    void termInsertRequested(const QString &);
    void signalSelectGlossaryEntryRequested(const QByteArray &entryId);

private:
    void clear();

private:
    QScrollArea *const m_browser;
    Catalog *const m_catalog;
    FlowLayout *const m_flowLayout;
    Glossary *const m_glossary;
    const QRegularExpression m_rxClean;
    const QRegularExpression m_rxSplit{QStringLiteral("\\W|\\d"), QRegularExpression::UseUnicodePropertiesOption};
    int m_currentIndex{-1};

    QString m_normTitle;
    QString m_hasInfoTitle;
    bool m_hasInfo{};
};
}
#endif
