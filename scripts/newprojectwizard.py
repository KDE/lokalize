# -*- coding: utf-8 -*-
import sys,os
import codecs
import subprocess

try:
    import Lokalize
    import Project
    import Kross
    standalone=False


    utf8_decoder=codecs.getdecoder("utf8")

    T = Kross.module("kdetranslation")
    def i18n(text, args = []):
        if T is not None: return utf8_decoder(T.i18n(text, args))[0]
        # No translation module, return the untranslated string
        for a in range(len(args)): text = text.replace( ("%" + "%d" % ( a + 1 )), str(args[a]) )
        return text

    #copied from translate-toolkit unit test
    def tt_args(src, tgt, **kwargs):
        arg_list = [src, tgt]
        for flag, value in kwargs.iteritems():
            value = unicode(value)
            if len(flag) == 1: arg_list.append(u'-%s' % flag)
            else:              arg_list.append(u'--%s' % flag)
            if not value==None:arg_list.append(value)
        return arg_list

except:
    standalone=True
    def i18n(x): return x #TODO fix





from PyQt4.QtCore import *
from PyQt4.QtGui import *

try:
    from PyKDE4.kdecore import *
    from PyKDE4.kdeui import *
    from PyKDE4.kio import *
    allLanguagesList=KGlobal.locale().allLanguagesList()
except:
    class KComboBox(QComboBox):
        def __init__(self, parent):
            QComboBox.__init__(self, parent)

    class KLineEdit(QLineEdit):
        def __init__(self, parent):
            QLineEdit.__init__(self, parent)

    class ActiveLineEdit(KLineEdit):
        def __init__(self, parent):
            KLineEdit.__init__(self, parent)
        def mouseDoubleClickEvent(self, event): self.emit(SIGNAL("doubleClicked()"))

    class KUrlRequester (QWidget):
        def __init__(self, parent):
            QWidget.__init__(self, parent)
            self.le=ActiveLineEdit(self)
            self.btn=QToolButton(self)
            l=QHBoxLayout(self)
            l.setContentsMargins(0,0,0,0)
            l.addWidget(self.le)
            l.addWidget(self.btn)

            self.dialog=QFileDialog(self)
            self.connect(self.dialog,SIGNAL("fileSelected(QString)"),self.le,SLOT("setText(QString)"))
#            self.connect(self.dialog,SIGNAL("fileSelected(QString)"),self.setText)
            self.connect(self.dialog,SIGNAL("accepted()"),self.chCh)
            self.connect(self.le,SIGNAL("textChanged(QString)"),self,SIGNAL("textChanged(QString)"))
            self.connect(self.btn,SIGNAL("clicked()"),self.dialog,SLOT("show()"))
            self.connect(self.le,SIGNAL("doubleClicked()"),self.dialog,SLOT("show()"))
            self.setAcceptDrops(True)

        def dragEnterEvent(self, event):
            if event.mimeData().hasUrls():
                event.acceptProposedAction()

        def dropEvent(self, event):
            self.setText(QDir.toNativeSeparators(event.mimeData().urls()[0].toLocalFile()))
            event.acceptProposedAction()

        def chCh(self): self.le.setText(self.dialog.selectedFiles()[0])

        def setClickMessage(self,_): return
        def setFilter(self,ftr): self.dialog.setFilter(ftr)
        def lineEdit(self): return self.le
        def text(self): return self.le.text()
        def setText(self,text): return self.le.setText(text)
        def fileDialog(self): return self.dialog
        def setMode(self,mode):
            if mode&2:self.dialog.setFileMode(QFileDialog.Directory)

   
    class kFile:
        def __init__(self):
            self.File=1
            self.Directory=2
            self.ExistingOnly=4
            self.LocalOnly=8

        def Modes(self, modes): return modes
    KFile=kFile()

    allLanguagesList=QStringList()
    for i in range(QLocale.Chewa):
        allLanguagesList.append(QLocale(QLocale.Language(i)).name())
#        allLanguagesList.append(QLocale.languageToString(QLocale.Language(i)))


