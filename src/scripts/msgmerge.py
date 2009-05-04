# -*- coding: utf-8 -*-
import Kross,Lokalize,Project,Editor
import sys,os,re,string,codecs

utf8_decoder=codecs.getdecoder("utf8")
T = Kross.module("kdetranslation")
def i18n(text, args = []):
    if T is not None: return utf8_decoder(T.i18n(text, args))[0]
    # No translation module, return the untranslated string
    for a in range(len(args)): text = text.replace( ("%" + "%d" % ( a + 1 )), str(args[a]) )
    return text



def merge():
    if Lokalize.projectOverview(): files=Lokalize.projectOverview().selectedItems()
    elif Editor.isValid():         files=[Editor.currentFile()]

    if os.system('which msgmerge')!=0:
        forms = Kross.module("forms")
        forms.showMessageBox("Error", i18n("Gettext not found"), i18n("Install gettext package for this feature to work"))

    progress=0
    if len(files)>1:
        forms = Kross.module("forms")
        progress=forms.showProgressDialog(i18n("Updating from templates..."), "")
        progress.setRange(0,len(files))
        #progress.setMaximum(len(files))
        counter=0

    for po in files:
        if progress:
            progress.addText(po)
            progress.setValue(counter)
            counter+=1
        mergeOne(po)


    if progress:
        progress.deleteLater()

def mergeOne(po):
    if po=='' or not po.endswith('.po'): return False
    if Project.translationsRoot() not in po: return False

    (path, pofilename)=os.path.split(po)
    path=path.replace(Project.translationsRoot(),Project.templatesRoot())

    (package, ext)=os.path.splitext(pofilename)
    pot=path+'/'+package+'.pot'

    pomtime=os.path.getmtime(po)
    os.system('msgmerge --previous -U %s %s' % (po,pot))
    if pomtime==os.path.getmtime(po):
        return False

    editor=Lokalize.editorForFile(po)
    if editor:
        editor.reloadFile()

    return True

merge()
