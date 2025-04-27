/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>
  SPDX-FileCopyrightText: 2025 Finley Watson <fin-w@tutanota.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef LOKALIZETABPAGEBASE_H
#define LOKALIZETABPAGEBASE_H

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QString>
#include <QWidget>
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

    virtual KXMLGUIClient *guiClient();
    void reloadUpdatedXML();
    void setUpdatedXMLFile();

    virtual QString currentFilePath()
    {
        return QString();
    }

    // Manually trigger the tab page signals for the
    // status bar. Forces a full update of the status
    // bar content, useful when switching tabs.
    virtual void updateStatusBarContents() { };

    QString m_tabLabel;
    QString m_tabToolTip;
    QIcon m_tabIcon;

Q_SIGNALS:
    void signalUpdatedTabLabelAndIconAvailable(LokalizeTabPageBase *);
    void signalStatusBarCurrent(const int);
    void signalStatusBarTotal(const int);
    void signalStatusBarFuzzyNotReady(const int, const int);
    void signalStatusBarUntranslated(const int, const int);
    void signalStatusBarTranslationStatus(const QString);

protected:
    QDateTime lastXMLUpdate;
};

class LokalizeTabPageBaseNoQMainWindow : public QWidget, public KXMLGUIClient
{
    Q_OBJECT
public:
    explicit LokalizeTabPageBaseNoQMainWindow(QWidget *parent)
        : QWidget(parent)
        , KXMLGUIClient()
    {
    }

    ~LokalizeTabPageBaseNoQMainWindow() override = default;

    virtual KXMLGUIClient *guiClient();
    void reloadUpdatedXML();
    void setUpdatedXMLFile();

    virtual QString currentFilePath()
    {
        return QString();
    }

    // Manually trigger the tab page signals for the
    // status bar. Forces a full update of the status
    // bar content, useful when switching tabs.
    virtual void updateStatusBarContents() { };

    QString m_tabLabel;
    QString m_tabToolTip;
    QIcon m_tabIcon;

Q_SIGNALS:
    void signalUpdatedTabLabelAndIconAvailable(LokalizeTabPageBaseNoQMainWindow *);
    void signalStatusBarCurrent(const int);
    void signalStatusBarTotal(const int);
    void signalStatusBarFuzzyNotReady(const int, const int);
    void signalStatusBarUntranslated(const int, const int);
    void signalStatusBarTranslationStatus(const QString);

protected:
    QDateTime lastXMLUpdate;
};

#endif