class RadioActiveButton(QRadioButton):
    def __init__(self, text, parent):
        QRadioButton.__init__(self, text, parent)
    def mouseDoubleClickEvent(self, event): self.emit(SIGNAL("doubleClicked()"))

   
try: from translate.convert import odf2xliff
except: print '%s' % i18n('Translate-Toolkit not found. Please install this package for the feature to work.')


pages=[]

pages.append('TypePage')
class TypePage(QWizardPage):
    def __init__(self, parent):
        QWizardPage.__init__(self, parent)
        self.setTitle(i18n("What do you want to do?"))
        self.setSubTitle(i18n("Identify the kind of project you want:"))
        self.group=QButtonGroup(self)
        document=RadioActiveButton(i18n('Translate a document'),self)
        document.setChecked(True)
        gui=RadioActiveButton(i18n('Translate application interface'),self)
        layout=QVBoxLayout(self)
        layout.addWidget(document)
        layout.addWidget(gui)
        self.group.addButton(document,0)
        self.group.addButton(gui,1)

        #self.connect(document,SIGNAL("doubleClicked()"),self.wizard(),SLOT("next()"))
        self.connect(document,SIGNAL("doubleClicked()"),self,SIGNAL("nextRequested()"))
        self.connect(gui,SIGNAL("doubleClicked()"),self,SIGNAL("nextRequested()"))

        self.registerField('kind-document',document)
        self.registerField('kind-gui',gui)

    def nextId(self): return pages.index(['OdfSourcePage','SoftSourcePage'][self.group.checkedId()])

pages.append('OdfSourcePage')
class OdfSourcePage(QWizardPage):
    def __init__(self, parent):
        QWizardPage.__init__(self, parent)
        self.setTitle(i18n("Choose a document to be translated"))
        self.setSubTitle(i18n("Choose document in a source language."))
        self.group=QButtonGroup(self)
        self.files=QRadioButton(i18n('Select file:'),self)
        self.files.setChecked(True)
        self.group.addButton(self.files,0)
        self.odfFilePath=KUrlRequester(self)
        self.odfFilePath.setFilter('*.odt *.ods|OpenDocument files')
        self.odfFilePath.setMode(KFile.Modes(KFile.File or KFile.ExistingOnly or KFile.LocalOnly))

        self.dirs=QRadioButton(i18n('Select a folder:'),self)
        self.group.addButton(self.dirs,1)
        self.odfDirPath=KUrlRequester(self)
        self.odfDirPath.setMode(KFile.Modes(KFile.Directory or KFile.ExistingOnly or KFile.LocalOnly))

        self.connect(self.odfFilePath,SIGNAL("textChanged(QString)"),self,SIGNAL("completeChanged()"))
        self.connect(self.odfDirPath.lineEdit(),SIGNAL("textChanged(QString)"),self,SIGNAL("completeChanged()"))
        self.connect(self.group,SIGNAL("buttonClicked(int)"),self,SIGNAL("completeChanged()"))

        self.connect(self.files,SIGNAL("toggled(bool)"),self.odfFilePath,SLOT("setEnabled(bool)"))
        self.connect(self.dirs,SIGNAL("toggled(bool)"),self.odfDirPath,SLOT("setEnabled(bool)"))
        self.odfDirPath.setEnabled(False)

        layout=QFormLayout(self)
        layout.addRow(self.files,self.odfFilePath)
        layout.addRow(self.dirs,self.odfDirPath)

        self.registerField('odf-template-source-files',self.files)
        self.registerField('odf-template-source-dirs',self.dirs)
        self.registerField('odf-template-files',self.odfFilePath.lineEdit())
        self.registerField('odf-template-dir',self.odfDirPath.lineEdit())

    def nextId(self): return pages.index('NamePage')

    def initializePage(self):
        self.odfDirPath.lineEdit().setText(QDesktopServices.storageLocation(QDesktopServices.StandardLocation(QDesktopServices.DocumentsLocation)))
        self.odfFilePath.fileDialog().setUrl(KUrl(QDesktopServices.storageLocation(QDesktopServices.StandardLocation(QDesktopServices.DocumentsLocation))))



