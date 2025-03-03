/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>
  SPDX-FileCopyrightText: 2025 Finley Watson <fin-w@tutanota.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef LOKALIZETABPAGEBASE_H
#define LOKALIZETABPAGEBASE_H

#include "actionproxy.h"

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QString>
#include <qtmetamacros.h>

#include <KMainWindow>
#include <KXMLGUIClient>

/**
 * Interface for LokalizeMainWindow
 */
class LokalizeSubwindowBase : public KMainWindow
{
    Q_OBJECT
public:
    explicit LokalizeSubwindowBase(QWidget *parent)
        : KMainWindow(parent)
    {
    }
    virtual ~LokalizeSubwindowBase()
    {
        Q_EMIT aboutToBeClosed();
    }
    virtual KXMLGUIClient *guiClient() = 0;
    virtual void reloadUpdatedXML() = 0;
    virtual void setUpdatedXMLFile() = 0;

    // interface for LokalizeMainWindow
    virtual void hideDocks() = 0;
    virtual void showDocks() = 0;

    virtual QString currentFilePath()
    {
        return QString();
    }

Q_SIGNALS:
    void aboutToBeClosed();

public:
    StatusBarProxy statusBarItems;

protected:
    QDateTime lastXMLUpdate;
};

/**
 * C++ casting workaround
 */
class LokalizeTabPageBase : public LokalizeSubwindowBase, public KXMLGUIClient
{
    Q_OBJECT
public:
    explicit LokalizeTabPageBase(QWidget *parent)
        : LokalizeSubwindowBase(parent)
        , KXMLGUIClient()
    {
    }

    ~LokalizeTabPageBase() override = default;

    KXMLGUIClient *guiClient() override;
    void setUpdatedXMLFile() override;
    void reloadUpdatedXML() override;
    void reflectNonApprovedCount(int count, int total);
    void reflectUntranslatedCount(int count, int total);

    QString m_tabLabel;
    QString m_tabToolTip;
    QIcon m_tabIcon;

Q_SIGNALS:
    void signalUpdatedTabLabelAndIconAvailable(LokalizeTabPageBase *);
};

#endif
