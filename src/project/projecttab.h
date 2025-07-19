/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2009 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>
  SPDX-FileCopyrightText: 2023      Łukasz Wojniłowicz <lukasz.wojnilowicz@gmail.com>
  SPDX-FileCopyrightText: 2024-2025 Finley Watson <fin-w@tutanota.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef PROJECTTAB_H
#define PROJECTTAB_H

#include "lokalizetabpagebase.h"

#include <QProcess>

class ProjectWidget;
class KProcess;
class KXMLGUIClient;
class QContextMenuEvent;
class QLineEdit;
class QProgressBar;
class QStackedLayout;

/**
 * Project Overview Tab
 */
class ProjectTab : public LokalizeTabPageBaseNoQMainWindow
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Lokalize.ProjectOverview")
    // qdbuscpp2xml -m -s projecttab.h -o org.kde.lokalize.ProjectOverview.xml

public:
    explicit ProjectTab(QWidget *parent);
    ~ProjectTab() override = default;

    void contextMenuEvent(QContextMenuEvent *event) override;

    KXMLGUIClient *guiClient() override
    {
        return (KXMLGUIClient *)this;
    }
    QString currentFilePath() override;

    int unitsCount()
    {
        return m_currentUnitsCount;
    }
    void updateStatusBarContents() override;

Q_SIGNALS:
    void projectOpenRequested();

    void fileOpenRequested(const QString &, const bool setAsActive);

    void searchRequested(const QStringList &);
    void replaceRequested(const QStringList &);
    void spellcheckRequested(const QStringList &);

public Q_SLOTS:
    Q_SCRIPTABLE void setCurrentItem(const QString &url);
    Q_SCRIPTABLE QString currentItem() const;
    ///@returns list of selected files recursively
    Q_SCRIPTABLE QStringList selectedItems() const;
    Q_SCRIPTABLE bool currentItemIsTranslationFile() const;

private Q_SLOTS:
    /**
     * @short Sets the regex used to filter the list in Project Overview.
     *
     * The regex accepts wildcards and matches against the relative
     * paths of the files and directories below the project directory.
     *
     * @author Finley Watson
     */
    void setFilterRegExp();
    void setFocus();
    void scanFilesToTM();
    void pologyOnFiles();
    void addComment();
    /**
     * A sole purpose of this slot is to workaround the bug #460634.
     */
    void findTriggered();
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

    void pologyHasFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    ProjectWidget *m_browser{};
    QLineEdit *m_filterEdit{};

    KProcess *m_pologyProcess{};
    bool m_pologyProcessInProgress{};

    int m_currentUnitsCount{0};

    int m_fuzzy;
    int m_translated;
    int m_untranslated;
    bool m_done;
};

#endif
