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
    myassistant = forms.createAssistant("MyAssistant")
    myassistant.showHelpButton(0)
    mypage = myassistant.addPage("name","header")
    mywidget = forms.createWidget(mypage, 'QLabel')
    mywidget.setText('test!')

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
