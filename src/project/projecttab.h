/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2009 by Nick Shaforostoff <shafff@ukr.net>
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