pages.append('NamePage')
class NamePage(QWizardPage):
    def __init__(self, parent):
        QWizardPage.__init__(self, parent)
        self.setTitle(i18n("Choose project name and location"))
        self.setSubTitle(i18n("If you choose custom paths then the source files will be copied to it."))
        ml=QVBoxLayout(self)

        self.samedir=QCheckBox(i18n('Use initial source dir, generate name automatically'),self)
        ml.addWidget(self.samedir)

        group=QGroupBox(i18n('Custom paths'),self)
        ml.addWidget(group)
        layout=QFormLayout(group)
        self.projectName=KLineEdit(self)
        layout.addRow(i18n("Name:"),self.projectName)
        self.projectLocation=KUrlRequester(self)
        self.projectLocation.setMode(KFile.Modes(KFile.Directory or KFile.ExistingOnly or KFile.LocalOnly))
        layout.addRow(i18n("Location:"),self.projectLocation)

        self.registerField('name*',self.projectName)
        self.registerField('location*',self.projectLocation.lineEdit())

        self.connect(self.samedir,SIGNAL("toggled(bool)"),group,SLOT("setDisabled(bool)"))
        self.samedir.setChecked(True)

    def nextId(self): return pages.index('LangPage')

    def initializePage(self):
        if self.field('odf-template-source-files').toBool():
            p=self.field('odf-template-files').toString()
            p=QFileInfo(p).absolutePath()
        else:
            p=self.field('odf-template-dir').toString()
        self.projectLocation.lineEdit().setText(QFileInfo(p).absolutePath())
        self.projectName.setText(QFileInfo(p).fileName())

        #field(fields[self.page(0).group.checkedId()]).toString()
        #self.projectLocation.lineEdit().setText(QDesktopServices.storageLocation(QDesktopServices.StandardLocation.DocumentsLocation))

pages.append('LangPage')
# TODO cache, make it cross-platform
class LanguageListModel(QStringListModel):
    def __init__(self, parent):
        QStringListModel.__init__(self,allLanguagesList,parent)
        try:
            KGlobal.locale()
            self.kGlobalPresent=True
        except: self.kGlobalPresent=False

    def data(self,index,role):
        if role==Qt.DecorationRole:
            code=self.stringList()[index.row()]
            if not '_' in code: code=QLocale(code).name()
            code=code[3:].toLower()
            return QVariant(QIcon(QString.fromUtf8("/usr/share/locale/l10n/%1/flag.png").arg(code)))
        elif self.kGlobalPresent and role==Qt.DisplayRole:
            code=self.stringList()[index.row()]
            return QVariant(KGlobal.locale().languageCodeToName(code))
        return QStringListModel.data(self,index,role)

class LangPage(QWizardPage):
    def __init__(self, parent):
        QWizardPage.__init__(self, parent)
        self.setTitle(i18n("Choose source and target languages"))
        self.setSubTitle(i18n("Click on a combobox then start typing the name of the language."))
        self.languageListModel=LanguageListModel(self)
        self.sourceLang=KComboBox(self)
        self.targetLang=KComboBox(self)
        self.sourceLang.setModel(self.languageListModel)
        self.targetLang.setModel(self.languageListModel)

        tlang=defaultTargetLang
        if tlang=='en_US':tlang=QLocale.system().name()
        tpos=self.languageListModel.stringList().indexOf(tlang)
        print self.languageListModel.stringList()[2]
        print tlang
        print QLocale.system().name()
        if tpos==-1: tpos=self.languageListModel.stringList().indexOf(QRegExp('^'+tlang[:2]+'.*'))
        if tpos!=-1: self.targetLang.setCurrentIndex(tpos)
        self.sourceLang.setCurrentIndex(self.languageListModel.stringList().indexOf('en_US'))

        layout=QFormLayout(self)
        layout.addRow(i18n("Source:"),self.sourceLang)
        layout.addRow(i18n("Target:"),self.targetLang)

        self.registerField('source-lang',self.sourceLang)
        self.registerField('target-lang',self.targetLang)

    def nextId(self): return -1

##########################


