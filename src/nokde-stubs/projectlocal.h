#ifndef PROJECTLOCAL_H
#define PROJECTLOCAL_H

#include <QObject>

class ProjectLocal: public QObject
{
    Q_OBJECT

public:
    enum PersonRole { Translator, Reviewer, Approver, Undefined };

    ProjectLocal();
    ~ProjectLocal(){save();}

public slots:
    void setRole( int v ){mRole = (PersonRole)v;}
public:    
    void setRole( PersonRole v ){mRole = v;}
    PersonRole role() const {return static_cast<PersonRole>(mRole);}

    void setFirstRun( bool v ){mFirstRun = v;}
    bool firstRun() const{return mFirstRun;}

    void setSourceDir( const QString& s){mSourceDir = s;}
    QString sourceDir() const{return mSourceDir;}

    void save();
    void setDefaults(){}
  protected:

    // Personal
    int mRole;
    bool mFirstRun;
    QString mSourceDir;

  private:
};

#endif

