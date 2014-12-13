#include "prefs.h"
#include "prefs_lokalize.h"
#include "projectbase.h"

#include <QLocale>
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

