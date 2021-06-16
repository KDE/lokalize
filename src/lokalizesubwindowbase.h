/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2014 by Nick Shaforostoff <shafff@ukr.net>
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
