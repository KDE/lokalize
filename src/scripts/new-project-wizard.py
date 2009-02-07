# -*- coding: utf-8 -*-
import Kross
import Lokalize
import Project
import sys,os

T = Kross.module("kdetranslation")
def i18n(text, args = []):
    if T is not None: return T.i18n(text, args)
    # No translation module, return the untranslated string
    for a in range(len(args)): text = text.replace( ("%" + "%d" % ( a + 1 )), str(args[a]) )
    return text

def do():
    
    forms = Kross.module("forms")
    myassistant = forms.createAssistant(i18n("New Project Wizard"))
    myassistant.showHelpButton(0)
    projectTypePage = myassistant.addPage(i18n("Project Type"),i18n("What you want to do"))
    #l=forms.createLayout(projectTypePage, 'QVBoxLayout')
    document = forms.createWidget(projectTypePage, 'QRadioButton')
    document.text=i18n('Translate a document')
    document.checked=True
    gui = forms.createWidget(projectTypePage, 'QRadioButton')
    gui.text=i18n('Translate application interface')

    odfSourcePage = myassistant.addPage(i18n("Document to translate"),i18n("Choose a document to be translated"))
    #l=forms.createLayout(projectType, 'QVBoxLayout')
    odfFileChooser = forms.createFileWidget(odfSourcePage)

    print dir(myassistant)
    def nextClicked():
        print 11
        myassistant.setAppropriate("name2",0)
    def finished():
        print 22
        myassistant.deleteLater() #remember to cleanup

    myassistant.connect("nextClicked()",nextClicked)
    myassistant.connect("finished()",finished)
    myassistant.show()

do()
