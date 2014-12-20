#include "prefs.h"
#include "prefs_lokalize.h"
#include "projectbase.h"
#include "projectlocal.h"
#include "tmtab.h"

#include "kaboutdata.h"

#include <QLocale>
#include <QFileInfo>
#include <QStandardPaths>
#include <QFileOpenEvent>
#include <QMessageBox>
#include <QStringBuilder>
#include <QSettings>

SettingsController* SettingsController::_instance=0;
void SettingsController::cleanupSettingsController()
{
  delete SettingsController::_instance;
  SettingsController::_instance = 0;
}

SettingsController* SettingsController::instance()
{
    if (_instance==0){
        _instance=new SettingsController;
        ///qAddPostRoutine(SettingsController::cleanupSettingsController);
    }

    return _instance;
}

QString fullUserName();

Settings::Settings()
 : mDefaultLangCode(QLocale::system().name())
 , mAddColor(0x99,0xCC,0xFF)
 , mDelColor(0xFF,0x99,0x99)
 , mMsgFont()
 , mHighlightSpaces(true)
 , mLeds(false)

    // Editor
 , mAutoApprove(true)
 , mAutoSpellcheck(false)
 , mMouseWheelGo(false)

    // TM
 , mPrefetchTM(false)
 , mAutoaddTM(true)
 , mScanToTMOnOpen(false)

 , mWordCompletionLength(3)
 , mSuggCount(10)
{
    QSettings s;
    mAuthorName = s.value(QStringLiteral("Author/Name"), QString()).toString();
    if (mAuthorName.isEmpty()) mAuthorName = fullUserName();
}

void Settings::save()
{
    QSettings s;
    s.setValue(QStringLiteral("Author/Name"), mAuthorName);
}

Settings *Settings::self()
{
    static Settings* s=new Settings;
    return s;
}







#include "editortab.h"

ProjectBase::ProjectBase()
 : m_tmTab(0)
 , mProjectID(QStringLiteral("default"))
 , mKind()
 , mTargetLangCode(Settings::defaultLangCode())
 , mSourceLangCode("en_US")
 , mPoBaseDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
 , mPotBaseDir()
 , mBranchDir()
 , mAltDir()
 , mGlossaryTbx(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)+"/terms.tbx")
 , mMainQA(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)+"/main.lqa")

    // RegExps
 , mAccel("&")
 , mMarkup("(<[^>]+>)+|(&[A-Za-z_:][A-Za-z0-9_\\.:-]*;)+")
 , mWordWrap(80)
{
    QSettings s;
    mSourceLangCode = s.value(QStringLiteral("Project/SourceLangCode"), mSourceLangCode).toString();
    mTargetLangCode = s.value(QStringLiteral("Project/TargetLangCode"), mTargetLangCode).toString();
}

void ProjectBase::save()
{
    QSettings s;
    s.setValue(QStringLiteral("Project/SourceLangCode"), mSourceLangCode);
    s.setValue(QStringLiteral("Project/TargetLangCode"), mTargetLangCode);
}

ProjectLocal::ProjectLocal()
 : mRole(Translator)
{
    QSettings s;
    mRole = s.value("Project/AuthorRole", mRole).toInt();
}

void ProjectLocal::save()
{
    QSettings s;
    s.setValue(QStringLiteral("Project/AuthorRole"), mRole);
}

