/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2009 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef MERGEVIEW_H
#define MERGEVIEW_H

#include "mergecatalog.h"
#include "pos.h"

#include <QDockWidget>
class QTextEdit;
class Catalog;
class MergeCatalog;
class QDragEnterEvent;
class QDropEvent;

class MergeView : public QDockWidget
{
    Q_OBJECT

public:
    explicit MergeView(QWidget *, Catalog *, bool primary);
    ~MergeView() override;

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *) override;
    QString filePath();
    bool isModified();

private:
    /**
     * checks if there are any other plural forms waiting to be synced for current pos
     * @returns number of form or -1
     */
    int pluralFormsAvailableForward();
    int pluralFormsAvailableBackward();

    bool event(QEvent *event) override;

public Q_SLOTS:
    void mergeOpen(QString mergeFilePath = QString());
    void cleanup();
    void slotNewEntryDisplayed(const DocPosition &);
    void slotUpdate(const DocPosition &);

    void gotoNextChanged(bool approvedOnly = false);
    void gotoNextChangedApproved();
    void gotoPrevChanged();
    void mergeAccept();
    void mergeAcceptAllForEmpty();
    void mergeBack();

Q_SIGNALS:
    void signalPriorChangedAvailable(bool);
    void signalNextChangedAvailable(bool);
    void signalEntryWithMergeDisplayed(bool);

    void gotoEntry(const DocPosition &, int);

    void mergeCatalogAvailable(bool);
    void mergeCatalogPointerChanged(MergeCatalog *mergeCatalog);

private:
    QTextEdit *m_browser{nullptr};
    Catalog *m_baseCatalog{nullptr};
    MergeCatalog *m_mergeCatalog{nullptr};
    DocPosition m_pos;
    QString m_normTitle;
    QString m_hasInfoTitle;
    bool m_hasInfo{false};
};

#endif
