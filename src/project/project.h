/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2009 by Nick Shaforostoff <shafff@ukr.net>

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


#ifndef PROJECT_H
#define PROJECT_H

#include <QVector>
#include <QList>
#include "projectbase.h"

#define WEBQUERY_ENABLE

class ProjectModel;
class ProjectLocal;
namespace GlossaryNS{class Glossary;}
namespace GlossaryNS{class GlossaryWindow;}
namespace TM{class TMManagerWin;}

/**
 * Singleton object that represents project.
 * It is shared between EditorWindow 'mainwindows' that use the same project file.
 * Keeps project's KDirModel, Glossary and kross::actions
 *
 * GUI for config handling is implemented in prefs.cpp
 *
 * @short Singleton object that represents project
 */

///////// * Also provides list of web-query scripts
class Project: public ProjectBase
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Lokalize.Project")
    //qdbuscpp2xml -m -s project.h -o org.kde.lokalize.Project.xml
public:
    explicit Project();
    virtual ~Project();

    void load(const QString &file);
    void save();
    bool isLoaded()const{return !m_path.isEmpty();}
    ProjectModel* model();

    //void setPath(const QString& p){m_path=p;}
    QString path()const{return m_path;}
    QString projectDir()const{return m_projectDir;}
    QString poDir()const{return absolutePath(poBaseDir());}
    QString potDir()const{return absolutePath(potBaseDir());}
    QString branchDir()const{return absolutePath(ProjectBase::branchDir());}
    QString glossaryPath()const{return absolutePath(glossaryTbx());}
    QString qaPath()const{return absolutePath(mainQA());}
    GlossaryNS::Glossary* glossary()const{return m_glossary;}
    QString altTransDir()const{return absolutePath(altDir());}

    bool queryCloseForAuxiliaryWindows();

    void setDefaults();
// private slots:
//     void initLater();

public slots:
    Q_SCRIPTABLE QString translationsRoot()const{return poDir();}
    Q_SCRIPTABLE QString templatesRoot()const{return potDir();}


    Q_SCRIPTABLE QString targetLangCode()const{return ProjectBase::langCode();}
    Q_SCRIPTABLE QString sourceLangCode()const{return ProjectBase::sourceLangCode();}
    Q_SCRIPTABLE void init(const QString& path, const QString& kind, const QString& id,
                           const QString& sourceLang, const QString& targetLang);
    Q_SCRIPTABLE QString kind()const{return ProjectBase::kind();}

    Q_SCRIPTABLE QString absolutePath(const QString&) const;

    Q_SCRIPTABLE void setDesirablePath(const QString& path){m_desirablePath=path;}
    Q_SCRIPTABLE QString desirablePath() const{return m_desirablePath;}

    Q_SCRIPTABLE bool isTmSupported() const;

signals:
    Q_SCRIPTABLE void loaded();

public slots:
    void populateDirModel();
    void populateGlossary();

    void showTMManager();
    GlossaryNS::GlossaryWindow* showGlossary();
    GlossaryNS::GlossaryWindow* defineNewTerm(QString en=QString(),QString target=QString());

private:
    static Project* _instance;
    static void cleanupProject();
public:
    static Project* instance();
    static ProjectLocal* local(){return instance()->m_localConfig;}

private:
    QString m_path;
    QString m_desirablePath;
    ProjectLocal* m_localConfig;
    ProjectModel* m_model;
    GlossaryNS::Glossary* m_glossary;
    GlossaryNS::GlossaryWindow* m_glossaryWindow;
    TM::TMManagerWin* m_tmManagerWindow;

    //cache
    QString m_projectDir;
 };




#endif
