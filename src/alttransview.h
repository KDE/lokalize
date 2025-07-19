/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2008 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>
  SPDX-FileCopyrightText: 2024 Finley Watson <fin-w@tutanota.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef ALTTRANSVIEW_H
#define ALTTRANSVIEW_H

#include "alttrans.h"
#include "pos.h"

#include <QDockWidget>

#define ALTTRANS_SHORTCUTS 9

class Catalog;
class QAction;

namespace TM
{
class DynamicItemHeightQListWidget;
}

class AltTransView : public QDockWidget
{
    Q_OBJECT

public:
    explicit AltTransView(QWidget *, Catalog *, const QVector<QAction *> &);
    ~AltTransView() override;
    /*
     * @short Triggered for each show / hide event.
     * Resizes the items in the view so that when the view
     * is first shown, its items are laid out correctly.
     */
    void showEvent(QShowEvent *event) override;
    /*
     * @short Open a dialogue when data is available.
     * When there is data in the Alternative Translation View
     * but the view is hidden, inform the user that displaying
     * the view might be useful, and allow the user to show it.
     * @author Finley Watson
     */
    void conditionallyPromptToDisplay();

public Q_SLOTS:
    void slotNewEntryDisplayed(const DocPosition &);
    void fileLoaded();
    void attachAltTransFile(const QString &);
    void addAlternateTranslation(int entry, const QString &);

private Q_SLOTS:
    void process();
    void initLater();
    void slotUseSuggestion(int);

Q_SIGNALS:
    void refreshRequested();
    void textInsertRequested(const QString &);

private:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    bool event(QEvent *event) override;

private:
    TM::DynamicItemHeightQListWidget *m_entriesList{nullptr};
    Catalog *m_catalog;
    QString m_normTitle;
    QString m_hasInfoTitle;
    bool m_hasInfo{};
    bool m_everCheckedAboutPromptingToShow{};
    DocPos m_entry;
    DocPos m_prevEntry;

    QVector<AltTrans> m_entries;
    QVector<QAction *> m_actions; // need them to get shortcuts
};

#endif
