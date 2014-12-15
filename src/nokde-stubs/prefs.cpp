#include "prefs.h"
#include "prefs_lokalize.h"
#include "projectbase.h"

#include <QLocale>
#include <QFileInfo>
#include <QStandardPaths>

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

Settings::Settings()
 : mDefaultLangCode(QLocale::system().name())
 , mAddColor(0x99,0xCC,0xFF)
 , mDelColor(0xFF,0x99,0x99)
 , mMsgFont()
 , mHighlightSpaces()
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
{}


Settings *Settings::self()
{
    static Settings* s=new Settings;
    return s;
}







#include "editortab.h"

ProjectBase::ProjectBase()
 : mProjectID("default")
 , mKind()
 , mLangCode(Settings::defaultLangCode())
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
{}



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
















