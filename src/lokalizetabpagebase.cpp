/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2025 Finley Watson <fin-w@tutanota.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "lokalizetabpagebase.h"

void LokalizeTabPageBaseNoQMainWindow::setUpdatedXMLFile()
{
    QString localXml = guiClient()->localXMLFile();
    if (QFile::exists(localXml)) {
        lastXMLUpdate = QFileInfo(localXml).lastModified();
    }
}

void LokalizeTabPageBaseNoQMainWindow::reloadUpdatedXML()
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

KXMLGUIClient *LokalizeTabPageBaseNoQMainWindow::guiClient()
{
    return (KXMLGUIClient *)this;
}

void LokalizeTabPageBase::setUpdatedXMLFile()
{
    QString localXml = guiClient()->localXMLFile();
    if (QFile::exists(localXml)) {
        lastXMLUpdate = QFileInfo(localXml).lastModified();
    }
}

void LokalizeTabPageBase::reloadUpdatedXML()
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

KXMLGUIClient *LokalizeTabPageBase::guiClient()
{
    return (KXMLGUIClient *)this;
}
