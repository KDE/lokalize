/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef PREFS_H
#define PREFS_H

#include <QLineEdit>
class KEditListWidget;

/**
 * Singleton that manages cfgs for Lokalize and projects
 */
class SettingsController: public QObject
{
    Q_OBJECT

public:
    SettingsController();
    ~SettingsController();

    bool dirty;

    void setMainWindowPtr(QWidget* w)
    {
        m_mainWindowPtr = w;
    }
    QWidget* mainWindowPtr()
    {
        return m_mainWindowPtr;
    }

public Q_SLOTS:
    void showSettingsDialog();

    bool ensureProjectIsLoaded();
    QString projectOpen(QString path = QString(), bool doOpen = true);
    bool projectCreate();
    void projectConfigure();

    void reflectProjectConfigChange();

    void reflectRelativePathsHack();

Q_SIGNALS:
    void generalSettingsChanged();

private:
    KEditListWidget* m_scriptsRelPrefWidget; //HACK to get relative filenames in the project file
    KEditListWidget* m_scriptsPrefWidget;
    QWidget* m_mainWindowPtr;

private:
    static SettingsController* _instance;
    static void cleanupSettingsController();
public:
    static SettingsController* instance();
};

/**
 * helper widget to save relative paths in project file,
 * thus allowing its publishing in e.g. svn
 */
class RelPathSaver: public QLineEdit
{
    Q_OBJECT
public:
    explicit RelPathSaver(QWidget* p): QLineEdit(p) {}
public Q_SLOTS:
    void setText(const QString&);
};


/**
 * helper widget to save lang code text values
 * identified by LanguageListModel string index internally
 */
class LangCodeSaver: public QLineEdit
{
    Q_OBJECT
public:
    explicit LangCodeSaver(QWidget* p): QLineEdit(p) {}
public Q_SLOTS:
    void setLangCode(int);
};

void writeUiState(const char* elementName, const QByteArray&);
QByteArray readUiState(const char* elementName);

#endif
