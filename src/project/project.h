/*****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2008  by Nick Shaforostoff <shafff@ukr.net>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

  In addition, as a special exception, the copyright holders give
  permission to link the code of this program with any edition of
  the Qt library by Trolltech AS, Norway (or with modified versions
  of Qt that use the same license as Qt), and distribute linked
  combinations including the two.  You must obey the GNU General
  Public License in all respects for all of the code used other than
  Qt. If you modify this file, you may extend this exception to
  your version of the file, but you are not obligated to do so.  If
  you do not wish to do so, delete this exception statement from
  your version.

**************************************************************************** */


#ifndef PROJECT_H
#define PROJECT_H

#include <QVector>
#include <QList>
#include "projectbase.h"
#include "projectmodel.h"

#define WEBQUERY_ENABLE

class KAction;
class KRecentFilesAction;
class ProjectModel;
namespace GlossaryNS{class Glossary;}
namespace TM{class SelectJob;}
class EditorWindow;
// class WebQueryThread;
// #include "webquerythread.h"
#include <threadweaver/Job.h>


/***
 * class to keep widgets that may be shared among MainWindows
 */
// class UiObjects
// {
//     
// };


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

public:
//    typedef KSharedPtr<Project> Ptr;

    explicit Project();
    virtual ~Project();

    void load(const QString &file);
    void save();
    bool isLoaded(){return !m_path.isEmpty();}
    ProjectModel* model();

    //void setPath(const QString& p){m_path=p;}
    QString path()const{return m_path;}
    QString projectDir()const;
    QString poDir()const{return absolutePath(poBaseDir());}
    QString potDir()const{return absolutePath(potBaseDir());}
    QString branchDir()const{return absolutePath(ProjectBase::branchDir());}
    QString glossaryPath()const{return absolutePath(glossaryTbx());}
    GlossaryNS::Glossary* glossary()const{return m_glossary;}

    QStringList scriptsList() const;

private slots:
    void initLater();

signals:
    void loaded();
private:
    QString absolutePath(const QString&)const;

public slots:

    void populateDirModel();
    void populateGlossary();
#ifdef WEBQUERY_ENABLE
    void populateWebQueryActions();
#endif
//     void populateKrossActions();

    void deleteScanJob(ThreadWeaver::Job*);
    void dispatchSelectJob(ThreadWeaver::Job*);//used fr safety: what mainwindow has been closed?
    void showTMManager();
    void showGlossary();
    void defineNewTerm(QString en=QString(),QString target=QString());


    void registerEditor(EditorWindow* e){}
    void unregisterEditor(EditorWindow* e){}


private:
    static Project* _instance;
    static void cleanupProject();
public:
    static Project* instance();

// public:
//     TMWordHash m_tmWordHash;

private:
    QString m_path;
    ProjectModel* m_model;
    GlossaryNS::Glossary* m_glossary;

    //TM scanning stats
    ushort m_tmCount;
    ushort m_tmAdded;
    ushort m_tmNewVersions;//e1.english==e2.english, e1.target!=e2.target
//     ushort m_tmTime;
//     QTime m_timeTracker;


//     QTime scanningTime;
//     WebQueryController* m_webQueryController;
//     WebQueryThread m_webQueryThread;
    QList<KAction*> m_projectActions;
    KRecentFilesAction* _openRecentProject;

    QVector<EditorWindow*> m_editors;
};

inline
ProjectModel* Project::model()
{
    if (KDE_ISUNLIKELY(!m_model))
        m_model=new ProjectModel(this);

    return m_model;
}



#endif
