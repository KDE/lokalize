/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

*/

#ifndef LOKALIZESUBWINDOWBASE_H
#define LOKALIZESUBWINDOWBASE_H

#include <QHash>
#include <QString>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>

#include "actionproxy.h"

#include <kmainwindow.h>
#include <kxmlguiclient.h>


/**
 * Interface for LokalizeMainWindow
 */
class LokalizeSubwindowBase: public KMainWindow
{
    Q_OBJECT
public:
    explicit LokalizeSubwindowBase(QWidget* parent): KMainWindow(parent) {}
    virtual ~LokalizeSubwindowBase()
    {
        Q_EMIT aboutToBeClosed();
    }
    virtual KXMLGUIClient* guiClient() = 0;
    virtual void reloadUpdatedXML() = 0;
    virtual void setUpdatedXMLFile() = 0;

    //interface for LokalizeMainWindow
    virtual void hideDocks() = 0;
    virtual void showDocks() = 0;
    //bool queryClose();

    virtual QString currentFilePath()
    {
        return QString();
    }

protected:
    void reflectNonApprovedCount(int count, int total);
    void reflectUntranslatedCount(int count, int total);

Q_SIGNALS:
    void aboutToBeClosed();

public:
    //QHash<QString,ActionProxy*> supportedActions;
    StatusBarProxy statusBarItems;

protected:
    QDateTime lastXMLUpdate;
};

/**
 * C++ casting workaround
 */
class LokalizeSubwindowBase2: public LokalizeSubwindowBase, public KXMLGUIClient
{
public:
    explicit LokalizeSubwindowBase2(QWidget* parent): LokalizeSubwindowBase(parent), KXMLGUIClient() {}
    ~LokalizeSubwindowBase2() override = default;

    KXMLGUIClient* guiClient() override
    {
        return (KXMLGUIClient*)this;
    }

    void setUpdatedXMLFile() override
    {
        QString localXml = guiClient()->localXMLFile();
        if (QFile::exists(localXml)) {
            lastXMLUpdate = QFileInfo(localXml).lastModified();
        }
    }

    void reloadUpdatedXML() override
    {
        QString localXml = guiClient()->localXMLFile();
        if (QFile::exists(localXml)) {
            QDateTime newXMLUpdate = QFileInfo(localXml).lastModified();
            if (newXMLUpdate > lastXMLUpdate) {
                lastXMLUpdate = newXMLUpdate;
                guiClient()->reloadXML();
            }
        }
    }
};

#endif
