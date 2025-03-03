/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2025 Finley Watson <fin-w@tutanota.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "lokalizetabpagebase.h"
#include "projectbase.h"

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

void LokalizeTabPageBase::reflectNonApprovedCount(int count, int total)
{
    QString text = i18nc("@info:status message entries\n'fuzzy' in gettext terminology", "Not ready: %1", count);
    if (count && total)
        text += i18nc("percentages in statusbar", " (%1%)", int(100.0 * count / total));
    statusBarItems.insert(ID_STATUS_FUZZY, text);
}

void LokalizeTabPageBase::reflectUntranslatedCount(int count, int total)
{
    QString text = i18nc("@info:status message entries", "Untranslated: %1", count);
    if (count && total)
        text += i18nc("percentages in statusbar", " (%1%)", int(100.0 * count / total));
    statusBarItems.insert(ID_STATUS_UNTRANS, text);
}

KXMLGUIClient *LokalizeTabPageBase::guiClient()
{
    return (KXMLGUIClient *)this;
}
