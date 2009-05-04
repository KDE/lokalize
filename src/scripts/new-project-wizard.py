# -*- coding: utf-8 -*-
import Kross
import Lokalize
import Project
import sys,os
import codecs
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyKDE4.kdecore import *
from PyKDE4.kdeui import *
from PyKDE4.kio import *

utf8_decoder=codecs.getdecoder("utf8")

T = Kross.module("kdetranslation")
def i18n(text, args = []):
    if T is not None: return utf8_decoder(T.i18n(text, args))[0]
    # No translation module, return the untranslated string
    for a in range(len(args)): text = text.replace( ("%" + "%d" % ( a + 1 )), str(args[a]) )
    return text

try: from translate.convert import odf2xliff
except: print i18n('Translate-Toolkit not found. Please install this package for the feature to work.')


#copied from translate-toolkit unit test
def args(src, tgt, **kwargs):
    arg_list = [src, tgt]
    for flag, value in kwargs.iteritems():
        value = unicode(value)
        if len(flag) == 1: arg_list.append(u'-%s' % flag)
        else:              arg_list.append(u'--%s' % flag)
        if not value==None:arg_list.append(value)
    return arg_list


pages=[]

pages.append('TypePage')
class TypePage(QWizardPage):
    def __init__(self, parent):
        QWizardPage.__init__(self, parent)
        self.setTitle(i18n("What do you want to do?"))
        self.setSubTitle(i18n("Identify the kind of project you want:"))
        self.group=QButtonGroup(self)
        document=QRadioButton(self)
        document.setText(i18n('Translate a document'))
        document.setChecked(True)
        gui=QRadioButton(self)
        gui.setText(i18n('Translate application interface'))
        layout=QVBoxLayout(self)
        layout.addWidget(document)
        layout.addWidget(gui)
        self.group.addButton(document,0)
        self.group.addButton(gui,1)

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
        self.odfFilePath.fileDialog().setUrl(KUrl(QDesktopServices.storageLocation(QDesktopServices.StandardLocation(QDesktopServices.DocumentsLocation))))
        self.odfDirPath.lineEdit().setText(QDesktopServices.storageLocation(QDesktopServices.StandardLocation(QDesktopServices.DocumentsLocation)))



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
        QStringListModel.__init__(self,KGlobal.locale().allLanguagesList(),parent)

    def data(self,index,role):
        if role==Qt.DecorationRole:
            code=self.stringList()[index.row()]
            if not '_' in code: code=QLocale(code).name()
            code=code[3:].toLower()
            return QVariant(QIcon(QString.fromUtf8("/usr/share/locale/l10n/%1/flag.png").arg(code)))
        elif role==Qt.DisplayRole:
            code=self.stringList()[index.row()]
            return QVariant(KGlobal.locale().languageCodeToName(code))
        return QStringListModel.data(self,index,role)

class LangPage(QWizardPage):
    def __init__(self, parent):
        QWizardPage.__init__(self, parent)
        self.setTitle(i18n("Choose source and target languages"))
        self.setSubTitle("Click on a combobox then start typing the name of the language.")
        self.languageListModel=LanguageListModel(self)
        self.sourceLang=KComboBox(self)
        self.targetLang=KComboBox(self)
        self.sourceLang.setModel(self.languageListModel)
        self.targetLang.setModel(self.languageListModel)

        tlang=Project.targetLangCode()
        if tlang=='en_US':tlang=QLocale.system().name()
        tpos=self.languageListModel.stringList().indexOf(tlang)
        if tpos==-1: tpos=self.languageListModel.stringList().indexOf(QRegExp('^'+tlang[:2]+'.*'))
        if tpos!=-1: self.targetLang.setCurrentIndex(tpos)
        self.sourceLang.setCurrentIndex(self.languageListModel.stringList().indexOf('en_US'))

        layout=QFormLayout(self)
        layout.addRow(i18n("Source:"),self.sourceLang)
        layout.addRow(i18n("Target:"),self.targetLang)

        self.registerField('source-lang',self.sourceLang)
        self.registerField('target-lang',self.targetLang)



##########################


pages.append('SoftSourcePage')
class SoftSourcePage(QWizardPage):
    def __init__(self, parent):
        QWizardPage.__init__(self, parent)
        self.setTitle(i18n("Choose a type of software project"))
        self.setSubTitle(i18n("Different projects use different translation files filesystem layout."))
        self.group=QButtonGroup(self)
        self.kde=QRadioButton(i18n('KDE'),self)
        self.kde.setChecked(True)
        self.group.addButton(self.kde,0)

        layout=QFormLayout(self)
        layout.addWidget(self.kde)
        self.registerField('soft-kde',self.kde)

    def nextId(self): return pages.index(['KdeSourcePage','generic'][self.group.checkedId()])