pages.append('SoftSourcePage')
class SoftSourcePage(QWizardPage):
    def __init__(self, parent):
        QWizardPage.__init__(self, parent)
        self.setTitle(i18n("Choose a type of software project"))
        self.setSubTitle(i18n("Different projects use different translation files filesystem layout."))
        self.group=QButtonGroup(self)
        self.kde=RadioActiveButton(i18n('KDE'),self)
        self.kde.setChecked(True)
        self.group.addButton(self.kde,0)

        self.connect(self.kde,SIGNAL("doubleClicked()"),self,SIGNAL("nextRequested()"))

        layout=QFormLayout(self)
        layout.addWidget(self.kde)
        self.registerField('soft-kde',self.kde)

    def nextId(self): return pages.index(['KdeSourcePage','generic'][self.group.checkedId()])


pages.append('KdeSourcePage')
class KdeSourcePage(QWizardPage):
    def __init__(self, parent):
        QWizardPage.__init__(self, parent)
        self.setTitle(i18n("Choose location of your software translation project"))
        self.setSubTitle(i18n("Choose whether you already have translation files on disk, or if you want to download them now."))
        self.group=QButtonGroup(self)
        self.existing=QRadioButton(i18n('Existing:'),self)
        self.group.addButton(self.existing,0)
        self.existingLocation=KUrlRequester(self)
        self.existingLocation.setMode(KFile.Modes(KFile.Directory or KFile.ExistingOnly or KFile.LocalOnly))
        self.existingLocation.setClickMessage(i18n("Your language's folder containing messages/ and docmessages/ subfolders"))
        #self.existingLocation.setStartDir(QDesktopServices.storageLocation(QDesktopServices.DocumentsLocation))
        self.connect(self.existingLocation,SIGNAL("textChanged(QString)"),self,SIGNAL("completeChanged()"))
        self.connect(self.existing,SIGNAL("toggled(bool)"),self.existingLocation,SLOT("setEnabled(bool)"))
        self.connect(self.group,SIGNAL("buttonClicked(int)"),self,SIGNAL("completeChanged()"))

        self.svn=QRadioButton(i18n('Get from svn repository\n(approx. 20 MB):'),self)
        self.group.addButton(self.svn,1)
        self.languageListModel=LanguageListModel(self)
        self.targetLang=KComboBox(self)
        self.targetLang.setModel(self.languageListModel)

        tlang=defaultTargetLang
        if tlang=='en_US':tlang=QLocale.system().name()
        tpos=self.languageListModel.stringList().indexOf(tlang)
        if tpos==-1: tpos=self.languageListModel.stringList().indexOf(QRegExp('^'+tlang[:2]+'.*'))
        if tpos!=-1: self.targetLang.setCurrentIndex(tpos)

        self.svnRootLocation=KUrlRequester(self)
        self.svnRootLocation.setMode(KFile.Modes(KFile.Directory or KFile.ExistingOnly or KFile.LocalOnly))
        self.svnRootLocation.setClickMessage(i18n("Local download folder (will/does contain trunk/l10n-kde4/...)"))
        #self.svnRootLocation.setStartDir(QDesktopServices.storageLocation(QDesktopServices.DocumentsLocation))
        self.connect(self.svnRootLocation,SIGNAL("textChanged(QString)"),self,SIGNAL("completeChanged()"))

        w=QWidget(self)
        self.connect(self.svn,SIGNAL("toggled(bool)"),w,SLOT("setEnabled(bool)"))
        self.svn.setChecked(True)
        self.existing.setChecked(True)


        self.progress=QProgressBar(self)
        self.progress.hide()
        self.counter=0


        layout=QFormLayout(self)
        layout.addRow(self.existing,self.existingLocation)
        svnl=QVBoxLayout(w)
        svnl.setContentsMargins(0,0,0,0)
        svnl.addWidget(self.targetLang)
        svnl.addWidget(self.svnRootLocation)
        layout.addRow(self.svn,w)
        layout.addWidget(self.progress)

        self.registerField('kde-existing-location',self.existingLocation.lineEdit())
        self.registerField('kde-svn-lang',self.targetLang)
        self.registerField('kde-svn-root-location',self.svnRootLocation.lineEdit())
        #self.registerField('kde-svn-project-location',self.svnRootLocation)
        self.registerField('kde-existing',self.existing)
        self.registerField('kde-svn',self.svn)

        self.setCommitPage(True)

    #def nextId(self): return pages.index(['svn-existing','KdeCheckoutPage'][self.group.checkedId()])

    def isComplete(self):
        if self.existing.isChecked():
            return QFileInfo(self.existingLocation.text()).exists()
        return QFileInfo(self.svnRootLocation.text()).exists()

    def validatePage(self):
        if self.existing.isChecked(): return True
        if os.system('which svn')!=0:
            KMessageBox.error(self,i18n("Please install 'subversion' package\nto have Lokalize download KDE translation files."),i18n("Subversion client not found"))
            return False

        self.progress.show()
        self.progress.setMaximum(6*30+3*10)
        QCoreApplication.processEvents()

        #localsvnroot=self.field('kde-svn-location').toString()
        localsvnroot=self.svnRootLocation.text()
        if not QFileInfo('%s/trunk' % localsvnroot).exists():
            print 'svn -N co svn://anonsvn.kde.org/home/kde/trunk "%s/trunk"' % localsvnroot
            os.system('svn -N co svn://anonsvn.kde.org/home/kde/trunk "%s/trunk"' % localsvnroot)
        else:
            print 'already exists:',
            print '"%s/trunk"' % localsvnroot
        self.reportProgress(10)

        if not QFileInfo('%s/trunk/l10n-kde4' % localsvnroot).exists():
            print 'svn -N up "%s/trunk/l10n-kde4"' % localsvnroot
            os.system('svn -N up "%s/trunk/l10n-kde4"' % localsvnroot)
        self.reportProgress(5)


        langs=allLanguagesList
        lang=langs[self.field('kde-svn-lang').toInt()[0]]

        for langlang in [lang[:2], lang]:
            print 'svn --set-depth files up "%s/trunk/l10n-kde4/%s"' % (localsvnroot, langlang)
            os.system('svn --set-depth files up "%s/trunk/l10n-kde4/%s"' % (localsvnroot, langlang))
            os.system('svn --set-depth files up "%s/trunk/l10n-kde4/%s"' % (localsvnroot, langlang))
            self.reportProgress(5)
            os.system('svn --set-depth infinity up "%s/trunk/l10n-kde4/%s/docs"' % (localsvnroot, langlang))
            os.system('svn --set-depth infinity up "%s/trunk/l10n-kde4/%s/docs"' % (localsvnroot, langlang))
            self.reportProgress(15)
            
            
            loc="%s/trunk/l10n-kde4/%s" % (localsvnroot, langlang)
            if os.path.exists(loc):
                self.existingLocation.setText(loc)
                try: self.existingLocation.setUrl(KUrl(loc))
                except: print  'KUrlRequester PyKDE4 bug'

            
        for langlang in [lang, lang[:2],'templates']:
            os.system('svn --set-depth infinity up "%s/trunk/l10n-kde4/%s/messages"' % (localsvnroot, langlang))
            os.system('svn --set-depth infinity up "%s/trunk/l10n-kde4/%s/messages"' % (localsvnroot, langlang))
            self.reportProgress(15)
            os.system('svn --set-depth infinity up "%s/trunk/l10n-kde4/%s/docmessages"' % (localsvnroot, langlang))
            os.system('svn --set-depth infinity up "%s/trunk/l10n-kde4/%s/docmessages"' % (localsvnroot, langlang))
            self.reportProgress(15)
            os.system('svn --set-depth infinity up "%s/trunk/l10n-kde4/%s"' % (localsvnroot, langlang))
            os.system('svn --set-depth infinity up "%s/trunk/l10n-kde4/%s"' % (localsvnroot, langlang))
            self.reportProgress(10)

        os.system('svn --set-depth infinity up "%s/trunk/l10n-kde4/scripts"' % localsvnroot)
        os.system('svn --set-depth infinity up "%s/trunk/l10n-kde4/scripts"' % localsvnroot)
        self.reportProgress(10)

        return True

    def reportProgress(self, weight):
        self.counter+=weight
        self.progress.setValue(self.counter)
        QCoreApplication.processEvents()


