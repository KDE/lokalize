/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2009 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>
  SPDX-FileCopyrightText: 2022-2023 Łukasz Wojniłowicz <lukasz.wojnilowicz@gmail.com>
  SPDX-FileCopyrightText: 2025 Finley Watson <fin-w@tutanota.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef PROJECT_H
#define PROJECT_H

#include "projectbase.h"

class ProjectModel;
class ProjectLocal;
class QFileSystemWatcher;

namespace GlossaryNS
{
class Glossary;
}
namespace GlossaryNS
{
class GlossaryTab;
}
namespace TM
{
class TMManagerWin;
}

/**
 * Singleton object that represents project.
 * It is shared between EditorWindow 'mainwindows' that use the same project file.
 * Keeps project's KDirModel and Glossary
 *
 * GUI for config handling is implemented in prefs.cpp
 *
 * @short Singleton object that represents project
 */

class Project : public ProjectBase
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Lokalize.Project")
    // qdbuscpp2xml -m -s project.h -o org.kde.lokalize.Project.xml
public:
    explicit Project();
    ~Project() override;

    bool isLoaded() const
    {
        return !m_path.isEmpty();
    }

    bool isSourceFilePathsReady() const
    {
        return m_sourceFilePathsReady;
    }

    ProjectModel *model();

    // void setPath(const QString& p){m_path=p;}
    QString path() const
    {
        return m_path;
    }
    QString projectDir() const
    {
        return m_projectDir;
    }
    QString poDir() const
    {
        return absolutePath(poBaseDir());
    }
    QString potDir() const
    {
        return absolutePath(potBaseDir());
    }
    QString branchDir() const
    {
        return absolutePath(ProjectBase::branchDir());
    }
    QString potBranchDir() const
    {
        return absolutePath(ProjectBase::potBranchDir());
    }
    QString glossaryPath() const
    {
        return absolutePath(glossaryTbx());
    }
    QString qaPath() const
    {
        return absolutePath(mainQA());
    }
    GlossaryNS::Glossary *glossary() const
    {
        return m_glossary;
    }
    QString altTransDir() const
    {
        return absolutePath(altDir());
    }

    bool isFileMissing(const QString &filePath) const;

    void setDefaults() override;

public Q_SLOTS:
    Q_SCRIPTABLE void load(const QString &newProjectPath, const QString &defaultTargetLangCode = QString(), const QString &defaultProjectId = QString());
    Q_SCRIPTABLE void reinit();
    Q_SCRIPTABLE void save();

    Q_SCRIPTABLE QString translationsRoot() const
    {
        return poDir();
    }
    Q_SCRIPTABLE QString templatesRoot() const
    {
        return potDir();
    }

    Q_SCRIPTABLE QString targetLangCode() const
    {
        return ProjectBase::langCode();
    }
    Q_SCRIPTABLE QString sourceLangCode() const
    {
        return ProjectBase::sourceLangCode();
    }
    Q_SCRIPTABLE void init(const QString &path, const QString &kind, const QString &id, const QString &sourceLang, const QString &targetLang);
    Q_SCRIPTABLE QString kind() const
    {
        return ProjectBase::kind();
    }

    Q_SCRIPTABLE QString absolutePath(const QString &) const;
    Q_SCRIPTABLE QString relativePath(const QString &) const;

    Q_SCRIPTABLE void setDesirablePath(const QString &path)
    {
        m_desirablePath = path;
    }
    Q_SCRIPTABLE QString desirablePath() const
    {
        return m_desirablePath;
    }

    Q_SCRIPTABLE bool isTmSupported() const;

Q_SIGNALS:
    Q_SCRIPTABLE void loaded();
    void fileOpenRequested(const QString &, const bool setAsActive);
    void closed();

public Q_SLOTS:
    void populateDirModel();
    void populateGlossary();

    void showTMManager();
    GlossaryNS::GlossaryTab *glossaryTab();

    void projectOdfCreate();

private:
    static Project *_instance;
    static void cleanupProject();

public:
    static Project *instance();
    static ProjectLocal *local()
    {
        return instance()->m_localConfig;
    }

    const QMultiMap<QByteArray, QByteArray> &sourceFilePaths();
    void resetSourceFilePaths()
    {
        m_sourceFilePaths.clear();
        m_sourceFilePathsReady = false;
    }

    friend class FillSourceFilePathsJob;
Q_SIGNALS:
    void sourceFilePathsAreReady();

private:
    QString m_path;
    QString m_desirablePath;
    QFileSystemWatcher *m_projectFileWatcher{nullptr};
    ProjectLocal *m_localConfig{nullptr};
    ProjectModel *m_model{nullptr};
    GlossaryNS::Glossary *m_glossary{nullptr};
    GlossaryNS::GlossaryTab *m_glossaryTab{nullptr};
    TM::TMManagerWin *m_tmManagerWindow{nullptr};

    QMultiMap<QByteArray, QByteArray> m_sourceFilePaths;
    bool m_sourceFilePathsReady{false};

    // cache
    QString m_projectDir;
};

#endif