pages.append('KdeSourcePage')
class KdeSourcePage(QWizardPage):
    def __init__(self, parent):
        QWizardPage.__init__(self, parent)
        self.setTitle(i18n("Choose location of your software translation project"))
        self.setSubTitle(i18n("Choose whether you're already have translation files on your disk or want to download them now."))
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

        self.svn=QRadioButton(i18n('Get from svn repository\n(approx. 20 mb):'),self)
        self.group.addButton(self.svn,1)
        self.languageListModel=LanguageListModel(self)
        self.targetLang=KComboBox(self)
        self.targetLang.setModel(self.languageListModel)

        tlang=Project.targetLangCode()
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
        if os.system('which svn')!=0:
            KMessageBox.error(self,i18n("Please install 'subversion' package\nto have Lokalize download KDE translation files."),i18n("Subversion client not found"))
            return False

        self.progress.show()
        self.progress.setMaximum(6*30+10)
        QCoreApplication.processEvents()

        #localsvnroot=self.field('kde-svn-location').toString()
        localsvnroot=self.svnRootLocation.text()
        if not QFileInfo('%s/trunk' % localsvnroot).exists():
            os.system('svn -N co svn://anonsvn.kde.org/home/kde/trunk %s/trunk' % localsvnroot)
        self.reportProgress(10)

        if not QFileInfo('%s/trunk/l10n-kde4' % localsvnroot).exists():
            os.system('svn -N up %s/trunk/l10n-kde4' % localsvnroot)
        self.reportProgress(5)


        langs=KGlobal.locale().allLanguagesList()
        lang=langs[self.field('kde-svn-lang').toInt()[0]]

        os.system('svn --depth files up %s/trunk/l10n-kde4/%s' % (localsvnroot, lang))
        self.reportProgress(5)
        os.system('svn --depth infinity up %s/trunk/l10n-kde4/%s/docs' % (localsvnroot, lang))
        self.reportProgress(30)
        for langlang in [lang,'templates']:
            os.system('svn --depth infinity up %s/trunk/l10n-kde4/%s/messages' % (localsvnroot, langlang))
            self.reportProgress(30)
            os.system('svn --depth infinity up %s/trunk/l10n-kde4/%s/docmessages' % (localsvnroot, langlang))
            self.reportProgress(30)
            os.system('svn --set-depth infinity up %s/trunk/l10n-kde4/%s' % (localsvnroot, langlang))
            self.reportProgress(10)

        os.system('svn --set-depth infinity up %s/trunk/l10n-kde4/scripts' % localsvnroot)
        self.reportProgress(10)

        self.existingLocation.setUrl(KUrl("%s/trunk/l10n-kde4/%s" % (localsvnroot, lang)))

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

        self.connect(self,SIGNAL("finished(int)"),self.finished)
        self.connect(self,SIGNAL("accepted()"),self.handleAccept)

    def handleAccept(self):
        fs=lambda name:self.field(name).toString()
        fi=lambda name:self.field(name).toInt()[0]
        fb=lambda name:self.field(name).toBool()

        kinds=['odf','kde']
        kind=kinds[self.page(0).group.checkedId()]
        langs=KGlobal.locale().allLanguagesList()

        doInit=True
        projectfilename='index.lokalize'
        #if kind=='odf' and fb('odf-template-source-files'):
            #projectfilename=QFileInfo(fs('odf-template-files')).baseName()+'.lokalize'

        if kind=='odf':
            loc=QDir(fs('location'))
            loc.mkdir(fs('name'))
            ppath=loc.absoluteFilePath(fs('name')+'/'+projectfilename)
            targetlang=langs[fi('target-lang')]
            name=fs('name').toLower()
        elif kind=='kde':
            name='kde'
            loc=QDir(fs('kde-existing-location'))
            targetlang=loc.dirName()
            print 'project lang: %s' % targetlang

            l=loc.entryList(QStringList('*.lokalize'), QDir.Filter(QDir.Files))
            if len(l):
                doInit=False
                projectfilename=l[0]
            ppath=loc.absoluteFilePath(projectfilename)


        if doInit: Project.init(ppath, kind, name+'-'+targetlang, langs[fi('source-lang')],targetlang)

        if kind=='odf':
            files=[fs('odf-template-files')]
            for f in files:
                info=QFileInfo(fs('odf-template-files'))
                odf2xliff.main(args(unicode(f), unicode(info.absolutePath()+'/'+info.baseName()+'.xlf')))

        Lokalize.openProject(ppath)

    def finished(self, result): self.deleteLater() #remember to cleanup

myassistant=ProjectAssistant()
myassistant.show()