##########################
class ProjectAssistant(QWizard):
    def __init__(self):
        QWizard.__init__(self)
        for p in pages:
            exec "self.addPage( %s(self) )" % p

        #for page in [self.page(pageId) for pageId in self.pageIds()]:
        for page in [self.page(pages.index(pageId)) for pageId in pages]:
            self.connect(page,SIGNAL("nextRequested()"),self,SLOT("next()"))
            
        self.connect(self,SIGNAL("finished(int)"),self,SLOT("deleteLater()"))
        self.connect(self,SIGNAL("accepted()"),self.handleAccept)

    def handleAccept(self):
        fs=lambda name:self.field(name).toString()
        fi=lambda name:self.field(name).toInt()[0]
        fb=lambda name:self.field(name).toBool()

        kinds=['odf','kde']
        projectKind=kinds[self.page(0).group.checkedId()]
        langs=allLanguagesList

        doInit=True
        projectfilename='index.lokalize'
        #if projectKind=='odf' and fb('odf-template-source-files'):
            #projectfilename=QFileInfo(fs('odf-template-files')).baseName()+'.lokalize'

        if projectKind=='odf':
            loc=QDir(fs('location'))
            loc.mkdir(fs('name'))
            projectFilePath=loc.absoluteFilePath(fs('name')+'/'+projectfilename)
            targetlang=langs[fi('target-lang')]
            name=fs('name').toLower()
        elif projectKind=='kde':
            name='kde'
            loc=QDir(fs('kde-existing-location'))
            targetlang=loc.dirName()
            print 'project lang: %s' % targetlang
            print 'project loc: %s' % fs('kde-existing-location')

            l=loc.entryList(QStringList('*.lokalize'), QDir.Filter(QDir.Files))
            if len(l):
                doInit=False
                projectfilename=l[0]
            projectFilePath=loc.absoluteFilePath(projectfilename)

        if projectKind=='odf':
            files=[fs('odf-template-files')]
            for f in files:
                info=QFileInfo(f)
                xlf=QDir.toNativeSeparators(info.absolutePath()+'/'+info.baseName()+'.xlf')
                #this makes Lokalize crash on close
                #try: odf2xliff.main(tt_args(unicode(f), unicode(xlf)))
                #except:
                if True:
                    print 'odf2xliff via subprocess.call', unicode(f),  unicode(xlf)
                    try:
                        retcode = subprocess.call(['odf2xliff', unicode(f),  unicode(xlf)])
                        print >>sys.stderr
                    except OSError, e:
                        print >>sys.stderr, "Execution failed:", e


        self.projectName=name+'-'+targetlang
        self.projectSourceLang=langs[fi('source-lang')]
        self.projectTargetLang=targetlang
        self.projectKind=projectKind
        self.projectFilePath=projectFilePath

        self.projectShouldBeInitialized=doInit
        if doInit and not standalone:
            Lokalize.openProject(projectFilePath)
            Project.init(projectFilePath, projectKind, self.projectName, self.projectSourceLang, targetlang)
            Lokalize.openProject(projectFilePath)



if __name__ == "__main__":
    app = QApplication(sys.argv)

    args=QCoreApplication.arguments()
    defaultTargetLang=''
    if len(args)==3:
        defaultSourceLang=args[1]
        defaultTargetLang=args[2]


    myassistant=ProjectAssistant()
    code=myassistant.exec_()

    if code and myassistant.projectShouldBeInitialized:
        ourPath=([p for p in sys.path if os.path.exists(p+'/newprojectwizard_standalone.pyw')]+[''])[0]
        file=open(ourPath+'/projectconf.tmp','w')
        vars=["projectFilePath", "projectKind", "projectName", "projectSourceLang", "projectTargetlang"]
        for var in vars:
            exec ("file.write(myassistant.%s)" % var) in locals()
            file.write('\n')

else:
    defaultTargetLang=Project.targetLangCode()
    myassistant=ProjectAssistant()
    myassistant.show()


