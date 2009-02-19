# -*- coding: utf-8 -*-
import Kross
import Lokalize
import Project
import sys,os
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyKDE4.kdecore import *
from PyKDE4.kdeui import *
from PyKDE4.kio import *

T = Kross.module("kdetranslation")
def i18n(text, args = []):
    if T is not None: return T.i18n(text, args)
    # No translation module, return the untranslated string
    for a in range(len(args)): text = text.replace( ("%" + "%d" % ( a + 1 )), str(args[a]) )
    return text



pages=[]

pages.append('type')
class TypePage(QWizardPage):
    def __init__(self, parent):
        QWizardPage.__init__(self, parent)
        self.setTitle(i18n("What do you want to do?"))
        self.setSubTitle(i18n("Let's identify the kind of project you want."))
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

    def nextId(self): return pages.index(['odf','soft'][self.group.checkedId()])

pages.append('odf')
class OdfSourcePage(QWizardPage):
    def __init__(self, parent):
        QWizardPage.__init__(self, parent)
        self.setTitle(i18n("Choose a document to be translated"))
        self.setSubTitle(i18n("Choose document in a source language."))
        self.group=QButtonGroup(self)
        self.files=QRadioButton(self)
        self.files.setText(i18n('Select file:'))
        self.files.setChecked(True)
        self.group.addButton(self.files,0)
        self.odfFilePath=KUrlRequester(self)
        self.odfFilePath.setFilter('*.odt *.ods|OpenDocument files')
        self.odfFilePath.setMode(KFile.Modes(KFile.File or KFile.ExistingOnly or KFile.LocalOnly))

        self.dirs=QRadioButton(self)
        self.dirs.setText(i18n('Select a directory:'))
        self.group.addButton(self.dirs,1)
        self.odfDirPath=KUrlRequester(self)
        self.odfDirPath.setMode(KFile.Modes(KFile.Directory or KFile.ExistingOnly or KFile.LocalOnly))

        self.connect(self.odfFilePath,SIGNAL("textChanged(QString)"),self,SIGNAL("completeChanged()"))
        self.connect(self.odfDirPath.lineEdit(),SIGNAL("textChanged(QString)"),self,SIGNAL("completeChanged()"))
        self.connect(self.group,SIGNAL("buttonClicked(int)"),self,SIGNAL("completeChanged()"))

        layout=QFormLayout(self)
        layout.addRow(self.files,self.odfFilePath)
        layout.addRow(self.dirs,self.odfDirPath)
       

    def nextId(self): return pages.index('name')

    def initializePage(self):
        #self.registerField('odf-template-source-files',self.files)
        self.registerField('odf-template-source-files',self.files)
        self.registerField('odf-template-source-dirs',self.dirs)
        self.registerField('odf-template-files',self.odfFilePath.lineEdit())
        self.registerField('odf-template-dir',self.odfDirPath.lineEdit())

        self.odfFilePath.fileDialog().setUrl(KUrl(QDesktopServices.storageLocation(QDesktopServices.StandardLocation(QDesktopServices.DocumentsLocation))))
        self.odfDirPath.lineEdit().setText(QDesktopServices.storageLocation(QDesktopServices.StandardLocation(QDesktopServices.DocumentsLocation)))

    def isComplete(self):
        #if self.group.checkedId()==0: return QFile.exists(self.odfFilePath.lineEdit().text())
        #else:                         return QFile.exists(self.odfFilePath.lineEdit().text())
        widgets=[self.odfFilePath,self.odfDirPath]
        return QFileInfo(widgets[self.group.checkedId()].lineEdit().text()).exists()
        

pages.append('name')
class NamePage(QWizardPage):
    def __init__(self, parent):
        QWizardPage.__init__(self, parent)
        self.setTitle(i18n("Choose project name and location"))
        self.setSubTitle(i18n("If you choose custom paths then the source files will be copied to it."))
        layout=QFormLayout(self)
        self.projectName=KLineEdit(self)
        layout.addRow(i18n("Name:"),self.projectName)
        self.projectLocation=KUrlRequester(self)
        self.projectLocation.setMode(KFile.Modes(KFile.Directory or KFile.ExistingOnly or KFile.LocalOnly))
        layout.addRow(i18n("Location:"),self.projectLocation)

        self.registerField('name*',self.projectName)
        self.registerField('location*',self.projectLocation.lineEdit())

    def nextId(self): return pages.index('languages')

    def initializePage(self):
        if self.field('odf-template-source-files').toBool():
            p=self.field('odf-template-files').toString()
            pp=QFileInfo(p).absolutePath()
            self.projectLocation.lineEdit().setText(QFileInfo(pp).absolutePath())
            self.projectName.setText(QFileInfo(pp).fileName())
        else:
            p=self.field('odf-template-dir').toString()
            self.projectLocation.lineEdit().setText(QFileInfo(p).absolutePath())
            self.projectName.setText(QFileInfo(p).fileName())

        #field(fields[self.page(0).group.checkedId()]).toString()
        #self.projectLocation.lineEdit().setText(QDesktopServices.storageLocation(QDesktopServices.StandardLocation.DocumentsLocation))

pages.append('languages')
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

    #def nextId(self): return pages.index('name')



class ProjectAssistant(QWizard):
    def __init__(self):
        QWizard.__init__(self)
        self.addPage(TypePage(self))
        self.addPage(OdfSourcePage(self))
        self.addPage(NamePage(self))
        self.addPage(LangPage(self))

        self.connect(self,SIGNAL("finished(int)"),self.finished)
        self.connect(self,SIGNAL("accepted()"),self.handleAccept)

    def handleAccept(self):
        f=lambda name:self.field(name).toString()
        fi=lambda name:self.field(name).toInt()[0]

        kinds=['odf','kde']

        loc=QDir(f('location'))
        loc.mkdir(f('name'))
        ppath=loc.absoluteFilePath(f('name')+'/index.lokalize')

        langs=self.page(pages.index('languages')).languageListModel.stringList()
        Project.init(ppath, kinds[self.page(0).group.checkedId()],
                    f('name').toLower()+'-'+langs[fi('target-lang')],
                    langs[fi('source-lang')],langs[fi('target-lang')])


        Lokalize.openProject(ppath)

    def finished(self, result): self.deleteLater() #remember to cleanup

myassistant=ProjectAssistant()
myassistant.show()

