/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2009 Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

*/

#ifndef PROJECTTAB_H
#define PROJECTTAB_H

#include "lokalizesubwindowbase.h"

#include <KMainWindow>
#include <KProcess>

#include <KXMLGUIClient>

class QStackedLayout;
class ProjectWidget;
class QLineEdit;
class QContextMenuEvent;
class QProgressBar;

/**
 * Project Overview Tab
 */
class ProjectTab: public LokalizeSubwindowBase2
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Lokalize.ProjectOverview")
    //qdbuscpp2xml -m -s projecttab.h -o org.kde.lokalize.ProjectOverview.xml

public:
    explicit ProjectTab(QWidget *parent);
    ~ProjectTab() override = default;

    void contextMenuEvent(QContextMenuEvent *event) override;

    void hideDocks() override {}
    void showDocks() override {}
    KXMLGUIClient* guiClient() override
    {
        return (KXMLGUIClient*)this;
    }
    QString currentFilePath() override;

    int unitsCount()
    {
        return m_currentUnitsCount;
    }
    void setLegacyUnitsCount(int to);

Q_SIGNALS:
    void projectOpenRequested(QString path);
    void projectOpenRequested();

    void fileOpenRequested(const QString&, const bool setAsActive);

    void searchRequested(const QStringList&);
    void replaceRequested(const QStringList&);
    void spellcheckRequested(const QStringList&);

public Q_SLOTS:
    Q_SCRIPTABLE void setCurrentItem(const QString& url);
    Q_SCRIPTABLE QString currentItem() const;
    ///@returns list of selected files recursively
    Q_SCRIPTABLE QStringList selectedItems() const;
    Q_SCRIPTABLE bool currentItemIsTranslationFile() const;
    void showRealProjectOverview();
    void showWelcomeScreen();

    //Q_SCRIPTABLE bool isShown() const;

private Q_SLOTS:
    void setFilterRegExp();
    void setFocus();
    void scanFilesToTM();
    void pologyOnFiles();
    void addComment();
    void searchInFiles(bool templ = false);
    void searchInFilesInclTempl();
    void openFile();
    void findInFiles();
    void replaceInFiles();
    void spellcheckFiles();

    void gotoPrevFuzzyUntr();
    void gotoNextFuzzyUntr();
    void gotoPrevFuzzy();
    void gotoNextFuzzy();
    void gotoPrevUntranslated();
    void gotoNextUntranslated();
    void gotoPrevTemplateOnly();
    void gotoNextTemplateOnly();
    void gotoPrevTransOnly();
    void gotoNextTransOnly();
    void toggleTranslatedFiles();

    void updateStatusBar(int fuzzy = 0, int translated = 0, int untranslated = 0, bool done = false);
    void initStatusBarProgress();

    void pologyHasFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    ProjectWidget* m_browser;
    QLineEdit* m_filterEdit;
    QProgressBar* m_progressBar;

    QStackedLayout *m_stackedLayout;

    KProcess* m_pologyProcess;
    bool m_pologyProcessInProgress;

    int m_legacyUnitsCount, m_currentUnitsCount;
};

#endif