EditorTab* ProjectBase::fileOpen(QString filePath, int entry, bool setAsActive, const QString& mergeFile, bool silent)
{
    if (filePath.length())
    {
        FileToEditor::const_iterator it=m_fileToEditor.constFind(filePath);
        if (it!=m_fileToEditor.constEnd())
        {
            qWarning()<<"already opened:"<<filePath;
            if (EditorTab* e=it.value())
            {
                e->activateWindow();
                return e;
            }
        }
    }

    QByteArray state=m_lastEditorState;
    EditorTab* w=new EditorTab(0);

    QString suggestedDirPath;
    if (EditorTab* e=qobject_cast<EditorTab*>(QApplication::activeWindow()))
    {
        QString fp=e->currentFilePath();
        if (fp.length()) suggestedDirPath=QFileInfo(fp).absolutePath();
    }

    if (!w->fileOpen(filePath,suggestedDirPath,silent))
    {
        w->deleteLater();
        return 0;
    }
    if (filePath.length())
    {
        FileToEditor::const_iterator it=m_fileToEditor.constFind(filePath);
        if (it!=m_fileToEditor.constEnd())
        {
            qWarning()<<"already opened:"<<filePath;
            if (EditorTab* e=it.value())
            {
                e->activateWindow();
                w->deleteLater();
                return e;
            }
        }
    }

    w->show();

    if (!state.isEmpty())
        w->restoreState(QByteArray::fromBase64(state));

    if (entry/* || offset*/)
        w->gotoEntry(DocPosition(entry/*, DocPosition::Target, 0, offset*/));

    if (!mergeFile.isEmpty())
        w->mergeOpen(mergeFile);

//    m_openRecentFileAction->addUrl(QUrl::fromLocalFile(filePath));//(w->currentUrl());
    connect(w, SIGNAL(destroyed(QObject*)),this,SLOT(editorClosed(QObject*)));
    connect(w, SIGNAL(fileOpenRequested(QString,QString,QString)),this,SLOT(fileOpen(QString,QString,QString)));
//    connect(w, SIGNAL(tmLookupRequested(QString,QString)),this,SLOT(lookupInTranslationMemory(QString,QString)));

    filePath=w->currentFilePath();
    QStringRef fnSlashed=filePath.midRef(filePath.lastIndexOf('/'));
    FileToEditor::const_iterator i = m_fileToEditor.constBegin();
    while (i != m_fileToEditor.constEnd())
    {
        if (i.key().endsWith(fnSlashed))
        {
            i.value()->setFullPathShown(true);
            w->setFullPathShown(true);
        }
        ++i;
    }
    m_fileToEditor.insert(filePath,w);

    //emit editorAdded();
    return w;
}

void ProjectBase::editorClosed(QObject* obj)
{
    m_fileToEditor.remove(m_fileToEditor.key(static_cast<EditorTab*>(obj)));
}

bool ProjectBase::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::FileOpen)
    {
        QFileOpenEvent *e = static_cast<QFileOpenEvent *>(event);
        fileOpen(e->file());
        return true;
    }
    return QObject::eventFilter(obj, event);
}

void ProjectBase::showTM()
{
    if (!m_tmTab)
    {
        m_tmTab=new TM::TMTab(0);
        connect(m_tmTab, SIGNAL(fileOpenRequested(QString,QString,QString)),this,SLOT(fileOpen(QString,QString,QString)));
    }
    m_tmTab->show();
}



KAboutData* KAboutData::instance=0;

KAboutData::KAboutData(const char*, const QString& n, const QString& v, const QString& d, KAboutLicense::L, const QString& c)
 : name(n)
 , version(v)
 , description(d)
 , copyright(c)
{
    KAboutData::instance=this;
}

void KAboutData::addAuthor(const QString& name, const QString&, const QString& mail)
{
//    Credit c;
//    c.name=name;
//    c.mail=mail;
//    credits.append(c);
}

void KAboutData::addCredit(const QString& name, const QString& forwhat, const QString& mail, const QString& site)
{
    Credit c;
    c.name=name;
    c.mail=mail;
    c.what=forwhat;
    c.site=site;
    credits.append(c);
}


void KAboutData::doAbout()
{
    QString cs;
    foreach(const Credit& c, credits)
    {
        cs+=c.name%": "%c.what%"<br >";
    }
    QMessageBox::about(0, name, "<h3>"%name%' '%version%"</h3><p>"%description%"</p><font style=\"font-weight:normal\"><p>"%copyright.replace('\n', "<br>")%"</p><br>Credits:<br>"%cs%"</font>");
}










